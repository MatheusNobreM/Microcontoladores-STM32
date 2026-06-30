#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/* ===========================================================================
 * config.h - Parametros de hardware e do modelo (centralizados)
 * Alvo : STM32F103C8 "Bluepill"
 * Sensor: LM393 + optoacoplador (modulo tipo FC-03 / sensor de velocidade)
 *
 * Ligacao do modulo (4 pinos):
 *   VCC -> 3V3 (ou 5V tolerante)        GND -> GND
 *   D0  -> PA0  : saida DIGITAL do comparador LM393
 *                 -> usada como INTERRUPCAO EXTERNA (EXTI0) p/ contar a helice
 *   A0  -> PA1  : saida ANALOGICA (nivel do feixe) -> lida pelo ADC1_IN1
 * ===========================================================================*/

/* ---- Sensor: ambos os sinais ficam no GPIOA ---- */
#define SENSOR_PORT           GPIOA

/* D0 -> PA0 : gera a interrupcao de contagem (linha EXTI0) */
#define SENSOR_D0_PIN         0U
#define SENSOR_D0_EXTI_LINE   0U

/* A0 -> PA1 : entrada analogica do ADC1 (canal 1) */
#define SENSOR_A0_PIN         1U
#define SENSOR_A0_ADC_CH      1U

/* ---- Helice ----
 * Quantas vezes o feixe e' cortado a cada volta completa.
 * Helice de N pas (ou disco com N fendas) -> N pulsos por volta. */
#define PULSOS_POR_VOLTA      2U

/* ---- Base de tempo: janela de contagem (TIM2) ---- */
#define TIM_CLK_HZ            8000000U   /* TIM2_CLK = 8 MHz (Bluepill default) */
#define JANELA_MS             1000U      /* mede a cada 1 s                      */
#define TIM_PRESCALER         7999U      /* 8 MHz / 8000 = 1 kHz (tick de 1 ms)  */
#define TIM_AUTORELOAD        ((JANELA_MS) - 1U)

/* ---- UART de debug (USART1) ---- */
#define DBG_TX_PIN            9U         /* PA9 = USART1_TX */
#define DBG_BAUD             115200U
#define PCLK2_HZ             8000000U    /* HSI 8 MHz (sem reconfig de clock) */

#endif /* CONFIG_H */
