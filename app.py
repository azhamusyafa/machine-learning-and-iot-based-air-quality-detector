from flask import Flask, render_template, jsonify
import firebase_admin
from firebase_admin import credentials, db
import pickle
import numpy as np
from datetime import datetime

app = Flask(__name__)

# Initialize Firebase Admin
cred = credentials.Certificate("E:\kuliah\iot x ml\iot-x-ml-5e0a7-firebase-adminsdk-ivniz-8003ac9b57.json")
firebase_admin.initialize_app(cred, {
    'databaseURL': 'https://iot-x-ml-5e0a7-default-rtdb.asia-southeast1.firebasedatabase.app/'
})

# Load the ML model
with open('model.pkl', 'rb') as file:
    model = pickle.load(file)

# Air Quality Thresholds (matching ESP32 thresholds)
GOOD_THRESHOLD = 200
MODERATE_THRESHOLD = 300

def get_air_quality_status(value):
    if value <= GOOD_THRESHOLD:
        return "Good", "green"
    elif value <= MODERATE_THRESHOLD:
        return "Moderate", "yellow"
    else:
        return "Bad", "red"

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/get_sensor_data')
def get_sensor_data():
    try:
        # Get real-time data from Firebase
        ref = db.reference('/')
        data = ref.get()
        
        air_quality = data.get('AirQuality', 0)
        status, led_color = get_air_quality_status(air_quality)
        
        # Update LED status in Firebase
        led_status = {
            'LED_GREEN': led_color == 'green',
            'LED_YELLOW': led_color == 'yellow',
            'LED_RED': led_color == 'red',
            'FAN': led_color == 'red'  # Fan turns on when air quality is bad
        }
        
        # Update LED status in Firebase
        db.reference('/LED_Status').set(led_status)
        
        return jsonify({
            'success': True,
            'airQuality': air_quality,
            'temperature': data.get('Temperature', 0),
            'humidity': data.get('Humidity', 0),
            'status': status,
            'led_color': led_color,
            'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        })
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)})

@app.route('/predict')
def predict():
    try:
        # Get latest sensor data
        ref = db.reference('/')
        data = ref.get()
        
        # Prepare features for prediction
        features = np.array([[
            data.get('AirQuality', 0),
            data.get('Temperature', 0),
            data.get('Humidity', 0)
        ]])
        
        # Make prediction
        prediction = model.predict(features)[0]
        
        return jsonify({
            'success': True,
            'prediction': str(prediction),
            'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        })
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)})

if __name__ == '__main__':
    app.run(debug=True)