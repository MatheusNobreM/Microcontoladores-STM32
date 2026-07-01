#ifndef ANEMOMETRO_SENSOR_H
#define ANEMOMETRO_SENSOR_H

#include <stdint.h>

void Sensor_Init(void);

void Sensor_TimerInit(uint32_t clock_hz);

uint8_t Sensor_DadoPronto(void);

uint32_t Sensor_LerPulsosUltimoSegundo(void);

uint32_t Sensor_GetMillis(void);

#endif 