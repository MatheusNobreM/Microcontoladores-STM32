#ifndef UART_DEBUG_H
#define UART_DEBUG_H

#include <stdint.h>

/**
 * @brief Inicializa a USART1 no pino PA9 (TX) com Baud Rate de 9600.
 *        Considera o clock padrão HSI de 8 MHz.
 */
void Uart_Init(void);

/**
 * @brief Transmite uma string terminada em nulo (uso opcional de debug).
 */
void Uart_SendString(const char* str);

/**
 * @brief Transmite um único byte cru pela UART (bloqueante).
 */
void Uart_SendByte(uint8_t b);

/**
 * @brief Transmite um bloco de bytes crus pela UART.
 * @param dados    Ponteiro para o buffer
 * @param tamanho  Quantidade de bytes a enviar
 */
void Uart_SendBytes(const uint8_t* dados, uint32_t tamanho);

/**
 * @brief Envia um frame binário do anemômetro (9 bytes, sem texto/printf):
 *
 *   [0]=0xAA [1]=0x55 | [2..3]=pulsos LE | [4..5]=rpm LE |
 *   [6..7]=vel_kmh_x100 LE | [8]=checksum (XOR dos bytes [2..7])
 *
 * Todos os campos são uint16 em little-endian (ordem nativa do Cortex-M3).
 * @param pulsos        Pulsos brutos da janela de 1s
 * @param rpm           Rotações por minuto
 * @param vel_kmh_x100  Velocidade em km/h * 100 (ex.: 1234 => 12.34 km/h)
 */
void Uart_EnviarFrame(uint16_t pulsos, uint16_t rpm, uint16_t vel_kmh_x100);

#endif /* UART_DEBUG_H */
