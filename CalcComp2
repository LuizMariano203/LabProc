#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "Calculadora_ESP32";
const char* password = "12345678";

WebServer server(80);

// LEDs
int ledPins[4] = {4,5,6,7};

// ======================================================
// HTML COMPLETO DENTRO DO CÓDIGO C DO ESP32
// ======================================================

String paginaHTML = R"rawliteral(

<!DOCTYPE html>
<html lang="pt-BR">

<head>

<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">

<title>Calculadora 4 Bits</title>

<style>

*{
    margin:0;
    padding:0;
    box-sizing:border-box;
}

body{

    background: linear-gradient(135deg,#0f172a,#1e293b);

    min-height:100vh;

    display:flex;
    justify-content:center;
    align-items:center;

    font-family:Arial,sans-serif;

    color:white;
}

.container{

    width:90%;
    max-width:420px;

    background:rgba(255,255,255,0.08);

    border-radius:20px;

    padding:30px;

    backdrop-filter:blur(10px);

    box-shadow:
    0 10px 30px rgba(0,0,0,0.4);

}

h1{

    text-align:center;

    margin-bottom:30px;

    color:#38bdf8;
}

.card{

    margin-bottom:20px;
}

label{

    display:block;

    margin-bottom:10px;

    color:#cbd5e1;
}

input{

    width:100%;

    padding:15px;

    border:none;

    border-radius:12px;

    background:#0f172a;

    color:white;

    font-size:24px;

    text-align:center;

    letter-spacing:6px;

    outline:none;

    border:2px solid transparent;

    transition:0.3s;
}

input:focus{

    border:2px solid #38bdf8;

    box-shadow:0 0 10px #38bdf8;
}

.botoes{

    display:flex;

    gap:15px;

    margin-top:25px;
}

button{

    flex:1;

    padding:15px;

    border:none;

    border-radius:14px;

    font-size:18px;

    font-weight:bold;

    cursor:pointer;

    transition:0.3s;
}

.soma{

    background:#22c55e;
    color:white;
}

.soma:hover{

    background:#16a34a;
}

.sub{

    background:#ef4444;
    color:white;
}

.sub:hover{

    background:#dc2626;
}

.resultado{

    margin-top:30px;

    background:#0f172a;

    border-radius:15px;

    padding:20px;

    text-align:center;
}

.bits{

    font-size:42px;

    letter-spacing:10px;

    color:#22c55e;

    font-weight:bold;

    margin-top:10px;
}

.status{

    margin-top:15px;

    font-size:18px;

    font-weight:bold;
}

.ok{

    color:#22c55e;
}

.erro{

    color:#ef4444;
}

.footer{

    margin-top:25px;

    text-align:center;

    color:#94a3b8;

    font-size:12px;
}

</style>

</head>

<body>

<div class="container">

    <h1>Calculadora 4 Bits</h1>

    <div class="card">

        <label>Operando A</label>

        <input
            id="a"
            maxlength="4"
            value="0000"
        >

    </div>

    <div class="card">

        <label>Operando B</label>

        <input
            id="b"
            maxlength="4"
            value="0000"
        >

    </div>

    <div class="botoes">

        <button
            class="soma"
            onclick="calcular('add')">
            SOMA
        </button>

        <button
            class="sub"
            onclick="calcular('sub')">
            SUB
        </button>

    </div>

    <div class="resultado">

        <h2>Resultado</h2>

        <div class="bits" id="bits">
            0000
        </div>

        <div class="status ok" id="status">
            Sistema pronto
        </div>

    </div>

    <div class="footer">
        ESP32 • Complemento de Dois
    </div>

</div>

<script>

function validar(v){

    return /^[01]{4}$/.test(v);

}

async function calcular(op){

    let a = document.getElementById("a").value;
    let b = document.getElementById("b").value;

    if(!validar(a) || !validar(b)){

        document.getElementById("status").innerHTML =
        "Digite exatamente 4 bits";

        document.getElementById("status").className =
        "status erro";

        return;
    }

    try{

        const resposta =
        await fetch(`/calc?a=${a}&b=${b}&op=${op}`);

        const texto =
        await resposta.text();

        // RESULT:0101:OK

        let partes = texto.split(":");

        let bits = partes[1];
        let status = partes[2];

        document.getElementById("bits").innerHTML =
        bits;

        if(status == "OVERFLOW"){

            document.getElementById("status").innerHTML =
            "OVERFLOW DETECTADO";

            document.getElementById("status").className =
            "status erro";

        }
        else{

            document.getElementById("status").innerHTML =
            "Operação realizada";

            document.getElementById("status").className =
            "status ok";
        }

    }
    catch(e){

        document.getElementById("status").innerHTML =
        "Erro de comunicação";

        document.getElementById("status").className =
        "status erro";
    }

}

</script>

</body>
</html>

)rawliteral";

// ======================================================
// Página principal
// ======================================================

void handleRoot(){

    server.send(200,"text/html",paginaHTML);

}

// ======================================================
// Converte binário 4 bits -> signed int
// ======================================================

int converter4Bits(String bin){

    int valor =
    strtol(bin.c_str(), NULL, 2);

    // extensão de sinal

    if(valor & 0x08){

        valor |= 0xF0;
    }

    return valor;
}

// ======================================================
// Atualiza LEDs
// ======================================================

void atualizarLEDs(int valor){

    valor &= 0x0F;

    for(int i=0;i<4;i++){

        digitalWrite(
            ledPins[i],
            (valor >> i) & 1
        );
    }
}

// ======================================================
// Operações
// ======================================================

void handleCalc(){

    String a =
    server.arg("a");

    String b =
    server.arg("b");

    String op =
    server.arg("op");

    int valA =
    converter4Bits(a);

    int valB =
    converter4Bits(b);

    int resultado;

    // ====================================
    // ARITMÉTICA FEITA NO ESP32 EM C
    // ====================================

    if(op == "add"){

        resultado = valA + valB;
    }
    else{

        resultado = valA - valB;
    }

    bool overflow = false;

    if(resultado > 7 || resultado < -8){

        overflow = true;
    }

    atualizarLEDs(resultado);

    resultado &= 0x0F;

    char bits[5];

    for(int i=0;i<4;i++){

        bits[3-i] =
        ((resultado >> i) & 1) + '0';
    }

    bits[4] = '\0';

    String resposta = "RESULT:";
    resposta += bits;

    if(overflow){

        resposta += ":OVERFLOW";
    }
    else{

        resposta += ":OK";
    }

    server.send(200,"text/plain",resposta);
}

// ======================================================
// Setup
// ======================================================

void setup(){

    Serial.begin(115200);

    for(int i=0;i<4;i++){

        pinMode(ledPins[i], OUTPUT);
    }

    WiFi.softAP(ssid,password);

    Serial.println("WiFi iniciado");
    Serial.println(WiFi.softAPIP());

    server.on("/",handleRoot);

    server.on("/calc",handleCalc);

    server.begin();
}

// ======================================================
// Loop
// ======================================================

void loop(){

    server.handleClient();
}
