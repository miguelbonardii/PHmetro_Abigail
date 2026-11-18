// =====================================================================
//  TESTE DOS BOTÕES  -  ARDUINO MEGA 2560 + LCD KEYPAD SHIELD
// =====================================================================
//  O QUE ESTE CÓDIGO FAZ:
//  Mostra no LCD o valor lido no pino A0 e o nome do botão apertado.
//
//  PARA QUE SERVE:
//  Os 5 botões do shield usam o mesmo pino (A0), por meio de um divisor de
//  tensão: cada botão gera um valor diferente. Esses valores mudam um pouco
//  de um shield para outro. Se o nome na tela vier trocado, anote o valor de
//  cada botão e ajuste os limites aqui e na função lerBotao() do código
//  principal.
//
//  LIGAÇÕES: basta encaixar o shield no Mega. Nenhum fio extra é necessário.
// =====================================================================

#include <LiquidCrystal.h>  // biblioteca que controla o LCD 16x2

// Pinos que o shield usa para a tela: RS, Enable, D4, D5, D6, D7.
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

const uint8_t PIN_BOTOES = A0;  // o pino onde chegam os 5 botões

// Pausa entre leituras: rápida para responder e lenta para o número
// não virar um borrão na tela.
const unsigned long TEMPO_ATUALIZACAO = 150;

// Escreve na linha de baixo o nome do botão correspondente ao valor lido.
// Recebe: o valor do A0 (0 a 1023). Não devolve nada.
// Os espaços completam 16 caracteres e apagam o nome anterior.
void mostrarNomeBotao(int valor) {
  lcd.setCursor(0, 1);
  if (valor < 60)       lcd.print(F("Botao: RIGHT    "));
  else if (valor < 200) lcd.print(F("Botao: UP       "));
  else if (valor < 400) lcd.print(F("Botao: DOWN     "));
  else if (valor < 600) lcd.print(F("Botao: LEFT     "));
  else if (valor < 800) lcd.print(F("Botao: SELECT   "));
  else                  lcd.print(F("Botao: nenhum   "));
}

// SETUP: roda uma vez ao ligar. Prepara o LCD.
void setup() {
  lcd.begin(16, 2);  // o LCD tem 16 colunas e 2 linhas
}

// LOOP: roda para sempre. Lê o A0 e atualiza a tela.
void loop() {
  int valor = analogRead(PIN_BOTOES);  // número de 0 (0 V) a 1023 (5 V)

  lcd.setCursor(0, 0);
  lcd.print(F("A0: "));
  lcd.print(valor);
  lcd.print(F("    "));  // apaga dígitos que sobraram (ex.: de 1023 para 99)

  mostrarNomeBotao(valor);

  delay(TEMPO_ATUALIZACAO);
}
