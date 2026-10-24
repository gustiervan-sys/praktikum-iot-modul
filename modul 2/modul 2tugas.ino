#include <DHT.h>

// Definisi Pin sesuai Instruksi
const int ldrPin = A0;
const int dhtPin = 13;    // Pin D7 pada NodeMCU
const int relayPin = 12;  // Pin D6 pada NodeMCU
const int ledPin = 5;     // Pin D1 pada NodeMCU

// Konfigurasi Sensor DHT
#define DHTTYPE DHT11     // Ubah menjadi DHT22 jika Anda menggunakan sensor berwarna putih
DHT dht(dhtPin, DHTTYPE);

void setup() {
  Serial.begin(115200);
  dht.begin();

  // Konfigurasi Aktuator sebagai OUTPUT
  pinMode(relayPin, OUTPUT);
  pinMode(ledPin, OUTPUT);

  // Status Awal: Pastikan aktuator mati demi keamanan
  // Catatan Penting: Jika modul Relay Anda berjenis "Active-Low", 
  // ubah LOW menjadi HIGH di bawah ini agar relay benar-benar mati di awal.
  digitalWrite(relayPin, LOW); 
  digitalWrite(ledPin, LOW);

  Serial.println("Sistem Otomasi Kontrol Lokal Dimulai...");
}

void loop() {
  // Sensor DHT membutuhkan jeda waktu antar pembacaan (idealnya 2 detik)
  delay(2000);

  // 1. Akuisisi Data Multi-Sensor
  int ldrValue = analogRead(ldrPin);
  float t = dht.readTemperature();

  // 2. Proteksi Kegagalan Pembacaan (Menghindari error NaN)
  if (isnan(t)) {
    Serial.println("Error: Gagal membaca data dari sensor DHT!");
    return; // Keluar dari loop saat ini dan coba lagi dari awal
  }

  // Cetak status pembacaan mentah
  Serial.print("Suhu Udara: ");
  Serial.print(t);
  Serial.print(" °C | Cahaya (ADC): ");
  Serial.println(ldrValue);

  // 3. Rule Engine Logika OR (Edge Computing Dasar)
  // Kondisi buruk: Suhu di atas 34 ATAU Cahaya di bawah 300
  if (t > 34 || ldrValue < 300) {
    // Aksi saat kondisi darurat
    digitalWrite(relayPin, HIGH); // Nyalakan kipas/pendingin (Relay)
    digitalWrite(ledPin, HIGH);   // Nyalakan lampu indikator
    Serial.println("-> Peringatan: Aktuator Aktif! (Lingkungan memburuk)");
  } else {
    // Aksi saat kondisi normal
    digitalWrite(relayPin, LOW);  // Matikan Relay
    digitalWrite(ledPin, LOW);    // Matikan lampu indikator
    Serial.println("-> Kondisi Aman.");
  }
  
  Serial.println("-------------------------------------");
}