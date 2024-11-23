#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

#define SCREEN_WIDTH 128 // Lebar OLED
#define SCREEN_HEIGHT 64 // Tinggi OLED
#define OLED_RESET    -1 // Reset pin (gunakan -1 jika tidak ada)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
MAX30105 particleSensor;

uint32_t irBuffer[100];   // Data sensor LED inframerah
uint32_t redBuffer[100];  // Data sensor LED merah

int32_t bufferLength;     // Panjang data
int32_t spo2;             // Nilai SPO2
int8_t validSPO2;         // Validitas perhitungan SPO2
int32_t heartRate;        // Nilai detak jantung
int8_t validHeartRate;    // Validitas perhitungan detak jantung

void setup() {
  Wire.begin(2, 3); // Inisialisasi I2C dengan SDA di pin 2, SCL di pin 3
  Serial.begin(115200); 

  // Inisialisasi OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Alamat I2C OLED adalah 0x3C
    Serial.println(F("OLED tidak ditemukan"));
    while (1);
  }
  display.clearDisplay(); //Memastikan OLED display clear
  display.display();

  // Inisialisasi sensor MAX30105
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println(F("MAX30102 tidak ditemukan"));
    while (1);
  }

  // Konfigurasi sensor
  particleSensor.setup(60, 4, 2, 100, 411, 4096);
}

void loop() {
  // Membaca suhu
  float temperature = particleSensor.readTemperature();
  float Calibrate = temperature - 9;

  // Mengumpulkan data detak jantung dan SpO2
  bufferLength = 100;
  for (byte i = 0; i < bufferLength; i++) {
    while (particleSensor.available() == false) {
      particleSensor.check();
    }
    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample();
  }

  // Menghitung detak jantung dan SpO2
  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  // Menampilkan data ke OLED
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(0, 0);
  display.println("Heart Rate: " + String(heartRate) + " bpm");

  display.setCursor(0, 10);
  display.println("SpO2: " + String(spo2) + "%");

  display.setCursor(0, 20);
  display.println("Temp: " + String(Calibrate, 1) + " C");

  display.display();

  delay(1000); // Update setiap detik
}