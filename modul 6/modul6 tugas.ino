#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// Kredensial Jaringan Wi-Fi
const char* ssid = "E";
const char* password = "66666666";

// Konfigurasi Broker MQTT Publik
const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;

// IDENTITAS KLIEN (Unik)
const char* clientId = "ESP8266_LabIoT_25782096_SafetySystem";

// Pembagian Topik Tugas 6.7
const char* topicTelemetri = "kampus/labiot/25782096/telemetri";
const char* topicKendali   = "kampus/labiot/25782096/kendali";
const char* topicAlert     = "kampus/labiot/25782096/alert";
const char* topicStatus    = "kampus/labiot/25782096/status";

// Pin Perangkat Keras
const byte dhtPin = 2; // Pin D4 (GPIO 2)
const byte ledPin = 5; // Pin D1 (GPIO 5)
#define DHTTYPE DHT22

DHT dht(dhtPin, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastTelemetryTime = 0;
const unsigned long telemetryInterval = 4000;
bool isOverheat = false; // Flag penanda kondisi darurat

void callback(char* topic, byte* payload, unsigned int length) {
  char incomingBuffer[length + 1];
  memcpy(incomingBuffer, payload, length);
  incomingBuffer[length] = '\0';

  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, incomingBuffer);
  if (err) return;

  if (doc.containsKey("led")) {
    // Override manual HANYA diizinkan jika suhu dalam kondisi NORMAL
    if (!isOverheat) {
      int val = doc["led"];
      digitalWrite(ledPin, val == 1 ? HIGH : LOW);
      Serial.printf("--> [MANUAL OVERRIDE] Status LED: %s\n", val == 1 ? "MENYALA" : "PADAM");
    } else {
      Serial.println("--> [DITOLAK] Sistem OVERHEAT! Kendali manual dikunci.");
    }
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Menghubungkan ke Broker...");
    
    // Konfigurasi Pesan Wasiat (Last Will and Testament)
    const char* lwtMessage = "{\"status\":\"OFFLINE\",\"keterangan\":\"Perangkat Terputus Tak Terduga\"}";
    
    // client.connect(clientId, willTopic, willQoS, willRetain, willMessage)
    if (client.connect(clientId, topicStatus, 1, true, lwtMessage)) {
      Serial.println(" Sukses Terhubung!");

      // Kirim pesan status ONLINE segera setelah koneksi pulih (Retain = true)
      const char* onlineMessage = "{\"status\":\"ONLINE\",\"keterangan\":\"Perangkat Beroperasi Normal\"}";
      client.publish(topicStatus, onlineMessage, true);

      // Subskripsi topik perintah kendali
      client.subscribe(topicKendali);
    } else {
      Serial.printf(" Gagal (rc=%d). Coba lagi dalam 4 detik...\n", client.state());
      delay(4000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  dht.begin();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Terkoneksi!");

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastTelemetryTime >= telemetryInterval) {
    lastTelemetryTime = now;

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (!isnan(t) && !isnan(h)) {
      // 1. LOGIKA PROTEKSI KESELAMATAN (EDGE SAFETY ENGINE)
      if (t > 31.0) {
        if (!isOverheat) {
          isOverheat = true;
          digitalWrite(ledPin, HIGH); // Nyalakan LED otomatis saat BAHAYA

          StaticJsonDocument<256> alertDoc;
          alertDoc["status"] = "BAHAYA";
          alertDoc["parameter"] = "OVERHEAT";
          alertDoc["nilai_suhu"] = serialized(String(t, 2));

          char alertBuffer[256];
          serializeJson(alertDoc, alertBuffer);
          client.publish(topicAlert, alertBuffer);
          Serial.printf("[ALERT] %s\n", alertBuffer);
        }
      } else {
        if (isOverheat) {
          isOverheat = false;
          digitalWrite(ledPin, LOW); // Padamkan LED saat kondisi kembali NORMAL
          Serial.println("[NORMAL] Suhu aman. LED dipadamkan.");
        }
      }

      // 2. PUBLIKASI TELEMETRI RUTIN
      StaticJsonDocument<256> docOut;
      docOut["node_id"] = clientId;
      docOut["suhu"] = serialized(String(t, 2));
      docOut["kelembapan"] = serialized(String(h, 2));
      docOut["status_led"] = digitalRead(ledPin);
      docOut["kondisi_suhu"] = (t <= 31.0) ? "NORMAL" : "PANAS";

      char buffer[256];
      serializeJson(docOut, buffer);
      client.publish(topicTelemetri, buffer);
      Serial.printf("[TELEMETRI] %s\n", buffer);
    }
  }
}