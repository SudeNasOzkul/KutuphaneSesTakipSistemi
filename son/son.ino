#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

// --- Wi-Fi Ayarları ---
const char* ssid = "Sudenas_WiFi";
const char* password = "1234567890";

// --- Pin Tanımlamaları ---
const int ledGreen = D1;
const int ledRed = D2;
const int soundSensorPin = A0;

ESP8266WebServer server(80);

// --- Global Değişkenler ---
int soundLevel = 0;
float noiseScore = 0; 
int warningTimer = 0; 
String status = "normal"; 

// --- HTML Arayüzü (Modern Tasarım ve Puan Çubuğu) ---
const char ANA_SAYFA[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="tr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Kütüphane Takip Sistemi</title>
    <style>
        body { background: #0f172a; color: white; font-family: 'Segoe UI', sans-serif; display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; overflow: hidden; }
        .card { background: #1e293b; padding: 40px; border-radius: 40px; text-align: center; transition: all 0.4s ease; border: 8px solid transparent; width: 380px; box-shadow: 0 20px 50px rgba(0,0,0,0.5); }
        .lamp { font-size: 80px; margin-bottom: 10px; }
        
        /* Yeşil Durum */
        .normal { color: #22c55e; border-color: #22c55e33; }
        
        /* Gürültü Durumu (Kırmızı) */
        .warning { 
            color: #ef4444 !important; 
            border-color: #ef4444 !important; 
            background: #2d1616 !important;
            animation: shake 0.2s infinite; 
            box-shadow: 0 0 60px rgba(239, 68, 68, 0.3); 
        }

        @keyframes shake { 
            0% { transform: translateX(3px) rotate(0deg); } 
            50% { transform: translateX(-3px) rotate(1deg); } 
            100% { transform: translateX(3px) rotate(0deg); } 
        }

        h1 { font-size: 2.8rem; margin: 10px 0; transition: 0.3s; }
        .info-container { display: flex; gap: 15px; margin-top: 25px; }
        .info-box { background: rgba(0,0,0,0.4); padding: 15px; border-radius: 20px; flex: 1; border: 1px solid rgba(255,255,255,0.1); }
        .val { font-size: 2.2rem; font-weight: bold; display: block; }
        .label { font-size: 0.7rem; opacity: 0.6; text-transform: uppercase; }
        
        .progress-bg { background: rgba(255,255,255,0.1); height: 12px; border-radius: 6px; margin-top: 20px; overflow: hidden; }
        #progress { background: #22c55e; height: 100%; width: 0%; transition: 0.3s; }
    </style>
</head>
<body>
    <div id="mainCard" class="card normal">
        <div id="lamp" class="lamp">✅</div>
        <h1 id="msg">SESSİZ</h1>
        <div class="progress-bg"><div id="progress"></div></div>
        <div class="info-container">
            <div class="info-box"><span class="label">Puan</span><span class="val" id="score">0</span></div>
            <div class="info-box"><span class="label">Süre</span><span class="val" id="seconds">0</span><span class="label">SN</span></div>
        </div>
    </div>
    <script>
        setInterval(() => {
            fetch('/data').then(r => r.json()).then(d => {
                const c = document.getElementById('mainCard');
                const m = document.getElementById('msg');
                const l = document.getElementById('lamp');
                const s = document.getElementById('seconds');
                const sc = document.getElementById('score');
                const p = document.getElementById('progress');
                
                // Verileri güncelle
                sc.innerText = Math.floor(d.nScore);
                s.innerText = Math.floor(d.wTime / 5);
                p.style.width = d.nScore + "%";

                // Durum Kontrolü
                if(d.status === 'warning') {
                    c.classList.add('warning');
                    c.classList.remove('normal');
                    m.innerText = 'GÜRÜLTÜ!';
                    l.innerText = '⚠️';
                    p.style.backgroundColor = '#ef4444';
                } else {
                    c.classList.add('normal');
                    c.classList.remove('warning');
                    m.innerText = 'SESSİZ';
                    l.innerText = '✅';
                    p.style.backgroundColor = '#22c55e';
                }
            }).catch(err => console.error("Veri alınamadı:", err));
        }, 200);
    </script>
</body>
</html>
)=====";
String getHTML() { return String(ANA_SAYFA); }

void setup() {
  Serial.begin(115200);
  
  // Pin ayarları
  pinMode(ledGreen, OUTPUT); 
  pinMode(ledRed, OUTPUT);
  
  // İlk açılışta yeşili yak, kırmızıyı söndür
  digitalWrite(ledGreen, HIGH);
  digitalWrite(ledRed, LOW);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nBağlandı! IP: " + WiFi.localIP().toString());

  server.on("/", []() { server.send(200, "text/html", getHTML()); });
  server.on("/data", []() {
    String json = "{\"status\":\"" + status + "\", \"wTime\":" + String(warningTimer) + ", \"nScore\":" + String(noiseScore) + "}";
    server.send(200, "application/json", json);
  });
  server.begin();
}

void loop() {
  server.handleClient();
  soundLevel = analogRead(soundSensorPin);
  
  int threshold = 550; //

  // --- Puanlama Mantığı ---
  if (soundLevel > threshold) {
    noiseScore += 8.0; //
    if (noiseScore > 100) noiseScore = 100; 
  } else {
    if (noiseScore > 0) noiseScore -= 4.0; //
    if (noiseScore < 0) noiseScore = 0;
  }

  // --- Karar ve LED Kontrolü ---
  if (noiseScore >= 25) { 
    // GÜRÜLTÜ VAR (Puan 25 ve Üstü)
    status = "warning";
    warningTimer++; 
    digitalWrite(ledGreen, LOW);   // Yeşil söner
    digitalWrite(ledRed, HIGH);    // Kırmızı yanar
  } 
  else {
    // SESSİZ DURUM (Puan 60'ın altı: 0-59 arası her zaman Yeşil yanar)
    status = "normal";
    warningTimer = 0;
    digitalWrite(ledGreen, HIGH);  // Yeşil her zaman yanar
    digitalWrite(ledRed, LOW);     // Kırmızı her zaman söner
  }

  Serial.print("Puan: "); Serial.println(noiseScore);
  delay(200); 
}