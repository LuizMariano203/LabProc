// ============================================================
//  DESAFIO - Semaforo inteligente com modo noturno
//  ESP32-C3 Super Mini
//  - Ciclo normal: VERDE -> AMARELO -> VERMELHO
//  - Modo noturno (LDR): amarelo piscando a 0.5 Hz, brilho baixo
//  - Botao de pedestre: salva estado atual, vai DIRETO para
//    vermelho por 3s, depois restaura o estado anterior
// ============================================================

// ---- Pinos ----
const int ldrPin    = 0;
const int rgbLedPin = 8;
const int botaoPed  = 7;

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

// ---- Emergencia global (qualquer estado) ----
bool emEmergencia            = false;
Estado estadoAnterior        = VERDE;
unsigned long inicioEstadoAnterior = 0;
unsigned long inicioEmerg    = 0;
bool piscaLigadoAnterior     = false;  // salva contexto do NOTURNO

// ---- Piscar noturno ----
int ldrVal = 0;
unsigned long prevLdr   = 0;
unsigned long prevBlink = 0;
bool piscaLigado = false;

// ---- ISR ----
volatile bool pedidoTravessia = false;
volatile unsigned long ultimoPed = 0;
const unsigned long debounceMs = 50;

// -------------------------------------------------------

void setCor(int r, int g, int b) { neopixelWrite(rgbLedPin, r, g, b); }

void IRAM_ATTR isrPedestre() {
  unsigned long agora = millis();
  if (agora - ultimoPed > debounceMs) {
    ultimoPed = agora;
    pedidoTravessia = true;
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

void setup() {
  Serial.begin(115200);
  analogSetAttenuation(ADC_11db);
  pinMode(botaoPed, INPUT);
  attachInterrupt(digitalPinToInterrupt(botaoPed), isrPedestre, RISING);
  trocaEstado(VERDE);
}

void loop() {
  unsigned long agora = millis();

  // ---- Leitura do LDR a cada 1 segundo ----
  if (agora - prevLdr >= 1000) {
    prevLdr = agora;
    ldrVal = analogRead(ldrPin);
  }

  // ---- Botao: salva estado atual e entra em emergencia ----
  if (pedidoTravessia && !emEmergencia) {
    pedidoTravessia = false;

    // Salva tudo do estado atual
    estadoAnterior       = estado;
    inicioEstadoAnterior = inicioEstado;
    piscaLigadoAnterior  = piscaLigado;

    // Entra no vermelho de emergencia
    emEmergencia = true;
    inicioEmerg  = agora;
    setCor(brilho, 0, 0);
    Serial.println("EMERGENCIA: vermelho por 3s.");
  }

  // ---- Gerencia a emergencia ----
  if (emEmergencia) {
    if (agora - inicioEmerg >= tempoEmergencia) {
      emEmergencia = false;

      // Restaura o estado anterior exatamente onde parou
      estado       = estadoAnterior;
      inicioEstado = inicioEstadoAnterior;
      piscaLigado  = piscaLigadoAnterior;

      // Para NOTURNO, reinicia o timer do piscar a partir de agora
      if (estado == NOTURNO) prevBlink = agora;

      aplicaCorEstado(estado);
      Serial.println("Emergencia encerrada: estado anterior restaurado.");
    }
    return;  // congela o restante do loop durante a emergencia
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
      // Pisca amarelo a 0.5 Hz (1000 ms por metade) com brilho baixo
      if (agora - prevBlink >= 1000) {
        prevBlink = agora;
        piscaLigado = !piscaLigado;
        if (piscaLigado) setCor(brilhoNoturno, brilhoNoturno, 0);
        else             setCor(0, 0, 0);
      }
      break;

    case VERDE:
      if (agora - inicioEstado >= tempoVerde) {
        trocaEstado(AMARELO);
      }
      break;

    case AMARELO:
      if (agora - inicioEstado >= tempoAmarelo) {
        trocaEstado(VERMELHO);
      }
      break;

    case VERMELHO:
      if (agora - inicioEstado >= tempoVermelho) {
        trocaEstado(VERDE);
      }
      break;
  }
}
