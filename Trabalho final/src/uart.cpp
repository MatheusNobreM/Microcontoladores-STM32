#include "uart.h"
#include "config.h"
#include "stm32f1xx.h"

/* BRR (USARTDIV em 1/16): BRR = fck / baud, com arredondamento. */
#define USART_BRR_VALUE(pclk, baud)  (((pclk) + ((baud) / 2U)) / (baud))

void UART_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;     /* GPIOA  */
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;   /* USART1 */

    /* PA9 (USART1_TX) como funcao alternativa push-pull 50 MHz (nibble 0xB).
     * PA9 esta no CRH (pinos 8..15). */
    GPIOA->CRH &= ~(0xFU << ((DBG_TX_PIN - 8U) * 4U));
    GPIOA->CRH |=  (0xBU << ((DBG_TX_PIN - 8U) * 4U));

    USART1->BRR = USART_BRR_VALUE(PCLK2_HZ, DBG_BAUD);
    USART1->CR1 = USART_CR1_TE | USART_CR1_UE;   /* TX e USART habilitados */
}

void UART_PutChar(char c)
{
    while (!(USART1->SR & USART_SR_TXE)) { /* espera buffer de TX livre */ }
    USART1->DR = (uint8_t)c;
}

void UART_PutStr(const char *s)
{
    while (*s) {
        UART_PutChar(*s++);
    }
}

void UART_PutUint(uint32_t v)
{
    char buf[11];                 /* 2^32 cabe em 10 digitos */
    uint8_t n = 0U;
    do {
        buf[n++] = (char)('0' + (v % 10U));
        v /= 10U;
    } while (v);
    while (n) {
        UART_PutChar(buf[--n]);
    }
}

void UART_PutFixed(float value, uint8_t decimals)
{
    if (value < 0.0f) {
        UART_PutChar('-');
        value = -value;
    }

    uint32_t scale = 1U;
    for (uint8_t i = 0U; i < decimals; ++i) {
        scale *= 10U;
    }

    const uint32_t scaled = (uint32_t)(value * (float)scale + 0.5f);
    UART_PutUint(scaled / scale);          /* parte inteira */

    if (decimals == 0U) {
        return;
    }

    UART_PutChar('.');
    uint32_t fp = scaled % scale;          /* parte decimal */
    for (uint8_t i = decimals; i > 0U; --i) {
        const uint32_t d = scale / 10U;
        UART_PutChar((char)('0' + (fp / d)));
        fp %= d;
        scale = d;
    }
}
