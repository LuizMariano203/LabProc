#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>


const char* AP_SSID = "ESP32-PWM";
const char* AP_PASS = "12345678";


#define PIN_LED   4
#define PIN_SERVO 5


int ledPct   = 50;
int servoAng = 90;


Servo servo;
WebServer server(80);


const char HTML[] PROGMEM = R"=====(
<!DOCTYPE html><html lang="pt-BR"><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32</title>
<style>
  body{font-family:sans-serif;max-width:340px;margin:40px auto;padding:0 16px;color:#111}
  h1{font-size:16px;font-weight:600;margin:0 0 24px}
  label{display:block;font-size:12px;color:#666;margin-bottom:6px}
  input[type=range]{width:100%;margin-bottom:4px}
  .val{font-size:22px;font-weight:700;margin-bottom:20px}
  .row{display:flex;gap:6px;margin-bottom:24px}
  button{flex:1;padding:8px;border:1px solid #ddd;border-radius:6px;
         background:#fff;font-size:12px;cursor:pointer}
  button:hover{background:#f5f5f5}
  hr{border:none;border-top:1px solid #eee;margin:0 0 20px}
</style></head><body>
<h1>ESP32 · PWM Dashboard</h1>
<label>Intensidade do LED</label>
<span class="val" id="lv">50%</span>
<input type="range" min="0" max="100" value="50"
  oninput="document.getElementById('lv').textContent=this.value+'%'"
  onchange="get('/led?v='+this.value)">
<div class="row">
  <button onclick="sl(0)">0%</button>
  <button onclick="sl(25)">25%</button>
  <button onclick="sl(50)">50%</button>
  <button onclick="sl(75)">75%</button>
  <button onclick="sl(100)">100%</button>
</div>
<hr>
<label>Posicao do Servo</label>
<span class="val" id="sv">90</span>
<input type="range" id="ss" min="0" max="180" value="90"
  oninput="document.getElementById('sv').textContent=this.value"
  onchange="get('/servo?v='+this.value)">
<div class="row">
  <button onclick="ss(0)">0</button>
  <button onclick="ss(45)">45</button>
  <button onclick="ss(90)">90</button>
  <button onclick="ss(135)">135</button>
  <button onclick="ss(180)">180</button>
</div>
<script>
function get(u){fetch(u)}
function sl(v){document.querySelector('input').value=v;document.getElementById('lv').textContent=v+'%';get('/led?v='+v)}
function ss(v){document.getElementById('ss').value=v;document.getElementById('sv').textContent=v;get('/servo?v='+v)}
</script>
</body></html>
)=====";


void setup() {
  Serial.begin(115200);


  // Servo primeiro – evita conflito de canal LEDC com o LED
  servo.attach(PIN_SERVO, 500, 2500);
  servo.write(servoAng);


  // LED via LEDC – inicializado APOS o servo
  ledcAttach(PIN_LED, 5000, 8);
  ledcWrite(PIN_LED, map(ledPct, 0, 100, 0, 255));


  // Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  Serial.printf("AP: %s  IP: %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());


  server.on("/", [](){
    server.send_P(200, "text/html", HTML);
  });
  server.on("/led", [](){
    int v = constrain(server.arg("v").toInt(), 0, 100);
    ledPct = v;
    ledcWrite(PIN_LED, map(v, 0, 100, 0, 255));
    server.send(200, "text/plain", "OK");
    Serial.printf("LED %d%%\n", v);
  });
  server.on("/servo", [](){
    int v = constrain(server.arg("v").toInt(), 0, 180);
    servoAng = v;
    servo.write(v);
    server.send(200, "text/plain", "OK");
    Serial.printf("SERVO %d graus\n", v);
  });
  server.begin();
}


void loop() {
  server.handleClient();
}






