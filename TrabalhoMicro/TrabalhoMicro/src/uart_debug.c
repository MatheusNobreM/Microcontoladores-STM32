#include "uart_debug.h"
#include <stm32f103xb.h>

void Uart_Init(void)
{
    /* Habilita o clock para a GPIOA e para a USART1 */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

    /* Configura o PA9 (TX) como Saída de Função Alternativa Push-Pull (AF PP), máx 50MHz */
    GPIOA->CRH &= ~(0xFu << 4);  /* Limpa os bits [7:4] */
    GPIOA->CRH |=  (0xBu << 4);  /* 1011 (0xB): AF Push-Pull, 50 MHz */

    /* Configura o Baud Rate para 9600 bps com o clock de 8 MHz
     * BRR = 8.000.000 / (16 * 9600) = 52.083
     * Hexadecimal: 0x341 */
    USART1->BRR = 0x341;

    /* Habilita a USART1 (UE) e o Transmissor (TE) */
    USART1->CR1 |= USART_CR1_UE | USART_CR1_TE;
}

static void Uart_SendChar(char c)
{
    /* Aguarda o registrador de deslocamento ficar vazio (TXE) */
    while (!(USART1->SR & USART_SR_TXE));
    USART1->DR = c;
}

void Uart_SendString(const char* str)
{
    while (*str)
    {
        Uart_SendChar(*str++);
    }
}