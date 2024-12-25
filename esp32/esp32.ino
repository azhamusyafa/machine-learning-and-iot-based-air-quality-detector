#include <DHT11.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Definisi Pin
#define MQ135_PIN 34         // Pin Analog untuk MQ-135
#define LAMP_RED_PIN 25      // Pin Digital untuk Lampu Merah
#define LAMP_YELLOW_PIN 26   // Pin Digital untuk Lampu Kuning
#define LAMP_GREEN_PIN 27    // Pin Digital untuk Lampu Hijau
#define Kipas_PIN 14         // Pin Digital untuk Kipas
DHT11 dht(14);

// Wi-Fi dan Firebase credentials
#define WIFI_SSID "Fiaa"
#define WIFI_PASSWORD "12345678"
#define API_KEY "ur token"
#define DATABASE_URL "firebase"

// Firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

void setup() {
  Serial.begin(115200);

  // Inisialisasi Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());

  // Konfigurasi Firebase
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;

  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("Firebase SignUp OK");
    signupOK = true;
  } else {
    Serial.printf("SignUp Error: %s\n", config.signer.signupError.message.c_str());
  }

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Konfigurasi Pin
  pinMode(LAMP_RED_PIN, OUTPUT);
  pinMode(LAMP_YELLOW_PIN, OUTPUT);
  pinMode(LAMP_GREEN_PIN, OUTPUT);
  pinMode(Kipas_PIN, OUTPUT);

  Serial.println("Sistem Monitoring Kualitas Udara Dimulai.");
}

void loop() {
  // Membaca nilai dari sensor MQ-135
  int airQualityValue = analogRead(MQ135_PIN);
  
  // Membaca suhu dan kelembaban dari DHT
  float suhu = dht.readTemperature();
  float kelembaban = dht.readHumidity();

  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 5000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    // Upload sensor data ke Firebase
    Firebase.RTDB.setInt(&fbdo, "/AirQuality", airQualityValue);
    Firebase.RTDB.setFloat(&fbdo, "/Temperature", suhu);
    Firebase.RTDB.setFloat(&fbdo, "/Humidity", kelembaban);

    // Membaca status LED dari Firebase
    if (Firebase.RTDB.getBool(&fbdo, "/LED_Status/LED_GREEN")) {
      bool greenLED = fbdo.boolData();
      digitalWrite(LAMP_GREEN_PIN, greenLED ? HIGH : LOW);
    }
    
    if (Firebase.RTDB.getBool(&fbdo, "/LED_Status/LED_YELLOW")) {
      bool yellowLED = fbdo.boolData();
      digitalWrite(LAMP_YELLOW_PIN, yellowLED ? HIGH : LOW);
    }
    
    if (Firebase.RTDB.getBool(&fbdo, "/LED_Status/LED_RED")) {
      bool redLED = fbdo.boolData();
      digitalWrite(LAMP_RED_PIN, redLED ? HIGH : LOW);
    }
    
    if (Firebase.RTDB.getBool(&fbdo, "/LED_Status/FAN")) {
      bool fan = fbdo.boolData();
      digitalWrite(Kipas_PIN, fan ? HIGH : LOW);
    }

    // Print status to Serial
    Serial.print("Air Quality: ");
    Serial.println(airQualityValue);
    Serial.print("Temperature: ");
    Serial.println(suhu);
    Serial.print("Humidity: ");
    Serial.println(kelembaban);
  }

  delay(5000);
}
