#ifndef UART_DEBUG_H
#define UART_DEBUG_H

#include <stdint.h>

void Uart_Init(void);

void Uart_SendString(const char* str);

void Uart_SendByte(uint8_t b);

void Uart_SendBytes(const uint8_t* dados, uint32_t tamanho);

void Uart_EnviarFrame(uint16_t pulsos, uint16_t rpm, uint16_t vel_kmh_x100);

#endif 