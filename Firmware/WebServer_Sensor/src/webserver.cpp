#include <WiFi.h>
#include <Wire.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"
#include <ArduinoJson.h>
#include <Update.h>
#include <BH1750.h>

const char* ssid = "server";
const char* password = "jeris6467";

WebServer server(80);
BH1750 lightMeter;
Adafruit_SSD1306 display(128, 64, &Wire, -1);

float denyutnadi, spo, suhu;

void setup() {
    Serial.begin(115200);
    Wire.begin();

    // Inisialisasi Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
    Serial.println(WiFi.localIP());

    // Inisialisasi LittleFS
    if (!LittleFS.begin()) {
        Serial.println("An error has occurred while mounting LittleFS");
        return;
    }
    Serial.println("LittleFS mounted successfully");

    // Inisialisasi BH1750
    if (!lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
        Serial.println("Error initializing BH1750");
    } else {
        Serial.println("BH1750 initialized");
    }

    // Inisialisasi OLED SSD1306
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("SSD1306 allocation failed");
        for (;;);
    }
    display.display();
    delay(2000);
    display.clearDisplay();

    // Menyajikan file `index.html` dari LittleFS
    server.serveStatic("/", LittleFS, "/index.html");

    // Rute untuk mendapatkan data acak
    server.on("/data", HTTP_GET, []() {
        denyutnadi = lightMeter.readLightLevel(); // Baca data dari sensor BH1750
        spo = random(0, 100); // Data acak kedua antara 0 dan 100
        suhu = random(0, 100); // Data acak ketiga antara 0 dan 100

        JsonDocument doc;
        doc["value1"] = denyutnadi;
        doc["value2"] = spo;
        doc["value3"] = suhu;
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    });

    server.begin();
    Serial.println("HTTP server started");
}

void loop() {
    server.handleClient();
    // Tampilkan data di OLED SSD1306
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.print("Denyut Nadi: ");
    display.println(denyutnadi);
    display.print("Saturasi O2: ");
    display.println(spo);
    display.print("Suhu Tubuh: ");
    display.println(suhu);
    display.display();
}
