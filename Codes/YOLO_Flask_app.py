import cv2
import numpy as np
import requests
import time
import eventlet
from flask import Flask, Response, jsonify

eventlet.monkey_patch()
app = Flask(__name__)

YOLO_CFG = "yolov4-tiny.cfg"
YOLO_WEIGHTS = "yolov4-tiny.weights"
COCO_NAMES = "coco.names"
CAMERA_URL = "http://<Cam_IP>/capture"

target_classes = {"person", "cat", "dog", "horse", "sheep", "cow"}
net = cv2.dnn.readNet(YOLO_WEIGHTS, YOLO_CFG)
net.setPreferableBackend(cv2.dnn.DNN_BACKEND_OPENCV)
net.setPreferableTarget(cv2.dnn.DNN_TARGET_CPU)
with open(COCO_NAMES, "r") as f:
    classes = [line.strip() for line in f.readlines()]
output_layers = [net.getLayerNames()[i - 1] for i in net.getUnconnectedOutLayers().flatten()]

def get_esp32_frame():
    try:
        response = requests.get(CAMERA_URL, timeout=2)
        img_array = np.frombuffer(response.content, np.uint8)
        return cv2.imdecode(img_array, cv2.IMREAD_COLOR)
    except Exception as e:
        print("Error fetching frame:", e)
        return None

def generate_frames():
    global latest_detected_class
    frame_count = 0
    while True:
        frame = get_esp32_frame()
        if frame is None:
            time.sleep(0.1)
            continue
        frame_count += 1
        if frame_count % 2 != 0:
            time.sleep(0.05)
            continue
        height, width = frame.shape[:2]
        blob = cv2.dnn.blobFromImage(
            cv2.resize(frame, (416, 416)), 1 / 255.0, (416, 416), swapRB=True, crop=False
        )
        net.setInput(blob)
        outputs = net.forward(output_layers)
        detected_name = "None"
        for output in outputs:
            for detection in output:
                scores = detection[5:]
                class_id = np.argmax(scores)
                confidence = scores[class_id]
                class_name = classes[class_id]
                if confidence > 0.5 and class_name in target_classes:
                    detected_name = class_name
                    center_x = int(detection[0] * width)
                    center_y = int(detection[1] * height)
                    w = int(detection[2] * width)
                    h = int(detection[3] * height)
                    x = int(center_x - w / 2)
                    y = int(center_y - h / 2)
                    cv2.rectangle(frame, (x, y), (x + w, y + h), (0, 255, 0), 2)
                    cv2.putText(frame, f"{class_name} ({int(confidence * 100)}%)", 
                               (x, y - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)
                    break
            if detected_name != "None":
                break
        latest_detected_class = detected_name
        _, buffer = cv2.imencode('.jpg', frame)
        frame_bytes = buffer.tobytes()
        yield (b'--frame\r\n'
               b'Content-Type: image/jpeg\r\n\r\n' + frame_bytes + b'\r\n')
        time.sleep(0.2)

@app.route('/')
def index():
    return """
    <html>
    <head><title>YOLO Stream</title></head>
    <body style="background-color: black; text-align: center;">
        <h2 style="color: white;">YOLO Object Detection</h2>
        <img src='/video' style='width: 100%; max-width: 1000px; height: auto; object-fit: cover; border: 2px solid white;'><br>
        <h3 style="color: lightgreen;">Detected Class: <span id="detected">None</span></h3>
        <script>
            async function updateClass() {
                try {
                    const response = await fetch('/class');
                    const data = await response.json();
                    document.getElementById("detected").textContent = data.detected_class;
                } catch (e) {
                    console.error("Failed to fetch detected class");
                }
                setTimeout(updateClass, 1000);
            }
            updateClass();
        </script>
    </body>
    </html>
    """

@app.route('/video')
def video():
    return Response(generate_frames(), mimetype='multipart/x-mixed-replace; boundary=frame')

@app.route('/class')
def get_class():
    return jsonify({"detected_class": latest_detected_class})

if __name__ == '__main__':
    from eventlet import wsgi
    import logging
    log = logging.getLogger('eventlet.wsgi.server')
    log.setLevel(logging.ERROR)
    print("Starting Flask app.....")
    wsgi.server(eventlet.listen(('0.0.0.0', 5000)), app, log_output=False)
