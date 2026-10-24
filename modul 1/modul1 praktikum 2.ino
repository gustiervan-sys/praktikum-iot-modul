const int buttonPin = 4;
const int ledPin = 5;

// Variabel untuk Logika Pelacakan Status (State Tracking)
int buttonState = LOW;         // Menyimpan status pembacaan tombol saat ini
int lastButtonState = LOW;     // Menyimpan status tombol sebelumnya (untuk mendeteksi tekanan baru)
int ledState = LOW;            // Menyimpan status lampu saat ini (diawali dengan OFF/LOW)

void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);
  
  // Memastikan LED mati saat sistem pertama kali dinyalakan
  digitalWrite(ledPin, ledState);
}

void loop() {
  // Membaca status kelistrikan dari tombol (karena menggunakan pull-down, ditekan = HIGH)
  buttonState = digitalRead(buttonPin);

  // Logika Pelacakan State & Edge Detection
  // Kondisi ini hanya terpenuhi tepat pada momen tombol berubah dari tidak ditekan (LOW) 
  // menjadi ditekan (HIGH). Hal ini mencegah lampu berkedip terus saat tombol ditahan.
  if (buttonState == HIGH && lastButtonState == LOW) {
    
    // Membalikkan (Toggle) status memori LED
    if (ledState == LOW) {
      ledState = HIGH;
      Serial.println("Lampu Dinyalakan (ON)");
    } else {
      ledState = LOW;
      Serial.println("Lampu Dimatikan (OFF)");
    }
    
    // Menerapkan memori status yang baru ke komponen fisik LED
    digitalWrite(ledPin, ledState);
    
    // Logika Debounce
    // Memberikan jeda waktu 200ms untuk mengabaikan getaran mekanis (bouncing)
    // pada plat logam di dalam tombol fisik. Tanpa delay ini, satu tekanan 
    // bisa terbaca sebagai beberapa kali tekanan yang membuat lampu berkedip acak (flickering).
    delay(200); 
  }

  // Menyimpan status tombol saat ini untuk dijadikan pembanding pada putaran loop berikutnya
  lastButtonState = buttonState;
}