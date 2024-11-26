#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "time.h"

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Informasi Wi-Fi
#define WIFI_SSID "server"
#define WIFI_PASSWORD "jeris6467"

// Informasi Firebase
#define API_KEY "AIzaSyA-GgbbQkIOWRJ2nYgGkYTq8EaN4h1R2HQ"
#define DATABASE_URL "https://edgebeat-indobot7-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define USER_EMAIL "untukalat@gmail.com"
#define USER_PASSWORD "alat12345"

// Objek Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
FirebaseJson json;

// Define PROJECT_ID
#define PROJECT_ID "edgebeat-indobot7"

// UID pengguna dan jalur database
String uid;
String databasePath;
String parentPath;

String documentPath = "pendaftaran";
unsigned int ledPin = 2;
int timestamp;
int status;

// Jalur data anak
String heartbeatPath = "/heartbeat";
String spoPath = "/spo";
String suhuTubuhPath = "/suhu_tubuh";
String timePath = "/timestamp";

// Variabel waktu
const char* ntpServer = "pool.ntp.org";
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 5000; // Kirim data setiap 60 detik

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

// Generate data acak untuk heartbeat, spo, suhu tubuh
float generateRandomData(float min, float max) {
  return min + (float)(rand()) / ((float)(RAND_MAX / (max - min)));
}

void DataFirebase() {
    // Update database path
    databasePath = "/rekaman";
    timestamp = getTime();
    int espheapram = ESP.getFreeHeap();
    Serial.println (timestamp);
    Serial.println (espheapram);
    
    // Dapatkan pembacaan data acak
    float heartbeat = generateRandomData(60, 100);   // Contoh: 60-100 bpm
    float spo = generateRandomData(90, 100);          // Contoh: 90-100%
    float suhu_tubuh = generateRandomData(36.5, 37.5); // Contoh: 36.5-37.5°C
    unsigned long timestamp = getTime();

    parentPath= databasePath + "/" + String(timestamp);

    json.set(heartbeatPath.c_str(), String(heartbeat));
    json.set(spoPath.c_str(), String(spo));
    json.set(suhuTubuhPath.c_str(), String(suhu_tubuh));
    json.set(timePath.c_str(), String(timestamp));

    Serial.printf("Set json... %s\n", 
    Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json) ? "ok" : fbdo.errorReason().c_str());
   
    Serial.println(ESP.getFreeHeap());
}

void readFirebase() {
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
  initWiFi();
  FirebaseSetup();
}

void loop() {
  // Kirim data setiap interval tertentu
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)) {
    readFirebase();
    DataFirebase();
    digitalWrite(ledPin, status);
    sendDataPrevMillis = millis();
  }
}
