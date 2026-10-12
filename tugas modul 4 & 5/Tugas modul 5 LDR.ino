#include <painlessMesh.h>
#include <ArduinoJson.h>

#define MESH_PREFIX   "LabIoTMesh"
#define MESH_PASSWORD "iotmeshpassword"
#define MESH_PORT     5555

const int ldrPin = A0;

Scheduler userScheduler;
painlessMesh mesh;

void sendMessage();
Task taskSendMessage(TASK_SECOND * 3, TASK_FOREVER, &sendMessage);

void sendMessage() {
  int adcValue = analogRead(ldrPin);

  // Membuat format JSON sesuai instruksi Tugas 5.7
  StaticJsonDocument<200> doc;
  doc["tipe"] = "cahaya_node";
  doc["adc"] = adcValue;

  String msg;
  serializeJson(doc, msg);
  mesh.sendBroadcast(msg);

  Serial.print("[KIRIM Node 2] ");
  Serial.println(msg);
}

void setup() {
  Serial.begin(115200);
  
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  
  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();

  Serial.println("\nNode 2 (LDR) Berjalan...");
}

void loop() {
  mesh.update();
}