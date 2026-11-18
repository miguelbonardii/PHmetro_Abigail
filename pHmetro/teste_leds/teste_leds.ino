// =====================================================================
//  TESTE DOS LEDs  -  ARDUINO MEGA 2560 + LCD KEYPAD SHIELD
// =====================================================================
//  O QUE ESTE CÓDIGO FAZ:
//  1. Acende os 4 LEDs um por vez, mostrando no LCD qual deveria acender.
//  2. Depois pisca em pares: azul + amarelo, e vermelho + transparente.
//  3. Repete tudo para sempre.
//  Se um LED não acender, confira o fio dele, a polaridade e o trilho de GND.
//
//  LIGAÇÕES: cada pino passa por um resistor de 220 ohm até a perna longa
//  (+) do LED. A perna curta (-) vai ao GND.
//    22 azul | 23 amarelo | 26 vermelho | 27 transparente
//
//  Aqui podemos usar delay(), que é mais simples: este teste não precisa
//  fazer mais nada enquanto espera. O código principal usa millis().
// =====================================================================

#include <LiquidCrystal.h>  // biblioteca que controla o LCD 16x2

// Pinos que o shield usa para a tela: RS, Enable, D4, D5, D6, D7.
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// Pinos dos LEDs, na ordem do teste.
const uint8_t LED_AZUL         = 22;
const uint8_t LED_AMARELO      = 23;
const uint8_t LED_VERMELHO     = 26;
const uint8_t LED_TRANSPARENTE = 27;

const unsigned long TEMPO_CADA_LED = 1000;  // cada LED fica aceso por 1 segundo
const unsigned long TEMPO_PISCA    = 300;   // velocidade do pisca em pares
const uint8_t       VEZES_PISCA    = 5;     // quantas vezes cada rodada pisca

// Mostra o nome do LED no LCD, acende-o, espera e apaga.
// Recebe: o pino do LED e o nome dele. Não devolve nada.
void testarUmLed(uint8_t pino, const __FlashStringHelper* nome) {
  lcd.clear();
  lcd.print(F("Deve acender:"));
  lcd.setCursor(0, 1);
  lcd.print(nome);

  digitalWrite(pino, HIGH);  // HIGH envia 5 V: o LED acende
  delay(TEMPO_CADA_LED);
  digitalWrite(pino, LOW);   // LOW envia 0 V: o LED apaga
}

// Coloca os 4 LEDs no estado pedido.
// Recebe: um estado (HIGH ou LOW) para cada par. Não devolve nada.
void acenderPares(uint8_t estadoPar1, uint8_t estadoPar2) {
  digitalWrite(LED_AZUL, estadoPar1);
  digitalWrite(LED_AMARELO, estadoPar1);
  digitalWrite(LED_VERMELHO, estadoPar2);
  digitalWrite(LED_TRANSPARENTE, estadoPar2);
}

// SETUP: roda uma vez ao ligar. Prepara os pinos e o LCD.
void setup() {
  pinMode(LED_AZUL, OUTPUT);
  pinMode(LED_AMARELO, OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  pinMode(LED_TRANSPARENTE, OUTPUT);

  lcd.begin(16, 2);  // o LCD tem 16 colunas e 2 linhas
}

// LOOP: roda para sempre. Primeiro um LED por vez, depois os pares.
void loop() {
  // Os nomes têm no máximo 16 caracteres, o tamanho de uma linha do LCD.
  testarUmLed(LED_AZUL, F("Azul pino 22"));
  testarUmLed(LED_AMARELO, F("Amarelo pino 23"));
  testarUmLed(LED_VERMELHO, F("Vermelho pino 26"));
  testarUmLed(LED_TRANSPARENTE, F("Transp. pino 27"));

  lcd.clear();
  lcd.print(F("Piscando pares"));
  lcd.setCursor(0, 1);
  lcd.print(F("2 LEDs de cada"));

  // Cada par acende enquanto o outro está apagado.
  for (uint8_t i = 0; i < VEZES_PISCA; i++) {
    acenderPares(HIGH, LOW);
    delay(TEMPO_PISCA);
    acenderPares(LOW, HIGH);
    delay(TEMPO_PISCA);
  }
  acenderPares(LOW, LOW);  // apaga tudo antes de recomeçar
  delay(500);
}
