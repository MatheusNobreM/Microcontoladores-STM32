#ifndef UART_H
#define UART_H

#include <stdint.h>

/* ===========================================================================
 * uart - Saida de debug pela USART1 (TX em PA9)
 *
 * Formatacao feita com aritmetica inteira (sem printf/%f), adequada ao
 * STM32F103 que nao possui FPU. Conecte um conversor USB-Serial em PA9.
 * ===========================================================================*/

void UART_Init(void);
void UART_PutChar(char c);
void UART_PutStr(const char *s);
void UART_PutUint(uint32_t v);                  /* inteiro sem sinal */
void UART_PutFixed(float value, uint8_t decimals); /* float -> casas fixas */

#endif /* UART_H */
