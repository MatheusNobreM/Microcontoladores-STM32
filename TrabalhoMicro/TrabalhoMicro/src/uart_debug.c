#include "uart_debug.h"
#include <stm32f103xb.h>

#define FRAME_SYNC0  0xAAu
#define FRAME_SYNC1  0x55u

void Uart_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;


    GPIOA->CRH &= ~(0xFu << 4);  /* Limpa os bits [7:4] */
    GPIOA->CRH |=  (0xBu << 4);  /* 1011 (0xB): AF Push-Pull, 50 MHz */

    USART1->BRR = 0x341;

    USART1->CR1 |= USART_CR1_UE | USART_CR1_TE;
}

void Uart_SendByte(uint8_t b)
{
    while (!(USART1->SR & USART_SR_TXE));
    USART1->DR = b;
}

void Uart_SendBytes(const uint8_t* dados, uint32_t tamanho)
{
    while (tamanho--)
    {
        Uart_SendByte(*dados++);
    }
}

void Uart_SendString(const char* str)
{
    while (*str)
    {
        Uart_SendByte((uint8_t)*str++);
    }
}

void Uart_EnviarFrame(uint16_t pulsos, uint16_t rpm, uint16_t vel_kmh_x100)
{
    uint8_t frame[9];

    frame[0] = FRAME_SYNC0;
    frame[1] = FRAME_SYNC1;
    frame[2] = (uint8_t)(pulsos       & 0xFFu);
    frame[3] = (uint8_t)(pulsos >> 8);
    frame[4] = (uint8_t)(rpm          & 0xFFu);
    frame[5] = (uint8_t)(rpm >> 8);
    frame[6] = (uint8_t)(vel_kmh_x100 & 0xFFu);
    frame[7] = (uint8_t)(vel_kmh_x100 >> 8);

    /* Checksum: XOR de todos os bytes de dados (payload) */
    frame[8] = frame[2] ^ frame[3] ^ frame[4] ^ frame[5] ^ frame[6] ^ frame[7];

    Uart_SendBytes(frame, sizeof(frame));
}
