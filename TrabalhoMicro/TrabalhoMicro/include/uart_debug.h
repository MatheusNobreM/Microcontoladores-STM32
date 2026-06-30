#ifndef UART_DEBUG_H
#define UART_DEBUG_H

/**
 * @brief Inicializa a USART1 no pino PA9 (TX) com Baud Rate de 9600.
 * Considera o clock padrão HSI de 8 MHz.
 */
void Uart_Init(void);

/**
 * @brief Transmite uma string terminada em nulo (C-string) via UART.
 * @param str Ponteiro para a string
 */
void Uart_SendString(const char* str);

#endif /* UART_DEBUG_H */