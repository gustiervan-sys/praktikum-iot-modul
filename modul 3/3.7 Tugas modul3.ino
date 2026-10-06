#include <ESP8266WiFi.h>  
#include <ESP8266WebServer.h>  
#include <DHT.h>

const char* ssid = "E";  
const char* password = "22222222";

ESP8266WebServer server(80);

const byte dhtPin = 2;         // Pin D2 untuk DHT
const byte relayPin = 12;      // Pin D6 (GPIO 12) untuk Relay/LED
DHT dht(dhtPin, DHT11);

const char index_html[] PROGMEM = R"rawliteral(  
<!DOCTYPE html>  
<html>  
<head>  
  <meta name="viewport" content="width=device-width, initial-scale=1">  
  <!-- Modifikasi Tugas 3.7: Memaksa browser reload otomatis setiap 5 detik -->
  <meta http-equiv="refresh" content="5">  
  <title>IoT Dashboard Real-Time</title>  
  <style>  
    body { font-family: Arial; text-align: center; margin-top: 50px; }  
    button { padding: 15px 30px; font-size: 20px; border-radius: 8px; margin: 10px; cursor: pointer;}  
    .btn-on { background-color: #4CAF50; color: white; border: none; }  
    .btn-off { background-color: #f44336; color: white; border: none; }  
    .sensor-box { font-size: 24px; font-weight: bold; }  
  </style>  
</head>  
<body>  
  <h1>ESP8266 Web Server (Auto-Refresh)</h1>  
  <div class="sensor-box">  
    <p>Suhu Saat Ini: <strong>%TEMPERATURE%</strong> Celcius</p>  
    <p>Kelembapan: <strong>%HUMIDITY%</strong> %</p>  
  </div>  
  <h2>Kendali Relay</h2>  
  %RELAY_BUTTON%  
</body>  
</html>  
)rawliteral";

void handleRoot() {  
  String html = index_html; 
  float t = dht.readTemperature();  
  float h = dht.readHumidity();  
  
  // Menangani placeholder Suhu
  if (isnan(t)) {
    html.replace("%TEMPERATURE%", "--"); 
  } else {
    html.replace("%TEMPERATURE%", String(t)); 
  }

  // Menangani placeholder Kelembapan
  if (isnan(h)) {
    html.replace("%HUMIDITY%", "--"); 
  } else {
    html.replace("%HUMIDITY%", String(h)); 
  }

  // Menangani tombol toggle dinamis berdasarkan status Relay
  int relayState = digitalRead(relayPin);
  if (relayState == LOW) {
    html.replace("%RELAY_BUTTON%", "<a href=\"/relay/on\"><button class=\"btn-on\">NYALAKAN (ON)</button></a>");
  } else {
    html.replace("%RELAY_BUTTON%", "<a href=\"/relay/off\"><button class=\"btn-off\">MATIKAN (OFF)</button></a>");
  }
  
  server.send(200, "text/html", html);  
}

void handleRelayOn() {  
  digitalWrite(relayPin, HIGH); 
  server.sendHeader("Location", "/");   
  server.send(303);  
}

void handleRelayOff() {  
  digitalWrite(relayPin, LOW);   
  server.sendHeader("Location", "/");   
  server.send(303);  
}

void setup() {  
  Serial.begin(115200);  
  pinMode(relayPin, OUTPUT);  
  digitalWrite(relayPin, LOW);  
  dht.begin();  
    
  WiFi.mode(WIFI_STA);   
  WiFi.begin(ssid, password);  
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }  
  
  Serial.println("\nIP Address Server Anda: ");  
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);  
  server.on("/relay/on", handleRelayOn);  
  server.on("/relay/off", handleRelayOff);  
  server.begin();  
}

void loop() {  
  server.handleClient();  
}