// LEDs nos pinos 4, 5, 6 e 7


int leds[] = {4, 5, 6, 7};


void setup() {


  for(int i = 0; i < 4; i++) {
    pinMode(leds[i], OUTPUT);
  }


}


// Função para apagar todos os LEDs
void apagarTudo() {


  for(int i = 0; i < 4; i++) {
    digitalWrite(leds[i], LOW);
  }


}


void loop() {


  // =========================
  // 1. Sequência normal
  // =========================
  for(int i = 0; i < 4; i++) {


    digitalWrite(leds[i], HIGH);
    delay(200);
    digitalWrite(leds[i], LOW);


  }


  delay(500);


  // =========================
  // 2. Sequência reversa
  // =========================
  for(int i = 3; i >= 0; i--) {


    digitalWrite(leds[i], HIGH);
    delay(200);
    digitalWrite(leds[i], LOW);


  }


  delay(500);


  // =========================
  // 3. Todos piscando juntos
  // =========================
  for(int j = 0; j < 3; j++) {


    for(int i = 0; i < 4; i++) {
      digitalWrite(leds[i], HIGH);
    }


    delay(300);


    apagarTudo();


    delay(300);
  }


  // =========================
  // 4. Efeito ping-pong
  // =========================
  for(int i = 0; i < 4; i++) {


    digitalWrite(leds[i], HIGH);
    delay(150);
    digitalWrite(leds[i], LOW);


  }


  for(int i = 2; i > 0; i--) {


    digitalWrite(leds[i], HIGH);
    delay(150);
    digitalWrite(leds[i], LOW);


  }


  delay(500);


  // =========================
  // 5. Acende acumulando
  // =========================
  apagarTudo();


  for(int i = 0; i < 4; i++) {


    digitalWrite(leds[i], HIGH);
    delay(250);


  }


  delay(500);


  // Apaga acumulando
  for(int i = 3; i >= 0; i--) {


    digitalWrite(leds[i], LOW);
    delay(250);


  }


  delay(500);


  // =========================
  // 6. Alternados
  // =========================
  for(int j = 0; j < 4; j++) {


    digitalWrite(4, HIGH);
    digitalWrite(6, HIGH);


    digitalWrite(5, LOW);
    digitalWrite(7, LOW);


    delay(250);


    digitalWrite(4, LOW);
    digitalWrite(6, LOW);


    digitalWrite(5, HIGH);
    digitalWrite(7, HIGH);


    delay(250);
  }


  apagarTudo();


  delay(1000);


}

