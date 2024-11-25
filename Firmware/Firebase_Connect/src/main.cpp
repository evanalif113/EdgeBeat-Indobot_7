#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "time.h"

// Informasi Wi-Fi
#define WIFI_SSID "MELON_KEBAKALAN"
#define WIFI_PASSWORD "Melon1234#"

// Informasi Firebase
#define API_KEY "YOUR_FIREBASE_API_KEY"
#define DATABASE_URL "staklimjerukagung-default-rtdb.asia-southeast1.firebasedatabase.app"
#define USER_EMAIL "YOUR_EMAIL@example.com"
#define USER_PASSWORD "YOUR_PASSWORD"

// Objek Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// UID pengguna dan jalur database
String uid;
String databasePath = "/rekaman";
String parentPath;

// Jalur data anak
String heartbeatPath = "/heartbeat";
String spoPath = "/spo";
String suhuTubuhPath = "/suhu_tubuh";
String timePath = "/timestamp";

// Variabel waktu
const char* ntpServer = "pool.ntp.org";
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 60000; // Kirim data setiap 60 detik

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

// Inisialisasi waktu dengan NTP
void initTime() {
  configTime(7 * 3600, 0, ntpServer);
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Gagal mendapatkan waktu!");
    return;
  }
  Serial.println(&timeinfo, "Waktu saat ini: %A, %d %B %Y %H:%M:%S");
}

// Dapatkan waktu saat ini sebagai timestamp
unsigned long getTime() {
  time_t now;
  time(&now);
  return now;
}

// Konfigurasi Firebase
void initFirebase() {
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  Firebase.begin(&config, &auth);

  Serial.println("Mengambil UID pengguna...");
  while (auth.token.uid == "") {
    delay(1000);
    Serial.print(".");
  }
  uid = auth.token.uid.c_str();
  Serial.println("\nUID berhasil didapatkan: " + uid);
}

// Generate data acak untuk heartbeat, spo, suhu tubuh
float generateRandomData(float min, float max) {
  return min + (float)(rand()) / ((float)(RAND_MAX / (max - min)));
}

void setup() {
  Serial.begin(115200);

  // Inisialisasi
  initWiFi();
  initTime();
  initFirebase();
}

void loop() {
  // Kirim data setiap interval tertentu
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    // Dapatkan pembacaan data acak
    float heartbeat = generateRandomData(60, 100);   // Contoh: 60-100 bpm
    float spo = generateRandomData(90, 100);          // Contoh: 90-100%
    float suhu_tubuh = generateRandomData(36.5, 37.5); // Contoh: 36.5-37.5°C
    unsigned long timestamp = getTime();

    // Buat jalur parent dengan timestamp
    parentPath = databasePath + "/" + String(timestamp);

    // Kirim data ke Firebase
    FirebaseJson json;
    json.set(heartbeatPath.c_str(), heartbeat);
    json.set(spoPath.c_str(), spo);
    json.set(suhuTubuhPath.c_str(), suhu_tubuh);
    json.set(timePath.c_str(), String(timestamp));

    Serial.printf("Mengirim data ke Firebase...\n");
    if (Firebase.RTDB.setJSON(&fbdo, parentPath.c_str(), &json)) {
      Serial.println("Data berhasil dikirim.");
    } else {
      Serial.println("Gagal mengirim data: " + fbdo.errorReason());
    }
  }
}
