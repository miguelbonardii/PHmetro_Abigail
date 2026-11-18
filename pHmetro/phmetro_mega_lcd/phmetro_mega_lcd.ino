// =====================================================================
//  pHMETRO DE BANCADA  -  ARDUINO MEGA 2560 + LCD KEYPAD SHIELD
// =====================================================================
//  O QUE ESTE CÓDIGO FAZ:
//  - Lê a sonda de pH (pino A8).
//  - Mostra tudo no LCD 16x2, em 2 modos de leitura: pH e mV.
//  - Tem um modo de CALIBRAÇÃO com os tampões 4,01 / 6,86 / 9,18.
//  - Guarda a calibração na memória EEPROM, que NÃO apaga ao desligar.
//  - Controla os 4 LEDs (azul, amarelo, vermelho e transparente).
//
//  BOTÕES DO SHIELD:
//    UP / DOWN -> alterna o modo de leitura entre pH e mV
//    SELECT    -> entra na calibração
//    Na calibração:
//      UP / DOWN -> escolhe o tampão (4,01 / 6,86 / 9,18)
//      SELECT    -> grava o ponto do tampão escolhido
//      RIGHT     -> apaga a calibração (volta aos valores padrão)
//      LEFT      -> sai da calibração
//
//  LIGAÇÕES (a tabela completa está no arquivo LIGACOES.txt):
//    LCD Keypad Shield -> encaixado no Mega (usa 4 a 9 e A0)
//    Módulo PH-4502C:  Po -> A8 | V+ -> 5V | G -> GND  (o pino To não é usado)
//    Sonda: sonda de pH universal com conector BNC (azul), sem sensor de
//    temperatura, rosqueada no BNC do módulo.
//    LEDs (com resistor de 220 ohm): 22 azul, 23 amarelo,
//                                    26 vermelho, 27 transparente
//
//  NÃO PRECISA INSTALAR NADA DE FORA: LiquidCrystal e EEPROM já vêm
//  junto com a IDE do Arduino.
// =====================================================================


// ======================= BIBLIOTECAS =======================
// Biblioteca é um código pronto que outras pessoas escreveram.

#include <LiquidCrystal.h>  // sabe "conversar" com o LCD 16x2
#include <EEPROM.h>         // lê e grava na EEPROM, a memória que guarda
                            // dados mesmo com o Arduino desligado


// ======================= PINOS =======================

// Cria o objeto "lcd", que representa a tela. Os números são os pinos
// que o shield já liga por baixo. Ordem: RS, Enable, D4, D5, D6, D7.
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// "const" quer dizer que o valor nunca muda enquanto o programa roda.
// "uint8_t" é um número inteiro pequeno (0 a 255), suficiente para um pino.
const uint8_t PIN_BOTOES = A0;  // os 5 botões do shield ficam todos no A0
const uint8_t PIN_PH     = A8;  // sinal da sonda (pino Po do módulo)

const uint8_t LED_LIGADO  = 22;  // Azul: aceso fixo, sistema ligado
const uint8_t LED_LEITURA = 23;  // Amarelo: pisca a cada leitura; fixo se estável
const uint8_t LED_ALERTA  = 26;  // Vermelho: pisca com o pH fora da faixa
const uint8_t LED_CALIB   = 27;  // Transparente: aceso na calibração

// ATENÇÃO: o pino 10 controla a luz de fundo do LCD.
// Este código NÃO mexe nele de propósito: em alguns shields,
// colocar o pino 10 em HIGH pode danificar o Arduino.


// ======================= CONFIGURAÇÕES =======================

// Faixa de pH considerada normal. Fora dela, o LED vermelho pisca.
const float PH_MIN = 6.0;
const float PH_MAX = 8.0;

// Valores dos tampões (soluções com pH conhecido, usadas como "régua").
// Esses valores valem para cerca de 25 graus Celsius.
const float TAMPOES[3] = {4.01, 6.86, 9.18};

// Quantas leituras entram em cada média.
const uint8_t NUM_AMOSTRAS = 20;

// Tempos, em milissegundos (1000 ms = 1 segundo).
const unsigned long INTERVALO_LEITURA = 500;   // mede 2 vezes por segundo
const unsigned long INTERVALO_PISCA   = 250;   // velocidade do pisca vermelho
const unsigned long PULSO_LEITURA     = 80;    // tempo aceso do amarelo
const unsigned long TEMPO_DEBOUNCE    = 30;    // botão precisa ficar firme por 30 ms
const unsigned long TEMPO_AVISO       = 1000;  // tempo de uma mensagem na tela
const unsigned long TEMPO_AVISO_SONDA = 3000;  // a saúde da sonda tem números: fica mais tempo

// Estabilidade: a leitura é considerada estável quando as últimas
// LEITURAS_ESTABILIDADE tensões variaram, no máximo, VARIACAO_ESTAVEL.
// 10 leituras x 500 ms = 5 segundos de observação.
// 0,010 V = 10 mV, que no PH-4502C dá cerca de 0,05 de pH (10 / 180 mV por pH).
// Usamos a tensão, e não o pH, para o critério valer igual antes e durante
// a calibração.
const uint8_t LEITURAS_ESTABILIDADE = 10;
const float   VARIACAO_ESTAVEL      = 0.010;  // em volts


// ======================= STRUCT DA CALIBRAÇÃO =======================
// A conversão de tensão (V) para pH é uma reta:
//     pH = inclinacao * V + intercepto
// No módulo PH-4502C, quanto MAIOR o pH, MENOR a tensão. Por isso a
// inclinação é negativa. Os valores padrão abaixo são aproximados:
// pH 7 perto de 2,5 V, mudando cerca de 0,18 V por ponto de pH.
// Depois de calibrar, o Arduino calcula a reta certa para a SUA sonda.

const float INCLINACAO_PADRAO = -5.56;  // pontos de pH por volt (= -1 / 0,18)
const float INTERCEPTO_PADRAO = 20.9;   // = 7 + 5,56 * 2,5

// Limites para aceitar uma calibração. Uma sonda normal fica perto de
// -5,56. Fora desta faixa, é quase certo que houve engano: tampão errado,
// sonda fora da solução ou fio solto. (-2 = 0,5 V por pH; -20 = 0,05 V por pH.)
const float INCLINACAO_MAIS_FRACA = -2.0;
const float INCLINACAO_MAIS_FORTE = -20.0;

// "struct" junta várias variáveis num pacote só. Fica mais fácil
// gravar tudo de uma vez na EEPROM.
struct Calibracao {
  uint16_t marca;     // número "assinatura": diz se a EEPROM já foi gravada
  float inclinacao;   // quanto o pH muda para cada volt
  float intercepto;   // o pH que a reta daria se a tensão fosse 0 V
  float tensao[3];    // tensão medida em cada tampão
  bool  temPonto[3];  // quais tampões já foram gravados
};

// Assinatura: qualquer número serve, desde que seja fixo. Uma EEPROM nova
// dificilmente terá este valor por acaso, então ele revela a primeira execução.
const uint16_t MARCA_VALIDA = 0xA5C3;

Calibracao cal;  // a calibração em uso neste momento


// ======================= ESTADO DO PROGRAMA =======================
// Variáveis que guardam "como o programa está" agora.

// "enum" dá nomes para números. Fica mais legível que usar 0, 1, 2...
enum Modo { MODO_PH, MODO_MV, MODO_CALIB };
Modo modo = MODO_PH;          // o programa começa mostrando o pH

uint8_t tampaoEscolhido = 1;  // posição na lista TAMPOES; 1 é o 6,86 (o do meio)

enum Botao { NENHUM, DIREITA, CIMA, BAIXO, ESQUERDA, SELECT };
Botao botaoLidoAntes = NENHUM;  // o que foi lido na volta anterior do loop
Botao botaoFirme     = NENHUM;  // o último botão que passou pelo debounce
unsigned long momentoMudanca = 0;  // quando a leitura do botão mudou pela última vez

float tensaoPH   = 0;  // última tensão lida da sonda, em volts
float ph         = 7;  // último pH calculado (começa neutro para não dar alerta)

// Histórico das últimas tensões, usado para saber se a leitura estabilizou.
// Funciona como uma fila circular: quando chega ao fim, volta ao começo e
// escreve por cima da leitura mais antiga.
float historico[LEITURAS_ESTABILIDADE];
uint8_t posicaoHistorico   = 0;  // onde a próxima leitura será guardada
uint8_t leiturasNoHistorico = 0; // quantas posições já têm leitura (até 10)
bool leituraEstavel = false;     // true quando a leitura parou de variar

// Cronômetros: guardam o momento em que cada coisa aconteceu pela última vez.
// "unsigned long" é o tipo que millis() usa: um inteiro grande e sem sinal.
unsigned long ultimaLeitura = 0;  // quando foi feita a última leitura
unsigned long ultimoPisca   = 0;  // quando o vermelho trocou de estado
unsigned long inicioPulso   = 0;  // quando o amarelo acendeu
unsigned long inicioAviso   = 0;  // quando a mensagem atual apareceu

bool estadoPisca = false;  // o vermelho está aceso (true) ou apagado (false)?
bool avisoAtivo  = false;  // há uma mensagem temporária na tela?
unsigned long duracaoAviso = TEMPO_AVISO;  // quanto tempo a mensagem atual fica na tela


// =====================================================================
// LEITURAS
// =====================================================================

// Lê um pino analógico várias vezes e calcula a média.
// Recebe: o pino a ser lido.
// Devolve: a tensão média nesse pino, em volts.
float lerTensao(uint8_t pino) {
  // O Arduino tem um único conversor, que é dividido entre os pinos.
  // A primeira leitura depois de trocar de pino pode vir "contaminada"
  // pelo pino anterior, então lemos uma vez e jogamos fora.
  analogRead(pino);

  // A média reduz o "ruído", que faz o valor ficar pulando.
  // "long" é um inteiro grande: dá folga se NUM_AMOSTRAS for aumentado,
  // pois o "int" do Arduino só vai até 32767.
  long soma = 0;
  for (uint8_t i = 0; i < NUM_AMOSTRAS; i++) {
    soma += analogRead(pino);
  }

  // analogRead devolve de 0 a 1023, que representa de 0 a 5 V.
  // O "(float)" obriga a divisão a manter as casas decimais.
  float media = soma / (float)NUM_AMOSTRAS;
  return media * 5.0 / 1023.0;
}

// Converte tensão em pH usando a reta da calibração.
// Recebe: a tensão em volts. Devolve: o valor de pH.
float calcularPH(float tensao) {
  return cal.inclinacao * tensao + cal.intercepto;
}

// Guarda uma tensão no histórico, no lugar da mais antiga.
// Recebe: a tensão em volts. Não devolve nada.
void guardarNoHistorico(float tensao) {
  historico[posicaoHistorico] = tensao;
  // Avança uma posição; o "% LEITURAS_ESTABILIDADE" faz voltar a 0 no fim.
  posicaoHistorico = (posicaoHistorico + 1) % LEITURAS_ESTABILIDADE;
  if (leiturasNoHistorico < LEITURAS_ESTABILIDADE) {
    leiturasNoHistorico++;
  }
}

// Confere se a leitura está estável: compara a maior e a menor tensão
// do histórico. Se a diferença for pequena, a sonda parou de variar.
// Não recebe nada. Devolve: true se estável, false se ainda variando.
bool verificarEstabilidade() {
  // Logo depois de ligar ainda não há leituras suficientes para decidir.
  if (leiturasNoHistorico < LEITURAS_ESTABILIDADE) {
    return false;
  }

  float maior = historico[0];
  float menor = historico[0];
  for (uint8_t i = 1; i < LEITURAS_ESTABILIDADE; i++) {
    if (historico[i] > maior) maior = historico[i];
    if (historico[i] < menor) menor = historico[i];
  }
  return (maior - menor) <= VARIACAO_ESTAVEL;
}


// =====================================================================
// BOTÕES
// =====================================================================
// Os 5 botões estão ligados no MESMO pino (A0), cada um com um resistor
// diferente (divisor de tensão). Assim, cada botão gera uma tensão
// diferente, e o Arduino descobre qual foi apertado pelo valor lido.

// Descobre qual botão está apertado AGORA.
// Não recebe nada. Devolve: o botão, ou NENHUM.
Botao lerBotao() {
  int valor = analogRead(PIN_BOTOES);
  if (valor < 60)  return DIREITA;
  if (valor < 200) return CIMA;
  if (valor < 400) return BAIXO;
  if (valor < 600) return ESQUERDA;
  if (valor < 800) return SELECT;
  return NENHUM;  // perto de 1023 = nenhum botão apertado
}

// Detecta o CLIQUE: devolve o botão só uma vez, no momento em que é apertado.
// Sem isso, segurar o botão repetiria a ação centenas de vezes.
//
// Debounce: ao apertar, o contato metálico "treme" por alguns milissegundos
// e a leitura pula. Por isso, só aceitamos um valor que ficou igual por
// TEMPO_DEBOUNCE. Usamos millis() para não parar o resto do programa.
//
// Recebe: o tempo atual. Devolve: o botão clicado, ou NENHUM.
Botao botaoClicado(unsigned long agora) {
  Botao lido = lerBotao();

  // A leitura mudou? Então recomeça a contar o tempo.
  if (lido != botaoLidoAntes) {
    botaoLidoAntes = lido;
    momentoMudanca = agora;
  }

  // Ficou igual por tempo suficiente E é diferente do último valor aceito?
  if (agora - momentoMudanca >= TEMPO_DEBOUNCE && lido != botaoFirme) {
    botaoFirme = lido;
    // Soltar o botão também é uma mudança, mas só apertar conta como clique.
    if (botaoFirme != NENHUM) {
      return botaoFirme;
    }
  }
  return NENHUM;
}


// =====================================================================
// CALIBRAÇÃO
// =====================================================================

// Coloca os valores padrão na variável "cal" e esquece os pontos gravados.
// Não recebe nem devolve nada.
void calibracaoPadrao() {
  cal.marca = MARCA_VALIDA;
  cal.inclinacao = INCLINACAO_PADRAO;
  cal.intercepto = INTERCEPTO_PADRAO;
  for (uint8_t i = 0; i < 3; i++) {
    cal.tensao[i] = 0;
    cal.temPonto[i] = false;
  }
}

// Conta quantos tampões já foram gravados.
// Não recebe nada. Devolve: um número de 0 a 3.
uint8_t pontosGravados() {
  uint8_t n = 0;
  for (uint8_t i = 0; i < 3; i++) {
    if (cal.temPonto[i]) n++;
  }
  return n;
}

// Calcula a linearidade da calibração (o R², "R ao quadrado"): o quanto os
// 3 pontos ficam em cima de uma linha reta. 1 = em cima da reta (100%).
// Com 2 pontos sempre existe uma reta que passa pelos dois, então o R²
// daria sempre 1 e não diria nada: por isso exigimos os 3 tampões.
// Não recebe nada. Devolve: o R² (de 0 a 1), ou -1 se faltar tampão.
float calcularLinearidade() {
  if (pontosGravados() < 3) {
    return -1;
  }

  // Médias da tensão (x) e do pH dos tampões (y).
  float mediaX = 0, mediaY = 0;
  for (uint8_t i = 0; i < 3; i++) {
    mediaX += cal.tensao[i];
    mediaY += TAMPOES[i];
  }
  mediaX /= 3;
  mediaY /= 3;

  // Somas dos desvios de cada ponto em relação à média.
  float sxy = 0, sxx = 0, syy = 0;
  for (uint8_t i = 0; i < 3; i++) {
    float dx = cal.tensao[i] - mediaX;
    float dy = TAMPOES[i] - mediaY;
    sxy += dx * dy;
    sxx += dx * dx;
    syy += dy * dy;
  }

  // Tensões todas iguais: a sonda não respondeu, então a linearidade é 0.
  if (sxx < 0.000001) {
    return 0;
  }
  return (sxy * sxy) / (sxx * syy);
}

// Recalcula a reta usando os tampões já gravados.
// - 1 ponto: mantém a inclinação e só ajusta o intercepto.
// - 2 ou 3 pontos: calcula a reta que passa mais perto de todos
//   (método dos "mínimos quadrados"; veja o EXPLICACAO.txt).
// Não recebe nada. Devolve: true se a reta faz sentido, false se não faz.
bool recalcularReta() {
  // x é a tensão, y é o pH do tampão. As letras "s" são somas.
  uint8_t n = 0;
  float sx = 0, sy = 0, sxx = 0, sxy = 0;

  for (uint8_t i = 0; i < 3; i++) {
    if (cal.temPonto[i]) {
      float x = cal.tensao[i];
      float y = TAMPOES[i];
      n++;
      sx += x;
      sy += y;
      sxx += x * x;
      sxy += x * y;
    }
  }

  if (n == 0) {
    return true;  // sem pontos: não há nada para recalcular
  }

  if (n == 1) {
    // Com 1 ponto, sx é a tensão e sy é o pH desse único tampão.
    // Isolando o intercepto em "pH = inclinacao * V + intercepto":
    cal.intercepto = sy - cal.inclinacao * sx;
    return true;
  }

  // Fórmula dos mínimos quadrados. Se todas as tensões forem praticamente
  // iguais, o denominador fica perto de zero e a conta não tem solução.
  float denominador = n * sxx - sx * sx;
  if (fabs(denominador) < 0.000001) {
    return false;
  }

  float novaInclinacao = (n * sxy - sx * sy) / denominador;

  // A reta precisa descer (inclinação negativa) e ter um tamanho plausível.
  if (novaInclinacao > INCLINACAO_MAIS_FRACA || novaInclinacao < INCLINACAO_MAIS_FORTE) {
    return false;
  }

  cal.inclinacao = novaInclinacao;
  cal.intercepto = (sy - novaInclinacao * sx) / n;
  return true;
}

// Mede a sonda agora e grava o ponto do tampão escolhido.
// Se a reta resultante não fizer sentido, desfaz tudo e avisa na tela.
// Não recebe nem devolve nada.
void gravarPonto() {
  Calibracao copia = cal;  // guarda como estava, para poder desfazer

  // Mede de novo, em vez de usar a leitura anterior (de até 500 ms atrás).
  cal.tensao[tampaoEscolhido] = lerTensao(PIN_PH);
  cal.temPonto[tampaoEscolhido] = true;

  if (recalcularReta()) {
    // put() só regrava os bytes que mudaram, o que poupa a EEPROM.
    EEPROM.put(0, cal);
    mostrarAviso(F("Ponto gravado!"), F("Lave a sonda"));
  } else {
    cal = copia;
    mostrarAviso(F("Ponto recusado"), F("Confira tampao"));
  }
}


// =====================================================================
// LEDs
// =====================================================================

// Acende os LEDs em sequência ao ligar, para conferir se todos funcionam.
// Aqui o delay() não atrapalha: o programa ainda nem começou a medir.
// Não recebe nem devolve nada.
void sequenciaInicial() {
  const uint8_t leds[4] = {LED_LIGADO, LED_LEITURA, LED_ALERTA, LED_CALIB};

  for (uint8_t i = 0; i < 4; i++) {
    digitalWrite(leds[i], HIGH);
    delay(200);
  }
  delay(300);
  for (uint8_t i = 0; i < 4; i++) {
    digitalWrite(leds[i], LOW);
  }
}

// Decide o estado de cada LED. Usa millis() (um cronômetro interno) em vez
// de delay(): com delay(), o Arduino ficaria parado enquanto o LED pisca,
// sem ler botões nem a sonda. Com millis(), ele só confere "já deu o tempo?"
// e segue em frente.
// Recebe: o tempo atual. Não devolve nada.
void atualizarLeds(unsigned long agora) {
  // Azul: sempre aceso enquanto o programa roda.
  digitalWrite(LED_LIGADO, HIGH);

  // Amarelo: aceso fixo se a leitura estiver estável. Se ainda estiver
  // variando, acende só nos primeiros 80 ms depois de cada leitura (pisca).
  if (leituraEstavel) {
    digitalWrite(LED_LEITURA, HIGH);
  } else if (agora - inicioPulso < PULSO_LEITURA) {
    digitalWrite(LED_LEITURA, HIGH);
  } else {
    digitalWrite(LED_LEITURA, LOW);
  }

  // Transparente: aceso enquanto estiver na calibração.
  if (modo == MODO_CALIB) {
    digitalWrite(LED_CALIB, HIGH);
  } else {
    digitalWrite(LED_CALIB, LOW);
  }

  // Vermelho: pisca se o pH sair da faixa. Fica desligado na calibração,
  // porque os tampões 4,01 e 9,18 estão fora da faixa de propósito.
  bool foraDaFaixa = (ph < PH_MIN || ph > PH_MAX);
  if (foraDaFaixa && modo != MODO_CALIB) {
    if (agora - ultimoPisca >= INTERVALO_PISCA) {
      ultimoPisca = agora;
      estadoPisca = !estadoPisca;  // "!" inverte: aceso vira apagado e vice-versa
    }
  } else {
    estadoPisca = false;
  }
  digitalWrite(LED_ALERTA, estadoPisca ? HIGH : LOW);
}


// =====================================================================
// TELA
// =====================================================================
// O LCD tem 2 linhas de 16 caracteres. setCursor(coluna, linha) começa em
// (0, 0), no canto de cima à esquerda. Os espaços no fim dos textos apagam
// restos do texto anterior. Os textos não têm acento porque o LCD não
// tem essas letras. F("...") guarda o texto na memória de programa (flash),
// economizando a memória RAM, que é bem menor.

// Escreve na tela as informações do modo atual.
// Não recebe nem devolve nada.
void desenharTela() {
  lcd.setCursor(0, 0);

  if (modo == MODO_PH) {
    lcd.print(F("pH: "));
    lcd.print(ph, 2);  // o 2 significa "duas casas decimais"
    lcd.print(F("    "));
    lcd.setCursor(0, 1);
    if (ph < PH_MIN) {
      lcd.print(F("ACIDO - ALERTA! "));
    } else if (ph > PH_MAX) {
      lcd.print(F("BASICO - ALERTA!"));
    } else {
      lcd.print(F("Faixa normal    "));
    }
  }

  else if (modo == MODO_MV) {
    lcd.print(F("Sinal da sonda  "));
    lcd.setCursor(0, 1);
    lcd.print(tensaoPH * 1000, 0);  // volts x 1000 = milivolts, sem casas decimais
    lcd.print(F(" mV    "));
  }

  else {  // MODO_CALIB
    lcd.print(F("CAL tampao "));
    lcd.print(TAMPOES[tampaoEscolhido], 2);
    // O asterisco indica que este tampão já foi gravado.
    if (cal.temPonto[tampaoEscolhido]) {
      lcd.print(F("*"));
    } else {
      lcd.print(F(" "));
    }
    lcd.setCursor(0, 1);
    lcd.print(tensaoPH, 3);
    lcd.print(F("V pts:"));
    lcd.print(pontosGravados());
    lcd.print(F("   "));
  }
}

// Mostra uma mensagem por 1 segundo, sem usar delay(). Aqui ela só aparece;
// quem a retira depois é o loop(), quando o tempo acabar.
// Recebe: o texto da linha de cima e o da linha de baixo. Não devolve nada.
void mostrarAviso(const __FlashStringHelper* linha1, const __FlashStringHelper* linha2) {
  lcd.clear();
  lcd.print(linha1);
  lcd.setCursor(0, 1);
  lcd.print(linha2);
  avisoAtivo = true;
  inicioAviso = millis();
  duracaoAviso = TEMPO_AVISO;
}

// Mostra a "saúde da sonda" por 3 segundos, ao sair da calibração.
// - Sensibilidade: quantos mV o sinal muda por ponto de pH. Química: a ponta
//   de vidro gera cerca de 59 mV por ponto de pH (equação de Nernst, 25 °C),
//   e o módulo amplifica isso. Sonda velha ou suja = sensibilidade menor.
// - Linearidade: o R² dos 3 tampões, em %. Acima de 99% é ótimo.
// Só é chamada com 2 ou mais pontos, quando a inclinação já foi calculada.
// Não recebe nem devolve nada.
void mostrarSaudeSonda() {
  // A inclinação está em pH por volt e é negativa. Invertendo (1 / x) temos
  // volts por pH; vezes 1000, milivolts por pH. O "-" deixa o número positivo.
  float sensibilidade = 1000.0 / -cal.inclinacao;
  float linearidade = calcularLinearidade();

  lcd.clear();
  lcd.print(F("Sens: "));
  lcd.print(sensibilidade, 0);
  lcd.print(F(" mV/pH"));
  lcd.setCursor(0, 1);
  if (linearidade < 0) {
    lcd.print(F("Linear: use 3"));  // só 2 tampões: falta um para o R²
  } else {
    lcd.print(F("Linear: "));
    lcd.print(linearidade * 100, 1);  // de 0..1 para %, com 1 casa decimal
    lcd.print(F("%"));
  }

  avisoAtivo = true;
  inicioAviso = millis();
  duracaoAviso = TEMPO_AVISO_SONDA;
}


// =====================================================================
// AÇÕES DOS BOTÕES
// =====================================================================

// Executa o que cada botão deve fazer, conforme o modo atual.
// Recebe: o botão clicado. Não devolve nada.
void tratarBotao(Botao botao) {
  if (botao == NENHUM) {
    return;  // nada foi clicado, então não há nada a fazer
  }

  if (modo != MODO_CALIB) {
    // --- Modos de leitura ---
    // Com só dois modos, UP e DOWN fazem a mesma coisa: alternam entre eles.
    // "||" quer dizer "ou": vale para qualquer um dos dois botões.
    if (botao == CIMA || botao == BAIXO) {
      if (modo == MODO_PH) modo = MODO_MV;
      else                 modo = MODO_PH;
    }
    if (botao == SELECT) {
      modo = MODO_CALIB;
      mostrarAviso(F("Modo calibracao"), F("Lave a sonda!"));
    }
  } else {
    // --- Modo calibração ---
    // O "% 3" (resto da divisão por 3) faz a lista dar a volta: 0, 1, 2, 0...
    // Somar 2 com "% 3" tem o mesmo efeito de voltar uma posição.
    if (botao == CIMA)  tampaoEscolhido = (tampaoEscolhido + 1) % 3;
    if (botao == BAIXO) tampaoEscolhido = (tampaoEscolhido + 2) % 3;

    if (botao == SELECT) {
      gravarPonto();
    }
    if (botao == DIREITA) {
      calibracaoPadrao();
      EEPROM.put(0, cal);
      mostrarAviso(F("Calibracao"), F("apagada"));
    }
    if (botao == ESQUERDA) {
      modo = MODO_PH;
      // Com 2 ou 3 tampões gravados, já dá para avaliar a sonda.
      if (pontosGravados() >= 2) {
        mostrarSaudeSonda();
      } else {
        mostrarAviso(F("Saindo da"), F("calibracao"));
      }
    }
  }

  // Sem mensagem na tela, redesenha já, para o clique responder na hora.
  if (!avisoAtivo) {
    lcd.clear();
    desenharTela();
  }
}


// =====================================================================
// SETUP: roda UMA vez, quando o Arduino liga
// =====================================================================
void setup() {
  // Avisa que os pinos dos LEDs vão enviar sinal (saída).
  pinMode(LED_LIGADO, OUTPUT);
  pinMode(LED_LEITURA, OUTPUT);
  pinMode(LED_ALERTA, OUTPUT);
  pinMode(LED_CALIB, OUTPUT);

  // Avisa que o LCD tem 16 colunas e 2 linhas, e mostra a abertura.
  lcd.begin(16, 2);
  lcd.print(F("pHmetro Arduino"));
  lcd.setCursor(0, 1);
  lcd.print(F("Iniciando..."));
  sequenciaInicial();

  // Busca a calibração guardada na EEPROM, a partir do endereço 0.
  // Se a assinatura não bater, é a primeira execução: usa os valores padrão.
  EEPROM.get(0, cal);
  if (cal.marca != MARCA_VALIDA) {
    calibracaoPadrao();
    EEPROM.put(0, cal);
  }

  lcd.clear();
}


// =====================================================================
// LOOP: roda para SEMPRE, milhares de vezes por segundo
// =====================================================================
void loop() {
  unsigned long agora = millis();  // quantos ms se passaram desde que ligou

  // 1. Botões: conferidos em toda volta, para responder rápido.
  // Durante uma mensagem o clique é ignorado, pois a pessoa não está vendo
  // o estado atual e poderia, por exemplo, gravar o tampão errado.
  Botao clique = botaoClicado(agora);
  if (!avisoAtivo) {
    tratarBotao(clique);
  }

  // 2. Fim da mensagem: passado 1 segundo, volta à tela normal.
  if (avisoAtivo && agora - inicioAviso >= duracaoAviso) {
    avisoAtivo = false;
    lcd.clear();
    desenharTela();
  }

  // 3. Leitura e tela: a cada 500 ms.
  if (agora - ultimaLeitura >= INTERVALO_LEITURA) {
    ultimaLeitura = agora;
    tensaoPH = lerTensao(PIN_PH);
    ph = calcularPH(tensaoPH);
    guardarNoHistorico(tensaoPH);
    leituraEstavel = verificarEstabilidade();
    inicioPulso = agora;  // dispara a piscada do amarelo
    if (!avisoAtivo) {
      desenharTela();
    }
  }

  // 4. LEDs: atualizados em toda volta, para o pisca ficar certinho.
  atualizarLeds(agora);
}
