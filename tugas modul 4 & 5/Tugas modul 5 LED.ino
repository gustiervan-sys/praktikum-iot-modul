#include <painlessMesh.h>
#include <ArduinoJson.h>

#define MESH_PREFIX   "LabIoTMesh"
#define MESH_PASSWORD "iotmeshpassword"
#define MESH_PORT     5555

const int ledPin = 12; // Pin D6

Scheduler userScheduler;
painlessMesh mesh;

// Variabel global untuk menyimpan data terakhir
float suhuTerakhir = 0.0;
int adcTerakhir = 1024; // Default anggap terang

void evaluasiRuleEngine() {
  // RULE ENGINE DIREVISI: 
  // Nyala jika Suhu > 31.0 ATAU Cahaya > 700 (Angka gelap)
  if (suhuTerakhir > 31.0 || adcTerakhir > 700) {
    digitalWrite(ledPin, HIGH); // Nyalakan LED
    Serial.println("-> STATUS: LED NYALA (ON) [Kondisi Gelap / Panas]");
  } else {
    digitalWrite(ledPin, LOW);  // Matikan LED
    Serial.println("-> STATUS: LED MATI (OFF) [Kondisi Normal / Terang]");
  }
}

void receivedCallback(uint32_t from, String &msg) {
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, msg);

  if (!error) {
    String tipe = doc["tipe"].as<String>();

    if (tipe == "suhu_node") {
      suhuTerakhir = doc["suhu"];
      Serial.printf("[TERIMA] Suhu: %.2f °C | ", suhuTerakhir);
    } 
    else if (tipe == "cahaya_node") {
      adcTerakhir = doc["adc"];
      Serial.printf("[TERIMA] Cahaya (ADC): %d | ", adcTerakhir);
    }
    
    // Panggil logika aktuator setiap ada data baru yang masuk
    evaluasiRuleEngine();
  }
}

void changedConnectionCallback() {
  Serial.println("--> Topologi rantai mesh telah berubah/diperbarui");
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW); // Default mati
  
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  
  mesh.onReceive(&receivedCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  
  // Note: taskSendMessage tidak diaktifkan karena node ini pasif penerima

  Serial.println("\nNode 3 (Hub Aktuator) Berjalan dan Menunggu Data...");
}

void loop() {
  mesh.update();
}