#include <painlessMesh.h>
#include <DHT.h>
#include <ArduinoJson.h>

#define MESH_PREFIX   "LabIoTMesh"
#define MESH_PASSWORD "iotmeshpassword"
#define MESH_PORT     5555

#define DHTPIN 2       // Pin D4
#define DHTTYPE DHT22  // Ganti DHT22 jika perlu
DHT dht(DHTPIN, DHTTYPE);

Scheduler userScheduler;
painlessMesh mesh;

void sendMessage();
Task taskSendMessage(TASK_SECOND * 3, TASK_FOREVER, &sendMessage);

void sendMessage() {
  float suhu = dht.readTemperature();
  float kelembapan = dht.readHumidity();

  if (isnan(suhu) || isnan(kelembapan)) {
    Serial.println("Gagal membaca DHT!");
    return;
  }

  // Membuat format JSON sesuai instruksi Tugas 5.7
  StaticJsonDocument<200> doc;
  doc["tipe"] = "suhu_node";
  doc["suhu"] = suhu;
  doc["kelembapan"] = kelembapan;

  String msg;
  serializeJson(doc, msg);
  mesh.sendBroadcast(msg);

  Serial.print("[KIRIM Node 1] ");
  Serial.println(msg);
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  
  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();

  Serial.println("\nNode 1 (DHT) Berjalan...");
}

void loop() {
  mesh.update();
}