// ============================================================
//  DESAFIO - Semaforo inteligente com modo noturno
//  ESP32-C3 Super Mini
//  + Access Point WiFi
//  + Digital Twin via WebServer
//
//  Conecte em: SSID "Semaforo-ESP32"  senha "12345678"
//  Acesse:     http://192.168.4.1
// ============================================================

#include <WiFi.h>
#include <WebServer.h>

// ---- WiFi AP ----
const char* apSSID = "Semaforo-ESP32";
const char* apPASS = "12345678";
WebServer server(80);

// ---- Pinos ----
const int ldrPin    = 0;   // GPIO0 = ADC (LDR)
const int rgbLedPin = 8;   // GPIO8 = LED RGB embarcado
const int botaoPed  = 7;   // GPIO7 = botao pedestre (pull-down externo -> RISING)

// ---- Parametros ----
const int limiarNoturno = 1500;
const int brilho        = 40;
const int brilhoNoturno = 12;

const unsigned long tempoVerde      = 5000;
const unsigned long tempoAmarelo    = 2000;
const unsigned long tempoVermelho   = 5000;
const unsigned long tempoEmergencia = 3000;

// ---- Maquina de estados ----
enum Estado { VERDE, AMARELO, VERMELHO, NOTURNO };
Estado estado = VERDE;
unsigned long inicioEstado = 0;

// ---- Emergencia global ----
bool emEmergencia        = false;
Estado estadoAnterior    = VERDE;
unsigned long inicioEmerg = 0;
bool piscaLigadoAnterior = false;

// ---- Piscar noturno ----
int ldrVal = 0;
unsigned long prevLdr   = 0;
unsigned long prevBlink = 0;
bool piscaLigado = false;

// ---- ISR ----
volatile bool     pedidoTravessia = false;
volatile uint32_t botaoPressCount = 0;   // contador para o digital twin detectar pressionamentos
volatile unsigned long ultimoPed  = 0;
const unsigned long debounceMs    = 50;

// -------------------------------------------------------

void setCor(int r, int g, int b) { neopixelWrite(rgbLedPin, r, g, b); }

void IRAM_ATTR isrPedestre() {
  unsigned long agora = millis();
  if (agora - ultimoPed > debounceMs) {
    ultimoPed = agora;
    pedidoTravessia = true;
    botaoPressCount++;
  }
}

void aplicaCorEstado(Estado est) {
  switch (est) {
    case VERDE:    setCor(0, brilho, 0);       break;
    case AMARELO:  setCor(brilho, brilho, 0);  break;
    case VERMELHO: setCor(brilho, 0, 0);       break;
    case NOTURNO:
      if (piscaLigado) setCor(brilhoNoturno, brilhoNoturno, 0);
      else             setCor(0, 0, 0);
      break;
  }
}

void trocaEstado(Estado novo) {
  estado = novo;
  inicioEstado = millis();
  switch (novo) {
    case VERDE:    setCor(0, brilho, 0);       break;
    case AMARELO:  setCor(brilho, brilho, 0);  break;
    case VERMELHO: setCor(brilho, 0, 0);       break;
    case NOTURNO:
      piscaLigado = false;
      prevBlink = millis();
      setCor(0, 0, 0);
      break;
  }
}

// -------------------------------------------------------
//  Pagina HTML do Digital Twin (servida em PROGMEM)
// -------------------------------------------------------

const char htmlPage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Semáforo — Digital Twin</title>
<style>
  *{box-sizing:border-box;margin:0;padding:0}
  body{background:#0d1117;color:#e6edf3;font-family:system-ui,sans-serif;
       display:flex;flex-direction:column;align-items:center;
       min-height:100vh;padding:24px 16px;gap:20px}
  h1{font-size:15px;letter-spacing:3px;text-transform:uppercase;
     color:#8b949e;font-weight:500}

  /* ── layout ── */
  .grid{display:grid;grid-template-columns:auto 1fr;gap:16px;
        width:100%;max-width:460px;align-items:start}

  /* ── semaforo ── */
  .housing{background:#161b22;border:2px solid #30363d;border-radius:18px;
           padding:18px 22px;display:flex;flex-direction:column;
           align-items:center;gap:14px;width:110px}
  .bulb{width:62px;height:62px;border-radius:50%;
        background:#1c2128;border:2px solid #30363d;transition:all .25s}
  .on-red   {background:#f85149;box-shadow:0 0 22px #f85149,0 0 44px #f8514966}
  .on-yellow{background:#e3b341;box-shadow:0 0 22px #e3b341,0 0 44px #e3b34166}
  .on-green {background:#3fb950;box-shadow:0 0 22px #3fb950,0 0 44px #3fb95066}

  /* ── painel ── */
  .panel{display:flex;flex-direction:column;gap:12px}
  .card{background:#161b22;border:1px solid #30363d;border-radius:12px;padding:14px}
  .label{font-size:10px;text-transform:uppercase;letter-spacing:1px;
         color:#8b949e;margin-bottom:6px}

  /* estado */
  #estado-txt{font-size:22px;font-weight:700;transition:color .3s}
  .c-verde    {color:#3fb950}
  .c-amarelo  {color:#e3b341}
  .c-vermelho {color:#f85149}
  .c-noturno  {color:#58a6ff}
  .c-emergencia{color:#f85149;animation:blink .5s infinite}
  @keyframes blink{0%,100%{opacity:1}50%{opacity:.25}}

  /* LDR */
  .gauge{background:#0d1117;border-radius:4px;height:8px;margin-top:8px;overflow:hidden}
  .gauge-fill{height:100%;border-radius:4px;
              background:linear-gradient(90deg,#3fb950,#e3b341,#f85149);
              transition:width .5s;width:0%}
  #ldr-num{color:#8b949e;font-size:11px;float:right}

  /* botao */
  .btn-row{display:flex;align-items:center;gap:10px;margin-top:4px}
  .dot{width:12px;height:12px;border-radius:50%;
       background:#21262d;border:1px solid #30363d;transition:all .2s;flex-shrink:0}
  .dot.active{background:#e3b341;box-shadow:0 0 8px #e3b341;border-color:#e3b341}
  #btn-lbl{font-size:13px;color:#8b949e}
  #press-count{font-size:11px;color:#8b949e;margin-left:auto}

  /* uptime */
  .footer{font-size:11px;color:#484f58}

  /* modo noturno chip */
  #modo-chip{display:inline-block;font-size:10px;padding:2px 8px;border-radius:20px;
             font-weight:600;margin-top:4px;transition:all .3s}
  .chip-dia{background:#1f3828;color:#3fb950;border:1px solid #3fb95044}
  .chip-noite{background:#1b2838;color:#58a6ff;border:1px solid #58a6ff44}
</style>
</head>
<body>

<h1>🚦 Digital Twin — ESP32-C3</h1>

<div class="grid">

  <!-- semaforo visual -->
  <div class="housing">
    <div class="bulb" id="b-red"></div>
    <div class="bulb" id="b-yel"></div>
    <div class="bulb" id="b-grn"></div>
  </div>

  <!-- painel de info -->
  <div class="panel">

    <div class="card">
      <div class="label">Estado atual</div>
      <div id="estado-txt">—</div>
      <div id="modo-chip" class="chip-dia">DIA</div>
    </div>

    <div class="card">
      <div class="label">Sensor LDR <span id="ldr-num">—</span></div>
      <div class="gauge"><div class="gauge-fill" id="ldr-bar"></div></div>
    </div>

    <div class="card">
      <div class="label">Botão Pedestre</div>
      <div class="btn-row">
        <div class="dot" id="btn-dot"></div>
        <span id="btn-lbl">aguardando</span>
        <span id="press-count">0 acionamentos</span>
      </div>
    </div>

  </div>
</div>

<div class="footer">uptime: <span id="uptime">—</span></div>

<script>
let lastCount = -1;
let btnTimer;

async function poll() {
  try {
    const d = await fetch('/api/status').then(r => r.json());

    // ── bulbs ──
    const br = document.getElementById('b-red');
    const by = document.getElementById('b-yel');
    const bg = document.getElementById('b-grn');
    br.className = by.className = bg.className = 'bulb';

    if (d.emEmergencia) {
      br.className = 'bulb on-red';
    } else {
      if (d.estado === 'VERDE')    bg.className = 'bulb on-green';
      if (d.estado === 'AMARELO')  by.className = 'bulb on-yellow';
      if (d.estado === 'VERMELHO') br.className = 'bulb on-red';
      if (d.estado === 'NOTURNO' && d.piscaLigado) by.className = 'bulb on-yellow';
    }

    // ── estado texto ──
    const label = d.emEmergencia ? 'EMERGÊNCIA' : d.estado;
    const el = document.getElementById('estado-txt');
    el.textContent = label;
    el.className = 'c-' + label.toLowerCase();

    // ── chip dia/noite ──
    const chip = document.getElementById('modo-chip');
    if (d.estado === 'NOTURNO') {
      chip.textContent = '🌙 NOITE';
      chip.className = 'chip-noite';
    } else {
      chip.textContent = '☀️ DIA';
      chip.className = 'chip-dia';
    }

    // ── LDR ──
    document.getElementById('ldr-num').textContent = d.ldrVal;
    document.getElementById('ldr-bar').style.width =
      Math.min(100, Math.round(d.ldrVal / 4095 * 100)) + '%';

    // ── botão ──
    if (lastCount !== -1 && d.botaoPressCount !== lastCount) {
      const dot = document.getElementById('btn-dot');
      const lbl = document.getElementById('btn-lbl');
      dot.classList.add('active');
      lbl.textContent = 'pressionado!';
      lbl.style.color = '#e3b341';
      clearTimeout(btnTimer);
      btnTimer = setTimeout(() => {
        dot.classList.remove('active');
        lbl.textContent = 'aguardando';
        lbl.style.color = '#8b949e';
      }, 800);
    }
    lastCount = d.botaoPressCount;
    document.getElementById('press-count').textContent =
      d.botaoPressCount + ' acionamento' + (d.botaoPressCount === 1 ? '' : 's');

    // ── uptime ──
    const s = Math.floor(d.uptime / 1000);
    const h = Math.floor(s / 3600);
    const m = Math.floor((s % 3600) / 60);
    const sc = s % 60;
    document.getElementById('uptime').textContent =
      (h ? h + 'h ' : '') + (m ? m + 'm ' : '') + sc + 's';

  } catch(e) { /* reconecta no proximo ciclo */ }
  setTimeout(poll, 400);
}

poll();
</script>
</body>
</html>
)=====";

// -------------------------------------------------------
//  Handlers do WebServer
// -------------------------------------------------------

void handleRoot() {
  server.send_P(200, "text/html", htmlPage);
}

void handleStatus() {
  const char* nomes[] = { "VERDE", "AMARELO", "VERMELHO", "NOTURNO" };

  String json = "{";
  json += "\"estado\":\"";       json += nomes[estado];         json += "\",";
  json += "\"emEmergencia\":";   json += emEmergencia ? "true" : "false"; json += ",";
  json += "\"ldrVal\":";         json += ldrVal;                json += ",";
  json += "\"piscaLigado\":";    json += piscaLigado ? "true" : "false";  json += ",";
  json += "\"botaoPressCount\":"; json += botaoPressCount;      json += ",";
  json += "\"uptime\":";         json += millis();
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

// -------------------------------------------------------

void setup() {
  Serial.begin(115200);
  analogSetAttenuation(ADC_11db);

  pinMode(botaoPed, INPUT);
  attachInterrupt(digitalPinToInterrupt(botaoPed), isrPedestre, RISING);

  // ---- Access Point ----
  WiFi.softAP(apSSID, apPASS);
  Serial.print("AP iniciado. IP: ");
  Serial.println(WiFi.softAPIP());   // 192.168.4.1

  // ---- Rotas ----
  server.on("/",           handleRoot);
  server.on("/api/status", handleStatus);
  server.begin();
  Serial.println("WebServer iniciado.");

  trocaEstado(VERDE);
}

void loop() {
  unsigned long agora = millis();

  // Servidor web sempre atendido (antes de qualquer return)
  server.handleClient();

  // ---- Leitura do LDR a cada 1 segundo ----
  if (agora - prevLdr >= 1000) {
    prevLdr = agora;
    ldrVal = analogRead(ldrPin);
  }

  // ---- Botao: salva estado atual e entra em emergencia ----
  if (pedidoTravessia && !emEmergencia) {
    pedidoTravessia = false;

    estadoAnterior       = estado;
    piscaLigadoAnterior  = piscaLigado;

    emEmergencia = true;
    inicioEmerg  = agora;
    setCor(brilho, 0, 0);
    Serial.println("EMERGENCIA: vermelho por 3s.");
  }

  // ---- Gerencia a emergencia ----
  if (emEmergencia) {
    if (agora - inicioEmerg >= tempoEmergencia) {
      emEmergencia = false;

      estado       = estadoAnterior;
      inicioEstado = agora;          // reinicia timer do zero
      piscaLigado  = piscaLigadoAnterior;

      if (estado == NOTURNO) prevBlink = agora;

      aplicaCorEstado(estado);
      Serial.println("Emergencia encerrada: estado anterior restaurado.");
    }
    return;
  }

  // ---- Transicao entre modo diurno e noturno ----
  if (ldrVal > limiarNoturno && estado != NOTURNO) {
    Serial.println("Modo noturno: amarelo piscando a 0.5 Hz.");
    trocaEstado(NOTURNO);
  } else if (ldrVal <= limiarNoturno && estado == NOTURNO) {
    Serial.println("Amanheceu: retomando ciclo normal.");
    trocaEstado(VERMELHO);
  }

  // ---- Maquina de estados ----
  switch (estado) {

    case NOTURNO:
      if (agora - prevBlink >= 1000) {
        prevBlink = agora;
        piscaLigado = !piscaLigado;
        if (piscaLigado) setCor(brilhoNoturno, brilhoNoturno, 0);
        else             setCor(0, 0, 0);
      }
      break;

    case VERDE:
      if (agora - inicioEstado >= tempoVerde)   trocaEstado(AMARELO);
      break;

    case AMARELO:
      if (agora - inicioEstado >= tempoAmarelo) trocaEstado(VERMELHO);
      break;

    case VERMELHO:
      if (agora - inicioEstado >= tempoVermelho) trocaEstado(VERDE);
      break;
  }
}
