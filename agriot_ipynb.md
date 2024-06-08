# Krishi_Mitra
Alerts for trespassers entered in the field and smart watering system
import requests
import time
import csv
from datetime import datetime
import matplotlib.pyplot as plt
import pandas as pd
import getpass


# Twilio configuration
TWILIO_ACCOUNT_SID = '***************'
TWILIO_AUTH_TOKEN = '********'
TWILIO_FROM_NUMBER = '***********'
TWILIO_TO_NUMBER = '+916201609470'

# Bolt configuration
BOLT_DEVICE_ID = getpass.getpass(prompt='BOLT_DEVICE_ID')
BOLT_API_KEY = getpass.getpass(prompt='BOLT_API_KEY')

# Initialize data storage
moisture_data = []
trespasser_count = 0

# Function to send SMS using Twilio
def send_sms(message):
    url = f"https://api.twilio.com/2010-04-01/Accounts/{TWILIO_ACCOUNT_SID}/Messages.json"
    data = {
        'From': TWILIO_FROM_NUMBER,
        'To': TWILIO_TO_NUMBER,
        'Body': message
    }
    response = requests.post(url, data=data, auth=(TWILIO_ACCOUNT_SID, TWILIO_AUTH_TOKEN))
    if response.status_code == 201:
        print("SMS Sent:", message)
    else:
        print(f"Failed to send SMS: {response.text}")
        



# Function to read moisture
def read_moisture():
    url = f"https://cloud.boltiot.com/remote/{BOLT_API_KEY}/analogRead"
    querystring = {"pin": "A0", "deviceName": BOLT_DEVICE_ID}
    headers = {'Cache-Control': "no-cache"}

    response = requests.get(url, headers=headers, params=querystring)
    data = response.json()

    if 'value' not in data:
        print("Error in reading moisture:", data)
        return None

    try:
        analog_value = int(data['value'])
        moisture_percentage = ((1024 - analog_value) / 1024) * 100  
        print(moisture_percentage)
        return moisture_percentage
    except (ValueError, TypeError) as e:
        print("Error in conversion to moisture:", e)
        return None

# Function to read PIR sensor data
def read_pir():
    url = f"https://cloud.boltiot.com/remote/{BOLT_API_KEY}/digitalRead"
    querystring = {"pin": "1", "deviceName": BOLT_DEVICE_ID}
    headers = {'Cache-Control': "no-cache"}

    response = requests.get(url, headers=headers, params=querystring)
    data = response.json()

    if 'value' not in data:
        print("Error in reading PIR sensor:", data)
        return None

    try:
        pir_value = int(data['value'])
        return pir_value
    except (ValueError, TypeError) as e:
        print("Error in conversion to PIR sensor data:", e)
        return None


# Function to turn on PUMP
def turn_on_pin():
    url = f"https://cloud.boltiot.com/remote/{BOLT_API_KEY}/serialWrite?data=PUMP_ON&deviceName={BOLT_DEVICE_ID}"
    response = requests.get(url)
    if response.status_code == 200:
        print("Pump turned on!")
    else:
        print(f"Failed to turn ON pump: {response.status_code}")
# Function to turn off PUMP

def turn_off_pin():
    url = f"https://cloud.boltiot.com/remote/{BOLT_API_KEY}/serialWrite?data=PUMP_OFF&deviceName={BOLT_DEVICE_ID}"
    response = requests.get(url)
    if response.status_code == 200:
        print("Pump turned off!")
    else:
       print(f"Failed to turn OFF pump: {response.status_code}")

# Main loop
moisture_check_interval = 60  # Check moisture every 60 seconds
last_moisture_check = time.time()

while True:
    current_time = time.time()

    # Read moisture data at specified intervals
    if current_time - last_moisture_check >= moisture_check_interval:
        moisture = None
        retry_count = 0
        max_retries = 3
        while moisture is None and retry_count < max_retries:
            moisture = read_moisture()
            if moisture is None:
                retry_count += 1
                time.sleep(5)

        if moisture is not None:
            # Store data
            timestamp = datetime.now().strftime('%d-%m-%Y %H:%M:%S')
            moisture_data.append([timestamp, moisture])

            # Check moisture and send alerts if necessary
            if moisture < 30:  # Adjust the threshold as needed
                send_sms(f"Soil moisture is low:{moisture} . Turning on the pump.")
                
                turn_on_pin()
                time.sleep(300)
                turn_off_pin()
        
       

    # Plot and store data every minute
            df_moisture = pd.DataFrame(moisture_data, columns=['Timestamp', 'Moisture'])
            plt.figure(figsize=(10, 5))
            plt.plot(df_moisture['Timestamp'], df_moisture['Moisture'], label='Moisture (%)')
            plt.xlabel('Timestamp')
            plt.ylabel('Moisture (%)')
            plt.title('Moisture Data Over Time')
            plt.legend()
            plt.xticks(rotation=45)
            plt.tight_layout()
            plt.savefig('moisture_data_plot.png')
            plt.close()

            # Save data to CSV
            df_moisture.to_csv('moisture_data.csv', index=False)

            # Display the plot 
            plt.show()

        last_moisture_check = current_time

         # Read PIR sensor data every 10 seconds
    pir_value = read_pir()
    if pir_value == 1:  # Motion detected
        trespasser_count += 1
        send_sms(f"Alert: Trespasser detected in the field. Total trespassers detected: {trespasser_count}")

    time.sleep(10)
        
