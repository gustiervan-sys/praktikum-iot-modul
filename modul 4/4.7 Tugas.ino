#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>

const char* ssid = "E";
const char* password = "88888888";

// Konfigurasi Pin Praktikum 1 & Tugas 4.7
const byte dhtPin = 2;        // D4 (GPIO 2)
const byte buttonPin = 4;     // D2 (GPIO 4)
const byte ledPin = 12;       // D6 (GPIO 12) - LED On/Off Utama
const byte pwmLedPin = 5;     // D1 (GPIO 5) - LED Dimmer (Tugas 4.7)

DHT dht(dhtPin, DHT22);

// Variabel Pelacak Status (State & Cache)
bool ledState = false;
String currentTemp = "--";
int buttonState;
int lastButtonState = LOW;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;
unsigned long lastTime = 0;
String currentPwm = "0"; // Menyimpan nilai PWM terakhir untuk sinkronisasi web

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// ---------------- HTML & JAVASCRIPT (FRONT-END) ----------------
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Real-Time IoT Web</title>
  <style>
    body { font-family: Arial; text-align: center; background-color: #f9f9f9; }
    .card { background: white; margin: 20px auto; padding: 20px; max-width: 320px; border-radius: 10px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }
    button { padding: 15px 30px; font-size: 20px; border-radius: 5px; cursor: pointer; color: white; border: none; }  
    .btn-on { background-color: #4CAF50; }  
    .btn-off { background-color: #f44336; }
    input[type=range] { width: 100%; margin-top: 15px; }
  </style>
</head>
<body>
  <h1>Smart Room Dashboard</h1>
  
  <div class="card">
    <h2>Suhu: <span id="tempValue">--</span> &deg;C</h2>
  </div>
  
  <div class="card">
    <h2>LED Utama: <span id="ledStatus">OFF</span></h2>
    <button id="toggleBtn" class="btn-off" onclick="toggleLed()">Turn ON</button>
  </div>

  <!-- KARTU BARU UNTUK TUGAS 4.7 -->
  <div class="card">
    <h2>Dimmer LED (PWM)</h2>
    <p>Intensitas: <span id="pwmValue">0</span></p>
    <!-- Menggunakan event oninput agar transmisi data dikirim persis saat jari menggeser layar -->
    <input type="range" min="0" max="1023" value="0" id="pwmSlider" oninput="sendPWM(this.value)">
  </div>

  <script>
    var gateway = `ws://${window.location.hostname}/ws`;
    var websocket;

    window.addEventListener('load', onLoad);
    function onLoad(event) { initWebSocket(); }

    function initWebSocket() {
      websocket = new WebSocket(gateway);
      websocket.onopen    = onOpen;
      websocket.onclose   = onClose;
      websocket.onmessage = onMessage;
    }

    function onOpen(event) { console.log('WebSocket Terkoneksi'); }
    function onClose(event) { setTimeout(initWebSocket, 2000); }

    function toggleLed(){  
      websocket.send('toggle');
    }

    // FUNGSI JAVASCRIPT BARU (TUGAS 4.7)
    function sendPWM(value) {
      document.getElementById('pwmValue').innerHTML = value; // Perbarui angka teks di layar
      websocket.send('pwm,' + value); // Kirimkan format paket "pwm,angka" ke NodeMCU
    }

    function onMessage(event) {
      var dataObj = JSON.parse(event.data);  
        
      if(dataObj.suhu !== undefined) {  
         document.getElementById('tempValue').innerHTML = dataObj.suhu;  
      }  
        
      if(dataObj.led !== undefined) {  
         var btn = document.getElementById('toggleBtn');  
         var status = document.getElementById('ledStatus');  
         if(dataObj.led == "1"){  
           status.innerHTML = "ON";  
           btn.innerHTML = "Turn OFF";  
           btn.className = "btn-on";  
         } else {  
           status.innerHTML = "OFF";  
           btn.innerHTML = "Turn ON";  
           btn.className = "btn-off";  
         }  
      }
      
      // Sinkronisasi posisi slider jika halaman di-refresh / dibuka dari HP lain
      if(dataObj.pwm !== undefined) {
         document.getElementById('pwmSlider').value = dataObj.pwm;
         document.getElementById('pwmValue').innerHTML = dataObj.pwm;
      }
    }  
  </script>  
</body>  
</html>  
)rawliteral";

// ---------------- BACK-END & WEBSOCKET LOGIC ----------------

void notifyClients() {
  String jsonString = "{\"led\":\"" + String(ledState ? 1 : 0) + "\", ";
  jsonString += "\"suhu\":\"" + currentTemp + "\", ";
  jsonString += "\"pwm\":\"" + currentPwm + "\"}"; // Menyisipkan state PWM
  ws.textAll(jsonString);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    String message = (char*)data;
    
    // Logika 1: Kendali LED Utama (Praktikum 1)
    if (message == "toggle") {
      ledState = !ledState;
      notifyClients();
    } 
    
    // Logika 2: Kendali Dimmer Slider (Tugas 4.7)
    else if (message.startsWith("pwm,")) {
      // Potong string mulai dari indeks ke-4 (setelah kata "pwm,")
      String pwmValueStr = message.substring(4); 
      
      // Ubah teks angka menjadi integer murni
      int pwmValue = pwmValueStr.toInt();
      
      // Simpan state & eksekusi fitur analogWrite (PWM) untuk meredup/terangkan LED
      currentPwm = pwmValueStr;
      analogWrite(pwmLedPin, pwmValue); 
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      notifyClients(); // Kirim semua data (termasuk posisi slider) ke klien baru
      break;
    case WS_EVT_DISCONNECT:
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(buttonPin, INPUT);
  
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  
  // INISIALISASI PIN PWM (Tugas 4.7)
  pinMode(pwmLedPin, OUTPUT);
  analogWrite(pwmLedPin, 0); // Pastikan LED mati saat NodeMCU pertama menyala

  dht.begin();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  ws.onEvent(onEvent);
  server.addHandler(&ws);
  server.begin();
}

void loop() {
  ws.cleanupClients();

  // Eksekusi perangkat keras LED Utama
  digitalWrite(ledPin, ledState ? HIGH : LOW);

  // Baca Tombol Fisik (Debounce)
  int reading = digitalRead(buttonPin);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == HIGH) {
        ledState = !ledState;
        notifyClients(); 
      }
    }
  }
  lastButtonState = reading;

  // Baca DHT 
  if ((millis() - lastTime) > 3000) {
    float t = dht.readTemperature();
    if(!isnan(t)) {
      currentTemp = String(t);
      notifyClients(); 
    }
    lastTime = millis();
  }
}