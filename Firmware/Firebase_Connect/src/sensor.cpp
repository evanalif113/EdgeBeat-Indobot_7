#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include "SSD1306.h" // For the OLED

MAX30105 particleSensor;
SSD1306 display(0x3c, 2, 3); // Initialize the OLED display (I2C address, SDA, SCL)

#define MAX_BRIGHTNESS 255

uint32_t irBuffer[100];   // infrared LED sensor data
uint32_t redBuffer[100];  // red LED sensor data

int32_t bufferLength;     // data length
int32_t spo2;             // SPO2 value
int8_t validSPO2;         // indicator to show if the SPO2 calculation is valid
int32_t heartRate;        // heart rate value
int8_t validHeartRate;    // indicator to show if the heart rate calculation is valid

void setup() {
  Wire.begin(2,3);
  Serial.begin(115200);
  
  // Initialize the OLED
  display.init();
 
  display.setFont(ArialMT_Plain_10);
  
  // Initialize the MAX30105 sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println(F("MAX30102 tidak ditemukan"));
    while (1);
  }
  
  // Sensor configuration
  particleSensor.setup(60, 4, 2, 100, 411, 4096);
   display.flipScreenVertically(); // Rotate the display 180 degrees
}

void loop() {
  // Read temperature
  float temperature = particleSensor.readTemperature();
  float Calibrate = temperature - 9;

  // Collect heart rate and SpO2 data
  bufferLength = 100;
  for (byte i = 0 ; i < bufferLength ; i++) {
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
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  
  display.drawString(0, 0, "Heart Rate: " + String(heartRate) + " bpm");
  display.drawString(0, 15, "SpO2: " + String(spo2) + "%");
  display.drawString(0, 30, "Temp: " + String(Calibrate, 1) + " C");
  
  display.display();
  
  delay(1000); // Update every second
}