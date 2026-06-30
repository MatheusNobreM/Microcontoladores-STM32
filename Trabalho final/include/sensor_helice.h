#ifndef SENSOR_HELICE_H
#define SENSOR_HELICE_H

#include <stdint.h>

/* ===========================================================================
 * sensor_helice - Driver do sensor LM393 + optoacoplador
 *
 *   D0 (PA0) : saida digital -> INTERRUPCAO EXTERNA (EXTI0).
 *              A cada corte do feixe pela helice, a ISR incrementa a contagem.
 *   A0 (PA1) : saida analogica -> lida sob demanda pelo ADC1 (monitoramento).
 *
 * Toda a contagem acontece por interrupcao; a aplicacao apenas le/zera.
 * ===========================================================================*/

/* Configura D0 (EXTI) e A0 (ADC). Chamar uma vez no boot. */
void SensorHelice_Init(void);

/* Le a contagem atual de pulsos sem altera-la. */
uint32_t SensorHelice_LerContagem(void);

/* Le e zera a contagem de forma atomica (sem perder pulsos). */
uint32_t SensorHelice_PegarEZerar(void);

/* Zera a contagem. */
void SensorHelice_Zerar(void);

/* Le a saida analogica A0 via ADC1 (resultado de 12 bits: 0..4095). */
uint16_t SensorHelice_LerAnalogico(void);

#endif /* SENSOR_HELICE_H */
