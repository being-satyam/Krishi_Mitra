import eventlet
eventlet.monkey_patch()

from flask import Flask, render_template, request, jsonify, send_file
from flask_socketio import SocketIO
from datetime import datetime
import matplotlib.pyplot as plt
import io
import csv

app = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins="*", async_mode='eventlet')

moisture_data = []

@app.route('/')
def index():
    return render_template('agriot.html')

@app.route('/data', methods=['POST'])
def handle_data():
    try:
        data = request.json

        moisture = float(data.get("moisture", 0))
        pump_str = data.get("pump", "OFF")
        motion = True if data.get("pir", 0) == 1 else False
        water_level = float(data.get("water_level", 0))

        timestamp = datetime.now().strftime("%H:%M:%S")

        entry = {
            "time": timestamp,
            "value": moisture,
            "motion": motion,
            "motor": pump_str == "ON",
            "water_level": water_level
        }

        moisture_data.append(entry)
        print(f"[Received] {timestamp}: moisture={moisture}, motion={motion}, motor={pump_str}, water_level={water_level}")

        socketio.emit('moisture_update', entry)

        return jsonify({"status": "success"}), 200
    except Exception as e:
        print(f"Error in /data: {e}")
        return jsonify({"error": str(e)}), 500


@app.route('/plot')
def download_plot():
    try:
        times = [row['time'] for row in moisture_data]
        moistures = [row['value'] for row in moisture_data]
        water_levels = [row['water_level'] for row in moisture_data]

        plt.figure(figsize=(10, 5))
        plt.plot(times, moistures, label='Moisture (%)', color='green', marker='o')
        plt.plot(times, water_levels, label='Water Level (%)', color='red', marker='x')
        plt.title('Moisture & Water Level Over Time')
        plt.xlabel('Time')
        plt.ylabel('Percentage (%)')
        plt.xticks(rotation=45)
        plt.legend()
        plt.tight_layout()

        img = io.BytesIO()
        plt.savefig(img, format='png')
        img.seek(0)
        plt.close()
        return send_file(img, mimetype='image/png', as_attachment=True, download_name='plot.png')
    except Exception as e:
        print(f"Error in /plot: {e}")
        return jsonify({"error": str(e)}), 500

@app.route('/export')
def export_csv():
    try:
        csv_file = io.StringIO()
        writer = csv.DictWriter(csv_file, fieldnames=["time", "value", "motion", "motor", "water_level"])
        writer.writeheader()
        writer.writerows(moisture_data)
        csv_file.seek(0)

        return send_file(
            io.BytesIO(csv_file.getvalue().encode()),
            mimetype='text/csv',
            as_attachment=True,
            download_name='moisture_data.csv'
        )
    except Exception as e:
        print(f"Error in /export: {e}")
        return jsonify({"error": str(e)}), 500

if __name__ == '__main__':
    socketio.run(app, host='0.0.0.0', port=8080)
