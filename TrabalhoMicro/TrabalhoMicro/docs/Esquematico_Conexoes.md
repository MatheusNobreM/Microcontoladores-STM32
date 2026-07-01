# Esquemático de Conexões — Anemômetro com STM32 Blue Pill

Documento de ligações entre o **sensor óptico do anemômetro** e o
microcontrolador **STM32F103C8 (Blue Pill)**.

- **Microcontrolador:** STM32F103C8T6 (Blue Pill), clock HSI de 8 MHz
- **Sensor:** módulo óptico LM393 + disco encoder (5 furos / PPR)
- **Contagem:** TIM2 em modo clock externo (ETR no pino PA0), janela de 1 s via SysTick
- **Gravação/Debug:** ST-Link V2 pela interface SWD

---

## 1. Tabela de conexões

### Sensor óptico (LM393) → Blue Pill

| Pino do sensor | Pino do MCU | Função                                        |
| -------------- | ----------- | --------------------------------------------- |
| `VCC`          | `3V3`       | Alimentação do módulo (3,3 V)                 |
| `GND`          | `GND`       | Referência comum                              |
| `D0`           | `PA0`       | Saída digital → entrada de contagem TIM2_ETR  |

> **Observação (hardware):** a saída `D0` do LM393 é coletor-aberto.
> O próprio módulo tem pull-up, mas se a contagem cair em alta rotação,
> adicione um pull-up externo de **~1 kΩ** de `D0` para `3V3` para acelerar
> a borda de subida. A contagem ocorre na **borda de descida** (sensor
> ocioso em nível alto).

### LED de feedback (indicação da velocidade)

| Sinal        | Pino do MCU | Função                                             |
| ------------ | ----------- | -------------------------------------------------- |
| LED onboard  | `PC13`      | Pisca proporcional à velocidade (ativo em nível baixo) |

> O LED já está integrado na placa Blue Pill (não precisa de fiação externa).

### Saída de dados — UART (conversor USB-TTL)

| Pino do MCU | Conversor USB-TTL | Função                                   |
| ----------- | ----------------- | ---------------------------------------- |
| `PA9`       | `RX`              | USART1 TX → transmite o frame binário    |
| `GND`       | `GND`             | Referência comum                         |

> USART1 configurada em **9600 bps, 8N1**. Apenas transmissão (TX);
> o pino RX (PA10) não é usado.

### Gravação / Depuração — ST-Link V2 (SWD)

| ST-Link | Pino do MCU | Função            |
| ------- | ----------- | ----------------- |
| `SWDIO` | `PA13`      | Dado SWD          |
| `SWCLK` | `PA14`      | Clock SWD         |
| `3V3`   | `3V3`       | Alimentação       |
| `GND`   | `GND`       | Referência comum  |

---

## 2. Diagrama esquemático (ASCII)

```
                 Sensor óptico                     STM32F103C8 (Blue Pill)
              (LM393 + encoder)
             +-----------------+                  +-----------------------+
   3,3V ---->| VCC             |                  |                       |
             |                 |                  |                       |
             |             D0  |----[~1k opc.]----| PA0  (TIM2_ETR)       |
             |                 |         |        |                       |
             |             GND |----+    +--3V3---| 3V3                   |
             +-----------------+    |             |                       |
                                    +-------------| GND                   |
                                                  |                       |
                                                  | PC13 --> LED onboard  |
                                                  |                       |
     Conversor USB-TTL                            |                       |
     +----------------+                           |                       |
     |  RX            |<--------------------------| PA9  (USART1 TX)       |
     |  GND           |<----------------+---------| GND                   |
     +----------------+                 |         |                       |
                                        |         |                       |
     ST-Link V2 (SWD)                   |         |                       |
     +----------------+                 |         |                       |
     | SWDIO          |------------------ ------->| PA13 (SWDIO)          |
     | SWCLK          |-------------------------->| PA14 (SWCLK)          |
     | 3V3            |----------------->3V3 ------| 3V3                  |
     | GND            |-----------------+          | GND                  |
     +----------------+                            +-----------------------+
```

> **GND comum:** sensor, conversor USB-TTL, ST-Link e a Blue Pill precisam
> compartilhar o mesmo terra (GND).

---

## 3. Fluxo do sinal

```
Vento -> hélice gira -> disco encoder (5 furos) interrompe o feixe óptico
      -> LM393 gera pulsos digitais em D0
      -> PA0 / TIM2_ETR conta os pulsos por HARDWARE
      -> SysTick fecha a janela de 1 s e calcula pulsos/s
      -> Anemometro_Calcular() -> RPM e velocidade (km/h)
      -> PC13 pisca conforme a velocidade
      -> USART1 (PA9) envia o frame binário pelo cabo TTL
```

Frame enviado pela UART (9 bytes): `AA 55 | pulsos(16) | rpm(16) | vel_kmh*100(16) | checksum(XOR)`.
