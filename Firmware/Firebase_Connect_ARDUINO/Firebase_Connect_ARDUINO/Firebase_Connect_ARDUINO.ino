#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "time.h"
#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include "SSD1306Wire.h" // Library SSD1306


// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Informasi Wi-Fi
#define WIFI_SSID "server" //Nama Wifi
#define WIFI_PASSWORD "jeris6467" //Password Wifi

// Informasi Firebase
#define API_KEY "AIzaSyA-GgbbQkIOWRJ2nYgGkYTq8EaN4h1R2HQ"
#define DATABASE_URL "https://edgebeat-indobot7-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define USER_EMAIL "untukalat@gmail.com"
#define USER_PASSWORD "alat12345"

#define SCREEN_WIDTH 128 // Lebar OLED
#define SCREEN_HEIGHT 64 // Tinggi OLED
#define OLED_RESET    -1 // Reset pin (gunakan -1 jika tidak ada)

MAX30105 particleSensor;
SSD1306Wire display(0x3c, 2, 3); // Initialize OLED display (I2C address, SDA, SCL)

uint32_t irBuffer[100];   // Data sensor LED inframerah
uint32_t redBuffer[100];  // Data sensor LED merah

int32_t bufferLength;     // Panjang data
int32_t spo2;             // Nilai SPO2
int8_t validSPO2;         // Validitas perhitungan SPO2
int32_t heartRate;        // Nilai detak jantung
int8_t validHeartRate;    // Validitas perhitungan detak jantung
uint calibratedTemperature;

const uint8_t heart_icon[] = {
  0b01110000, 0b00001110, // Row 1
  0b11111000, 0b00011111, // Row 2
  0b11111100, 0b00111111, // Row 3
  0b11111110, 0b01111111, // Row 4
  0b11111110, 0b01111111, // Row 5
  0b11111110, 0b01111111, // Row 6
  0b11111100, 0b00111111, // Row 7
  0b11111000, 0b00011111, // Row 8
  0b11110000, 0b00001111, // Row 9
  0b11100000, 0b00000111, // Row 10
  0b11000000, 0b00000011, // Row 11
  0b10000000, 0b00000001, // Row 12
  0b00000000, 0b00000000, // Row 13
  0b00000000, 0b00000000, // Row 14
  0b00000000, 0b00000000, // Row 15
  0b00000000, 0b00000000  // Row 16

};

// Objek Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseJson json;

// UID pengguna dan jalur database
String uid;
String databasePath;
String parentPath;

String documentPath = "pendaftaran";
unsigned int ledPin = 2;
int timestamp;
int status;
int id;

// Jalur data anak
String heartbeatPath = "/heartbeat";
String spoPath = "/spo";
String suhuTubuhPath = "/suhu_tubuh";
String timePath = "/timestamp";

// Variabel waktu
const char* ntpServer = "pool.ntp.org";
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 2500; // Kirim data setiap 60 detik

// Inisialisasi Wi-Fi
void initWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Menghubungkan ke WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nWiFi terhubung: " + WiFi.localIP().toString());
}

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return (0);
  }
  time(&now);
  return now;
}

void initSensor() {
  Wire.begin(2, 3); // Inisialisasi I2C dengan pin SDA=2, SCL=3
  delay(500);

  // Initialize OLED display
  display.init();
  display.setFont(ArialMT_Plain_16);
  display.flipScreenVertically(); // Rotate display 180 degrees

  // Initialize MAX30105 sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println(F("MAX30102 tidak ditemukan"));
    while (1);
  }

  // Sensor configuration
  particleSensor.setup(60, 4, 2, 100, 411, 4096); // LED pulse width, sample rate, LED current
}

void getData() {
    // Read temperature and calibrate
  float temperature = particleSensor.readTemperature();
  calibratedTemperature = temperature - 5;

  // Collect heart rate and SpO2 data
  bufferLength = 100; // Data length for calculation
  for (byte i = 0; i < bufferLength; i++) {
    while (particleSensor.available() == false) {
      particleSensor.check();
    }
    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample();
  }

  // Calculate heart rate and SpO2
  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  // Display data on OLED
  display.clear();

  // Display the heart icon at (0,0)
  display.drawXbm(0, 0, 16, 16, (const char*)heart_icon); // Cast to const char*

  // Display heart rate next to the heart icon
  display.drawString(20, 0, String(heartRate) + " bpm");

  // Display SpO2 and calibrated temperature
  display.drawString(0, 20, "SpO2: " + String(spo2) + "%");
  display.drawString(0, 40, "Temp: " + String(calibratedTemperature, 1) + " C");

  display.display();
}

// Konfigurasi Firebase
void FirebaseSetup() {
  configTime(0, 0, ntpServer); //NTP Server
  // Assign the api key (required)
  config.api_key = API_KEY;
  // Assign the user sign in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;
  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);
  // Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authen and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }
  // Print user UID
  String uid = auth.token.uid.c_str();
  Serial.print("User UID: "); Serial.println(uid);
}

void ReadLatestID() {
  String idPath = "/id";
  if (Firebase.RTDB.getInt(&fbdo, idPath.c_str())) {
    id = fbdo.intData();
    Serial.print("ID Terakhir: ");
    Serial.println(id);
  } else {
    Serial.print("Gagal mendapatkan ID Terakhir: ");
    Serial.println(fbdo.errorReason());
  }
}

void SendRekamanFirebase() {
    // Update database path
    databasePath = "/rekaman";
    timestamp = getTime();
    int espheapram = ESP.getFreeHeap();
    Serial.println (timestamp);
    Serial.println (espheapram);
    
    // Dapatkan pembacaan data acak
    unsigned long timestamp = getTime();

    parentPath= databasePath + "/" + id + "/" + String(timestamp);

    json.set(heartbeatPath.c_str(), String(heartRate));
    json.set(spoPath.c_str(), String(spo2));
    json.set(suhuTubuhPath.c_str(), String(calibratedTemperature));
    json.set(timePath.c_str(), String(timestamp));

    Serial.printf("Set json... %s\n", 
    Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
   
    Serial.println(ESP.getFreeHeap());
}

void ReadStatusFirebase() {
  String statusPath = "/statusrekam";
  if (Firebase.RTDB.getInt(&fbdo, statusPath.c_str())) {
    status = fbdo.intData();
    Serial.print("Status rekam: ");
    Serial.println(status);
  } else {
    Serial.print("Gagal mendapatkan status rekam: ");
    Serial.println(fbdo.errorReason());
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  initSensor();
  initWiFi();
  FirebaseSetup();
}

void loop() {
  // Kirim data setiap interval tertentu
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)) {
    ReadStatusFirebase();
    ReadLatestID();
    if (status == 1) {
    SendRekamanFirebase();
    }
    digitalWrite(ledPin, status);
    sendDataPrevMillis = millis();
  }
}
