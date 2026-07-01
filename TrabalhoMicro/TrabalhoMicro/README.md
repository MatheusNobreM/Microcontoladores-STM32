# Anemômetro com STM32 Blue Pill

Anemômetro digital baseado em **STM32F103C8 (Blue Pill)** que conta os pulsos
de um disco encoder óptico (LM393), calcula **RPM** e **velocidade do vento
(km/h)** em ponto fixo (sem `float`) e envia os dados por **UART/TTL**. No PC,
scripts Python recebem os frames, gravam em `JSON Lines` e geram gráficos.

```
Vento -> hélice/disco encoder (5 furos) -> LM393 gera pulsos
      -> PA0 / TIM2 conta por hardware -> janela de 1 s (SysTick)
      -> RPM + velocidade (km/h) -> UART (PA9) -> Python (receptor + gráficos)
```

---

## 1. Requisitos

**Hardware**
- STM32F103C8T6 (Blue Pill)
- Módulo sensor óptico LM393 + disco encoder de **5 furos**
- Gravador **ST-Link V2** (SWD)
- Conversor **USB–TTL** (para receber os dados no PC)
- (Opcional) resistor de **~1 kΩ** como pull-up em `D0` (ver seção 6)

**Software**
- [PlatformIO](https://platformio.org/) (extensão do VS Code ou CLI) — compila e grava o firmware
- **Python 3.8+** — recebe e plota os dados

---

## 2. Ligações (resumo)

| Sensor (LM393) | Blue Pill | Função                          |
| -------------- | --------- | ------------------------------- |
| `VCC`          | `3V3`     | Alimentação (3,3 V)             |
| `GND`          | `GND`     | Terra comum                     |
| `D0`           | `PA0`     | Pulsos → contagem por TIM2_ETR  |

| Blue Pill | Destino          | Função                        |
| --------- | ---------------- | ----------------------------- |
| `PA9`     | `RX` do USB–TTL  | USART1 TX (frame binário)     |
| `GND`     | `GND` do USB–TTL | Terra comum                   |
| `PC13`    | LED onboard      | Pisca conforme a velocidade   |

> Esquemático completo (com ST-Link e diagrama ASCII):
> [docs/Esquematico_Conexoes.md](docs/Esquematico_Conexoes.md).
> **Todos os GNDs (sensor, USB–TTL, ST-Link, Blue Pill) devem ser comuns.**

---

## 3. Compilar e gravar o firmware (PlatformIO)

O projeto usa `framework = cmsis`, `board = bluepill_f103c8` e grava via
`stlink` (ver [platformio.ini](platformio.ini)).

**Pela CLI:**

```bash
# na raiz do projeto
pio run                 # compila
pio run -t upload       # compila e grava via ST-Link
pio run -t clean        # limpa artefatos de build
```

**Pelo VS Code:** com a extensão PlatformIO instalada, use a barra inferior
(✓ *Build* e → *Upload*) ou a árvore *Project Tasks → bluepill_f103c8*.

> Se `pio` não estiver no PATH, ele fica em
> `~/.platformio/penv/Scripts/pio` (Windows) ou use o terminal do próprio
> PlatformIO no VS Code.

---

## 4. Rodar o lado Python

Os scripts ficam em [tools/](tools/).

### 4.1. Instalar as dependências

```bash
cd tools
python -m venv .venv

# Ativar o ambiente virtual:
#   Windows (PowerShell):
.venv\Scripts\Activate.ps1
#   Windows (cmd):
#   .venv\Scripts\activate.bat
#   Linux/macOS:
#   source .venv/bin/activate

pip install -r requirements.txt   # pyserial + matplotlib
```

### 4.2. Receber e gravar os dados — `receptor.py`

Com a Blue Pill ligada e o USB–TTL conectado, descubra a porta serial
(ex.: `COM3` no Windows, `/dev/ttyUSB0` no Linux) e rode:

```bash
python receptor.py COM3 9600
```

- Lê os frames binários (9 bytes, 9600 bps, 8N1), valida o *checksum* e
  **acrescenta** cada leitura em `dados_anemometro.jsonl` (uma leitura por linha).
- Mostra no terminal `pulsos`, `rpm` e `vel (km/h)` a cada segundo.
- `Ctrl+C` encerra.

> Sem argumentos, assume `COM3` e `9600`. Se der erro de porta, confira o
> gerenciador de dispositivos / `ls /dev/ttyUSB*` e ajuste.

### 4.3. Gerar os gráficos — `graficos.py`

Depois de coletar dados (com o `receptor.py`):

```bash
python graficos.py                      # usa dados_anemometro.jsonl -> grafico_anemometro.png
python graficos.py dados.jsonl saida.png
python graficos.py --completo           # mostra a medição inteira (sem recorte)
```

Gera 3 gráficos ao longo do tempo: **velocidade (km/h)**, **RPM** e
**pulsos/s** (com eixo secundário em voltas/s; 5 furos = 1 volta). Por padrão
ele foca na região com movimento; use `--completo` para ver tudo.

---

## 5. Formato do frame (UART)

9 bytes, *little-endian*:

| Byte(s) | Conteúdo            |
| ------- | ------------------- |
| `0`     | `0xAA` (sync 0)     |
| `1`     | `0x55` (sync 1)     |
| `2–3`   | `pulsos` (uint16)   |
| `4–5`   | `rpm` (uint16)      |
| `6–7`   | `vel_kmh × 100` (uint16) |
| `8`     | checksum = XOR dos bytes `2–7` |

---

## 6. Calibração e parâmetros

Ajustáveis em [include/Anemometro_calculo.h](include/Anemometro_calculo.h):

| `#define`                | Valor | Significado                                   |
| ------------------------ | ----- | --------------------------------------------- |
| `ANEM_N_FUROS`           | `5`   | Furos do disco encoder (pulsos por volta)     |
| `ANEM_FATOR_KMH_X1000`   | `23`  | Velocidade = `pulsos/s × 0,023 km/h` (x1000)  |

- **RPM** = `pulsos/s × 60 / N_FUROS` (com 5 furos → `× 12`).
- **Velocidade**: reta pela origem `V_kmh = FATOR × pulsos/s` (0 pulso → 0 km/h),
  ancorada na referência de **7,9 m/s** do anemômetro comercial.
- **Reancorar o fator:** `FATOR_x1000 = round(1000 × V_ref_kmh / pulsos_no_ref)`.
  Recompile e regrave após alterar.

> **Sensor "morrendo" em alta rotação?** É a subida lenta da saída
> coletor-aberto do LM393. Adicione um pull-up de **~1 kΩ** (até 470 Ω) de
> `D0` para `3V3` e confirme que está lendo a saída **D0** (digital), não `A0`.

---

## 7. Estrutura do projeto

```
├── platformio.ini              # configuração de build/upload
├── include/                    # cabeçalhos dos módulos
│   ├── Anemometro_sensor.h     #  contagem por TIM2 + janela SysTick
│   ├── Anemometro_calculo.h    #  RPM e velocidade (calibração)
│   ├── Led_feedback.h          #  LED proporcional à velocidade
│   └── uart_debug.h            #  envio do frame binário
├── src/                        # implementações (.c) + main.c
├── tools/                      # lado PC (Python)
│   ├── receptor.py             #  recebe UART -> dados_anemometro.jsonl
│   ├── graficos.py             #  gera os gráficos
│   └── requirements.txt        #  pyserial, matplotlib
└── docs/
    └── Esquematico_Conexoes.md # ligações detalhadas
```

---

## 8. Passo a passo rápido

1. Montar as ligações (seção 2 / esquemático).
2. `pio run -t upload` — grava o firmware.
3. `cd tools` → criar venv → `pip install -r requirements.txt`.
4. `python receptor.py COM3 9600` — começa a gravar os dados.
5. Ligar o vento; acompanhar `pulsos/rpm/vel` no terminal.
6. `python graficos.py` — gerar os gráficos da medição.
