/**
 ******************************************************************************
 * @file    anemometro_sensor.h
 * @brief   Leitura do sensor óptico (LM393 + disco encoder) via EXTI5,
 *          e janela de tempo de 1s via SysTick.
 ******************************************************************************
 */

#ifndef ANEMOMETRO_SENSOR_H
#define ANEMOMETRO_SENSOR_H

#include <stdint.h>

/**
 * @brief Configura o pino PA5 (D0 do sensor) como entrada e habilita
 *        a interrupção EXTI5 na borda de descida.
 */
void Sensor_Init(void);

/**
 * @brief Configura o SysTick para gerar interrupção a cada 1 ms.
 * @param clock_hz Frequência do clock do sistema em Hz (ex: 8000000 para HSI padrão)
 */
void Sensor_TimerInit(uint32_t clock_hz);

/**
 * @brief Indica se uma nova janela de 1 segundo foi concluída e os
 *        dados estão prontos para serem processados no loop principal.
 * @return 1 se há novo dado pronto, 0 caso contrário
 */
uint8_t Sensor_DadoPronto(void);

/**
 * @brief Retorna o número de pulsos contados no último segundo completo
 *        e limpa a flag de "dado pronto".
 * @return Quantidade de pulsos (furos do encoder) detectados em 1s
 */
uint32_t Sensor_LerPulsosUltimoSegundo(void);

/**
 * @brief Retorna um contador de milissegundos em contagem livre (nunca
 *        reseta), usado como base de tempo para o módulo de LED.
 * @return Milissegundos desde a inicialização do SysTick
 */
uint32_t Sensor_GetMillis(void);

#endif /* ANEMOMETRO_SENSOR_H */