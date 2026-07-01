/**
 ******************************************************************************
 * @file    led_feedback.h
 * @brief   Pisca o LED onboard (PC13) em uma cadência proporcional à
 *          velocidade do vento medida (vento forte = pisca rápido).
 ******************************************************************************
 */

#ifndef LED_FEEDBACK_H
#define LED_FEEDBACK_H

#include <stdint.h>

/**
 * @brief Configura o pino PC13 (LED onboard da Blue Pill) como saída.
 */
void Led_Init(void);

/**
 * @brief Atualiza o estado do LED de acordo com o tempo decorrido (não
 *        bloqueante). Deve ser chamada continuamente dentro do loop
 *        principal (while(1)).
 * @param millis_atual   Valor atual do contador de millis (Sensor_GetMillis())
 */
void Led_Atualizar(uint32_t millis_atual);

/**
 * @brief Define a velocidade atual do vento, usada para calcular o
 *        intervalo de piscar do LED (quanto maior a velocidade, menor o
 *        intervalo entre piscadas). Recebe o valor em ponto fixo (sem float).
 * @param velocidade_kmh_x1000 Velocidade do vento em km/h * 1000
 */
void Led_SetVelocidade(uint32_t velocidade_kmh_x1000);

#endif /* LED_FEEDBACK_H */