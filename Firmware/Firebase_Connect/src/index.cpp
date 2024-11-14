#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <Update.h>


const char* ssid = "server";
const char* password = "jeris6467";

WebServer server(80);

void setup() {
    Serial.begin(115200);

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

    // Menyajikan file `index.html` dari LittleFS
    server.serveStatic("/", LittleFS, "/index.html");

    // Rute untuk mendapatkan data acak
    server.on("/data", HTTP_GET, []() {
        JsonDocument doc;
        doc["value1"] = random(0, 100); // Data acak pertama antara 0 dan 100
        doc["value2"] = random(0, 100); // Data acak kedua antara 0 dan 100
        doc["value3"] = random(0, 100); // Data acak ketiga antara 0 dan 100
        String response;
        serializeJson(doc, response);
        server.send(200, "application/json", response);
    });

    server.begin();
    Serial.println("HTTP server started");
}

void loop() {
    server.handleClient();
}
