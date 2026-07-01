#ifndef LED_FEEDBACK_H
#define LED_FEEDBACK_H

#include <stdint.h>

void Led_Init(void);

void Led_Atualizar(uint32_t millis_atual);

void Led_SetVelocidade(uint32_t velocidade_kmh_x1000);

#endif 