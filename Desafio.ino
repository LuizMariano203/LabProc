// ================================
// COMPLEMENTO DE 1 - SERIAL ONLY
// ESP32
// ================================

#include <Arduino.h>

// ================================
// CONVERTE 4 BITS (COMPLEMENTO 1)
// ================================
int convComplemento1(String bin){

    int v = strtol(bin.c_str(), NULL, 2);

    // se MSB = 1 → negativo em complemento de 1
    if(v & 0x08){
        v = (~v) & 0x0F; // inverte 4 bits
        v = -v;
    }

    return v;
}

// ================================
// CONVERTE PARA 4 BITS (SAÍDA)
// ================================
String to4bits(int v){

    v &= 0x0F;

    String r = "";

    for(int i = 3; i >= 0; i--){
        r += String((v >> i) & 1);
    }

    return r;
}

// ================================
// PROCESSAMENTO
// ================================
void processar(String linha){

    linha.trim();

    // esperado: "0110 0011 +"
    int sp1 = linha.indexOf(' ');
    int sp2 = linha.indexOf(' ', sp1 + 1);

    if(sp1 == -1 || sp2 == -1){
        Serial.println("ERRO FORMATO");
        return;
    }

    String A = linha.substring(0, sp1);
    String B = linha.substring(sp1 + 1, sp2);
    String OP = linha.substring(sp2 + 1);

    int a = convComplemento1(A);
    int b = convComplemento1(B);

    int resultado = 0;

    // =========================
    // ARITMÉTICA EM C (ESP32)
    // =========================
    if(OP == "+"){
        resultado = a + b;
    } else if(OP == "-"){
        resultado = a - b;
    } else {
        Serial.println("OPERACAO INVALIDA");
        return;
    }

    // =========================
    // END AROUND CARRY (COMP 1)
    // =========================
    if(resultado > 7){
        resultado = (resultado & 0x0F) + 1;
    }

    bool overflow = (resultado > 7 || resultado < -7);

    String bits = to4bits(resultado);

    // =========================
    // SAÍDA SERIAL
    // =========================
    Serial.print("RESULTADO: ");
    Serial.print(bits);

    if(overflow){
        Serial.print(" | OVERFLOW");
    }

    Serial.println();
}

// ================================
// SETUP
// ================================
void setup(){

    Serial.begin(115200);

    Serial.println("================================");
    Serial.println("CALCULADORA COMPLEMENTO DE 1");
    Serial.println("FORMATO: A B + ou A B -");
    Serial.println("EX: 0110 0011 +");
    Serial.println("================================");
}

// ================================
// LOOP
// ================================
void loop(){

    if(Serial.available()){

        String linha = Serial.readStringUntil('\n');

        processar(linha);
    }
}
