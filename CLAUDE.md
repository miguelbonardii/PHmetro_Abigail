# Projeto: Medidor de pH com Arduino

## Sobre o projeto
- Protótipo de bancada de um **pHmetro** feito em grupo, para as aulas de projeto.
- É o **primeiro projeto do grupo** com eletrônica e química: somos **iniciantes**.
- Integrantes: Miguel Bonardi, Lucas da Costa, Davi Lucas, Roger Riquelme e Vinicius.

## Pastas
Os códigos em uso ficam em `pHmetro/`. A IDE do Arduino exige que cada `.ino` fique numa pasta com o mesmo nome.
- `pHmetro/phmetro_mega_lcd/phmetro_mega_lcd.ino`: código principal.
- `pHmetro/teste_leds/teste_leds.ino`: teste dos 4 LEDs.
- `pHmetro/teste_botoes/teste_botoes.ino`: mostra no LCD o valor do A0 e o nome do botão.
- `pHmetro/EXPLICACAO.txt`: explicação de cada parte, em texto simples para iniciantes.
- `pHmetro/LIGACOES.txt`: tabela de onde vai cada fio.
- `antigo_nao_usar/`: cópia do primeiro código, guardada só como histórico. **Não use nem edite.**

Se mudar o código, os pinos ou o funcionamento, atualize também `EXPLICACAO.txt` e `LIGACOES.txt`.

## Hardware
- Placa: **um único Arduino Mega 2560**. Não usamos Arduino Uno, esquema mestre e escravo nem I2C.
- Porta USB do Mega: **COM4** (foi a detectada; pode mudar, então confira com `arduino-cli board list`).
- **LCD Keypad Shield 16x2**, encaixado no Mega:
  - Pinos 8 (RS), 9 (E), 4, 5, 6 e 7 (dados) e A0 (os 5 botões, por divisor de tensão).
  - Limites do A0: <60 RIGHT, <200 UP, <400 DOWN, <600 LEFT, <800 SELECT.
  - **Nunca mexer no pino 10** (luz de fundo; em alguns shields, pode danificar o Arduino).
- **Módulo de pH PH-4502C**:
  - Po -> A8 (sinal do pH), V+ -> 5V, G -> GND. O pino To (temperatura da placa) **não é usado**: o fio To -> A9 saiu e o código não lê o A9.
- **Sonda**: agora é uma **sonda de pH universal com conector BNC (azul)**, **sem sensor de temperatura**. Ela substituiu a E-201-C, que estava em curto.
  - Com a sonda nova, é preciso **apagar a calibração antiga** (SELECT e depois RIGHT) e **recalibrar** com os tampões 4,01 / 6,86 / 9,18. A calibração guardada na EEPROM foi feita com a sonda antiga e não vale para a nova.
  - Quanto maior o pH, menor a tensão (padrão aproximado: pH 7 perto de 2,5 V, cerca de -0,18 V por ponto de pH).
- **4 LEDs**, cada um com resistor de 220 ohm:
  - Pino 22, azul: sistema ligado (aceso fixo).
  - Pino 23, amarelo: pisca 80 ms a cada leitura enquanto varia; aceso fixo quando a leitura está estável (últimas 10 leituras, ou 5 s, variando até 10 mV, cerca de 0,05 pH).
  - Pino 26, vermelho: pisca a cada 250 ms com o pH fora da faixa de 6,0 a 8,0 (desligado durante a calibração).
  - Pino 27, transparente: aceso no modo calibração.
- **Não temos** buzzer nem display OLED.

## Funcionamento
- Modos de leitura: só **pH** e **mV**. UP e DOWN alternam entre os dois. Não há modo de temperatura.
- SELECT entra na calibração. Nela: UP/DOWN escolhe o tampão (4,01 / 6,86 / 9,18), SELECT grava o ponto, RIGHT apaga a calibração e LEFT sai.
- Com 1 ponto, ajusta só o intercepto. Com 2 ou 3, calcula a reta por mínimos quadrados e recusa inclinações fora de -2 a -20 pH/V.
- Ao sair da calibração (LEFT) com 2 ou 3 pontos, o LCD mostra por 3 s a **saúde da sonda**: sensibilidade em mV/pH e, com 3 tampões, a linearidade (R² em %). Com 2 pontos, aparece "Linear: use 3".
- A calibração fica na EEPROM, com um número de assinatura para detectar a primeira execução.
- Lê a cada 500 ms, com média de 20 leituras. Usa `millis()` em vez de `delay()` no programa principal, para não travar o programa (e explique por quê). Botões com detecção de clique e debounce simples.

## Ferramentas
- Use o **arduino-cli** para compilar, gravar e abrir o monitor serial. A placa é `arduino:avr:mega`.
- Use só bibliotecas que já vêm com a IDE (LiquidCrystal e EEPROM).
- Os sketches devem continuar abrindo normalmente na **Arduino IDE**.
- Sistema: Windows, terminal PowerShell.

## Como você deve trabalhar comigo
- Responda sempre em **português**, com linguagem simples.
- **Comente o código explicando cada parte**:
  - Coloque um comentário no topo do arquivo dizendo o que o programa faz.
  - Explique para que serve cada biblioteca incluída (`#include`).
  - Explique cada variável e constante: o que ela guarda e por que tem aquele valor.
  - Explique o que acontece no `setup()` e no `loop()`.
  - Comente cada função: o que ela recebe, o que faz e o que devolve.
  - Explique as linhas que não são óbvias para um iniciante (cálculos, fórmulas de conversão do pH, números "mágicos", pinos usados).
  - Explique também a parte de química quando ela aparecer no código (por exemplo, a calibração do sensor com soluções tampão).
- **Antes de rodar qualquer comando** no terminal, explique em uma frase o que ele faz.
- Sempre diga qual sketch será usado e **confirme comigo antes de compilar ou gravar** na placa.
- Quando der erro, explique o que o erro significa e por que aconteceu, não só como corrigir.
- Prefira código simples e fácil de entender a código "esperto" ou muito curto.
- Diga sempre quais fios ligar em quais pinos quando o código usar um componente novo.
