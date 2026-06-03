// =========================================
// CALCULADORA 4 BITS - ESP32-C3
// Complemento de 2 com overflow
// Operacoes: + - * / fatorial
// + Benchmark multiplicacao: 2, 4, 8 bits
// =========================================

#include <WiFi.h>

constexpr char SSID_RED[]  = "TEXTOALTA";
constexpr char PASS_RED[]  = "12345678";

WiFiServer server(80);

constexpr int LEDS_PINOS[4] = {4, 5, 6, 7};
constexpr unsigned long N_REPETICOES = 10000UL;

struct CasoTeste { 
  int a; 
  int b; 
};

// 2 bits com sinal: faixa -2 a 1 (Apenas 4 combinações possíveis no total do universo de 2 bits)
// Repetidos e combinados para preencher os requisitos de estresse do benchmark
const CasoTeste casos2Bits[30] = {
  {-2,-2}, {-2,-1}, {-2,0}, {-2,1},  // Limites e nulos
  {-1,-2}, {-1,-1}, {-1,0}, {-1,1},  // Linha do -1
  {0,-2},  {0,-1},  {0,0},  {0,1},   // Linha do 0
  {1,-2},  {1,-1},  {1,0},  {1,1},   // Linha do 1
  {-2,-2}, {-2,1},  {1,1},  {-1,-1}, // Repetições de estresse
  {-2,0},  {0,1},   {1,-2}, {-1,1},
  {-2,-1}, {1,0},   {0,0},  {-2,1},
  {-1,-2}, {1,1}
};

// 4 bits com sinal: faixa -8 a 7 (30 casos estratégicos)
const CasoTeste casos4Bits[30] = {
  {-8,-8}, {-8,7},  {7,7},  {-4,-4}, // Limites e quadrados extremos
  {5,3},   {-3,6},  {2,-5}, {1,7},   // Operações mistas
  {-6,-2}, {4,4},   {0,0},  {0,7},   // Elementos neutros e nulos
  {-8,0},  {-8,1},  {7,1},  {7,-1},  // Multiplicações por 1 e -1
  {-1,-1}, {-5,-5}, {3,3},  {-2,-2}, // Quadrados diversos
  {-8,-1}, {-7,2},  {6,-3}, {-5,4},  // Sinais invertidos alternados
  {4,-4},  {-2,7},  {3,-8}, {5,5},   // Testes de overflow variados
  {-7,-2}, {2,3}
};

// 8 bits com sinal: faixa -128 a 127 (30 casos estratégicos)
const CasoTeste casos8Bits[30] = {
  {-128,-128}, {-128,127}, {127,127}, {-64,64}, // Limites absolutos
  {100,100},   {-99,99},   {33,-33},   {15,15},   // Valores altos
  {-50,50},    {-1,-1},    {0,0},      {127,0},   // Identidades e nulos
  {-128,0},    {-128,1},   {127,1},    {127,-1},  // Testes de borda por unidade
  {-10,10},    {-20,-20},  {50,2},     {-25,4},   // Multiplicações exatas comuns
  {12,12},     {-15,8},    {64,2},     {-128,-1}, // Inversões de sinal no limite
  {10,100},    {-5,50},    {2,120},    {-3,90},   // Escalares preditivos
  {-12,12},    {9,9}
};

// =========================================
// HTML
// =========================================

const String paginaWeb = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<title>Calculadora 4 bits</title>
<style>
body{font-family:Arial;text-align:center;background:#101820;color:white;margin-top:40px;padding-bottom:40px;}
input,select,button{padding:10px;margin:5px;font-size:18px;}
.card{background:#1d2b36;width:420px;margin:auto;padding:20px;border-radius:12px;margin-bottom:20px;}
.aviso{color:#ff8080;font-weight:bold;}
</style>
</head>
<body>
<div class="card">
<h1>Calculadora 4 bits</h1>
<label>A</label><br>
<input type="number" id="a" min="-8" max="7" value="0"><br>
<label>B</label><br>
<input type="number" id="b" min="-8" max="7" value="0"><br>
<select id="op">
  <option value="add">A + B</option>
  <option value="sub">A - B</option>
  <option value="mul">A &times; B</option>
  <option value="div">A &divide; B</option>
  <option value="fat">A!</option>
</select>
<br>
<button onclick="calc()">Calcular</button>
<div id="resultado" style="margin-top:20px;"></div>
</div>
<script>
function bin4(n){return(n&0x0F).toString(2).padStart(4,'0');}
function calc(){
  let a=parseInt(document.getElementById('a').value);
  let b=parseInt(document.getElementById('b').value);
  let op=document.getElementById('op').value;
  let box=document.getElementById('resultado');
  if(isNaN(a)||isNaN(b)||a<-8||a>7||b<-8||b>7){box.innerHTML='<span class="aviso">Use valores entre -8 e 7</span>';return;}
  fetch(`/calc?a=${a}&b=${b}&op=${op}`).then(r=>r.json()).then(d=>{
    if(d.erro){box.innerHTML='<span class="aviso">Operacao invalida.</span>';return;}
    let s='+';
    if(op==='sub')s='-';if(op==='mul')s='&times;';if(op==='div')s='&divide;';if(op==='fat')s='!';
    let conta=op==='fat'?a+'! = '+d.completo:a+' '+s+' '+b+' = '+d.completo;
    box.innerHTML='Resultado: '+d.resultado+'<br>Bits: '+bin4(d.bits)+'<br>'+conta+'<br>Overflow: '+(d.overflow?'SIM':'NAO');
  });
}
</script>
</body>
</html>
)rawliteral";

// =========================================
// Funcoes auxiliares — calculadora
// =========================================

void atualizarPainelLeds(int valor) {
  valor &= 0x0F;
  for (int i = 0; i < 4; i++) {
    digitalWrite(LEDS_PINOS[i], (valor >> i) & 1);
  }
}

int extrairSinal4Bits(int v) {
  v &= 0x0F;
  return (v & 0x08) ? (v - 16) : v;
}

int calcularFatorial(int n) {
  if (n < 0) return 0;
  int resultado = 1;
  for (int i = 2; i <= n; i++) {
    resultado *= i;
  }
  return resultado;
}

String filtrarParametroReq(const String &requisicao, const String &chave) {
  int indiceInicio = requisicao.indexOf(chave + "=");
  if (indiceInicio < 0) return "";
  
  indiceInicio += chave.length() + 1;
  int indiceFim = requisicao.indexOf('&', indiceInicio);
  if (indiceFim < 0) {
    indiceFim = requisicao.indexOf(' ', indiceInicio);
  }
  return requisicao.substring(indiceInicio, indiceFim);
}

// =========================================
// CALCULO CALCULADORA
// =========================================

void processarRequisicaoCalc(WiFiClient &cliente, const String &requisicao) {
  int aBruto = filtrarParametroReq(requisicao, "a").toInt() & 0x0F;
  int bBruto = filtrarParametroReq(requisicao, "b").toInt() & 0x0F;
  String operacao = filtrarParametroReq(requisicao, "op");
  
  int valA = extrairSinal4Bits(aBruto);
  int valB = extrairSinal4Bits(bBruto);
  int valorTotal = 0;
  bool possuiErro = false;

  if (operacao == "add") {
    valorTotal = valA + valB;
  } else if (operacao == "sub") {
    valorTotal = valA - valB;
  } else if (operacao == "mul") {
    valorTotal = valA * valB;
  } else if (operacao == "div") {
    if (valB == 0) possuiErro = true; 
    else valorTotal = valA / valB;
  } else if (operacao == "fat") {
    if (valA < 0) possuiErro = true; 
    else valorTotal = calcularFatorial(valA);
  } else {
    possuiErro = true;
  }

  bool flagOverflow = (valorTotal < -8 || valorTotal > 7);
  int mascaraBits = valorTotal & 0x0F;
  int resultadoFinal = extrairSinal4Bits(mascaraBits);
  
  atualizarPainelLeds(mascaraBits);

  String respostaJson;
  if (possuiErro) {
    respostaJson = "{\"erro\":true}";
  } else {
    respostaJson = "{\"resultado\":" + String(resultadoFinal) +
                   ",\"bits\":" + String(mascaraBits) +
                   ",\"completo\":" + String(valorTotal) +
                   ",\"overflow\":" + (flagOverflow ? "true" : "false") + "}";
  }

  cliente.println("HTTP/1.1 200 OK");
  cliente.println("Content-type:application/json");
  cliente.println("Connection: close");
  cliente.println();
  cliente.println(respostaJson);
}

// =========================================
// BENCHMARK — funcoes de tempo e impressao
// =========================================

String formatarParaBinario(int valor, int totalBits) {
  int mascara = (1 << totalBits) - 1;
  valor &= mascara;
  String stringBinaria = "";
  for (int i = totalBits - 1; i >= 0; i--) {
    stringBinaria += String((valor >> i) & 1);
  }
  return stringBinaria;
}

unsigned long benchmarkMultiplicacaoUs(int operadorA, int operadorB) {
  volatile int32_t registradorSaida;
  int32_t copiaA = operadorA;
  int32_t copiaB = operadorB;
  
  unsigned long tempoInicial = micros();
  for (unsigned long i = 0; i < N_REPETICOES; i++) {
    registradorSaida = copiaA * copiaB;
  }
  return micros() - tempoInicial;
}

unsigned long benchmarkFatorialUs(int num) {
  volatile int registradorSaida;
  unsigned long tempoInicial = micros();
  for (unsigned long i = 0; i < N_REPETICOES; i++) {
    registradorSaida = calcularFatorial(num);
  }
  return micros() - tempoInicial;
}

void rodarBenchmarkFatorialSerial() {
  const int arrayValores[3] = {3, 5, 8};

  Serial.println("\n=========================================");
  Serial.println("  BENCHMARK DE FATORIAL");
  Serial.printf("  N_REP = %lu repeticoes por medicao\n", N_REPETICOES);
  Serial.println("=========================================\n");
  Serial.println("N   | Medida | Resultado | Tempo(us)");
  Serial.println("----|--------|-----------|----------");

  for (int g = 0; g < 3; g++) {
    int alvoN = arrayValores[g];
    int resFat = calcularFatorial(alvoN);

    for (int m = 1; m <= 10; m++) {
      unsigned long duracao = benchmarkFatorialUs(alvoN);
      char logBuffer[60];
      snprintf(logBuffer, sizeof(logBuffer), " %2d |    %2d  | %9d | %lu", alvoN, m, resFat, duracao);
      Serial.println(logBuffer);
    }

    if (g < 2) {
      Serial.println("----|--------|-----------|----------");
    }
  }

  Serial.println("\n=========================================");
  Serial.println("  Benchmark de fatorial concluido.");
  Serial.println("=========================================");
}

void rodarBenchmarkMultiplicacaoSerial() {
  const CasoTeste* referenciasListas[3] = {casos2Bits, casos4Bits, casos8Bits};
  const int vetorBits[3]                 = {2, 4, 8};
  const int limitesMinimos[3]            = {-2, -8, -128};
  const int limitesMaximos[3]            = {1, 7, 127};

  Serial.println("\n=========================================");
  Serial.println("  BENCHMARK DE MULTIPLICACAO (30 CASOS POR TIPO)");
  Serial.printf("  N_REP = %lu repeticoes por caso\n", N_REPETICOES);
  Serial.println("=========================================");

  for (int g = 0; g < 3; g++) {
    int numBits   = vetorBits[g];
    int valMinimo = limitesMinimos[g];
    int valMaximo = limitesMaximos[g];

    Serial.printf("\n--- %d bits  (faixa: %d a %d) ---\n", numBits, valMinimo, valMaximo);
    Serial.println("Caso | A          | B          | Tempo(us) | Overflow");
    Serial.println("-----|------------|------------|-----------|--------");

    for (int i = 0; i < 30; i++) {
      int dadoA = referenciasListas[g][i].a;
      int dadoB = referenciasListas[g][i].b;
      long resultadoMultiplicacao = (long)dadoA * (long)dadoB;
      bool statusOverflow = (resultadoMultiplicacao < valMinimo || resultadoMultiplicacao > valMaximo);
      unsigned long tempoGasto = benchmarkMultiplicacaoUs(dadoA, dadoB);

      String strBinA = formatarParaBinario(dadoA, numBits);
      String strBinB = formatarParaBinario(dadoB, numBits);

      char logBuffer[80];
      snprintf(logBuffer, sizeof(logBuffer), " %3d | %-10s | %-10s | %9lu | %s",
               i + 1, strBinA.c_str(), strBinB.c_str(), tempoGasto, statusOverflow ? "SIM" : "nao");
      Serial.println(logBuffer);
    }
  }

  Serial.println("\n=========================================");
  Serial.println("  Benchmark concluido.");
  Serial.println("=========================================");
}

// =========================================
// Setup
// =========================================

void setup() {
  Serial.begin(115200);
  delay(500);

  for (int i = 0; i < 4; i++) {
    pinMode(LEDS_PINOS[i], OUTPUT);
  }

  rodarBenchmarkMultiplicacaoSerial();
  rodarBenchmarkFatorialSerial();

  WiFi.softAP(SSID_RED, PASS_RED);
  Serial.println("\nWiFi iniciado");
  Serial.println(WiFi.softAPIP());
  server.begin();
}

// =========================================
// Loop
// =========================================

void loop() {
  WiFiClient clienteConectado = server.available();
  if (!clienteConectado) return;

  String linhaRequisicao = clienteConectado.readStringUntil('\r');
  clienteConectado.readStringUntil('\n');

  if (linhaRequisicao.indexOf("GET /calc") >= 0) {
    processarRequisicaoCalc(clienteConectado, linhaRequisicao);
  } else {
    clienteConectado.println("HTTP/1.1 200 OK");
    clienteConectado.println("Content-type:text/html");
    clienteConectado.println("Connection: close");
    clienteConectado.println();
    clienteConectado.println(paginaWeb);
  }

  delay(1);
  clienteConectado.stop();
}