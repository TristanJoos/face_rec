#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

WebServer server(80);

#define LED_GREEN 2
#define LED_RED   0

bool doorState = true;
bool hasDoor = false;
String doorId = "";

// --------------------
// CORS
// --------------------
void sendCors() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, PATCH, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

// --------------------
// Handlers
// --------------------
void handleAddDoor() {
  sendCors();
  Serial.println(">>> POST /esp32/AddDoor");

  String body = server.arg("plain");
  Serial.print("Body: ");
  Serial.println(body);

  if (body.length() == 0) {
    server.send(400, "application/json", "{\"error\":\"empty body\"}");
    return;
  }

  DynamicJsonDocument doc(256);
  if (deserializeJson(doc, body)) {
    server.send(400, "application/json", "{\"error\":\"bad json\"}");
    return;
  }

  if (!doc.containsKey("doorId")) {
    server.send(400, "application/json", "{\"error\":\"doorId missing\"}");
    return;
  }

  doorId = doc["doorId"].as<String>();
  hasDoor = true;

  Serial.print("Door registered: ");
  Serial.println(doorId);

  server.send(200, "application/json",
    "{\"receivedDoorId\":\"" + doorId + "\"}"
  );
}

void handleToggle() {
  sendCors();

  if (!hasDoor) {
    server.send(400, "application/json", "{\"error\":\"No door registered\"}");
    Serial.println("Toggle requested but no door registered");
    return;
  }

  String body = server.arg("plain");
  DynamicJsonDocument doc(128);
  if (deserializeJson(doc, body)) {
    server.send(400, "application/json", "{\"error\":\"bad json\"}");
    return;
  }

  if (!doc.containsKey("doorId") || doc["doorId"].as<String>() != doorId) {
    server.send(400, "application/json", "{\"error\":\"Invalid doorId\"}");
    Serial.println("Received wrong doorId");
    return;
  }

  doorState = !doorState;
  digitalWrite(LED_GREEN, doorState ? LOW : HIGH);
  digitalWrite(LED_RED,   doorState ? HIGH : LOW);

  Serial.print("Door ");
  Serial.print(doorId);
  Serial.print(" state -> ");
  Serial.println(doorState ? "OPEN (Green LED)" : "CLOSED (Red LED)");

  server.send(200, "application/json",
    "{\"doorId\":\"" + doorId + "\",\"state\":" +
    String(doorState ? "true" : "false") + "}"
  );
}

void handleLed() {
  digitalWrite(LED_GREEN , HIGH);
  digitalWrite(LED_RED , LOW);
  delay(5000);
  digitalWrite(LED_GREEN , LOW);
  digitalWrite(LED_RED , HIGH);
}
void handleNotFound() {
  sendCors();
  Serial.print("404 -> ");
  Serial.println(server.uri());
  server.send(404, "text/plain", "Not Found");
}

// --------------------
// Setup
// --------------------
void setup() {
  Serial.begin(115200);

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, HIGH);

  Serial.println("Starting ESP32 AP...");
  WiFi.softAP("ESP32-NETWORK", "12345678");
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/esp32/AddDoor", HTTP_OPTIONS, []() {
    sendCors();
    Serial.println(">>> OPTIONS /esp32/AddDoor");
    server.send(200);
  });
  server.on("/esp32/AddDoor", HTTP_POST, handleAddDoor);

  server.on("/esp32/toggle", HTTP_OPTIONS, []() {
    sendCors();
    Serial.println(">>> OPTIONS /esp32/toggle");
    server.send(200);
  });
  server.on("/esp32/toggle", HTTP_POST, handleToggle);

  server.on("/esp32/status", HTTP_OPTIONS, []() {
    sendCors();
    server.send(200);
  });

  server.on("/esp32/led", HTTP_POST, handleLed);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
