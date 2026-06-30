/**
 ******************************************************************************
 * @file    anemometro_sensor.c
 * @brief   Implementação da leitura do sensor óptico via EXTI5 e
 *          da janela de tempo de 1s via SysTick.
 ******************************************************************************
 */

#include "Anemometro_sensor.h"
#include "stm32f1xx.h"

/* Variáveis internas do módulo (não expostas no .h) */
static volatile uint32_t s_contadorPulsos      = 0;
static volatile uint32_t s_pulsosUltimoSegundo = 0;
static volatile uint8_t  s_dadoPronto          = 0;
static volatile uint32_t s_millis              = 0; /* Contador livre de ms, usado p/ temporização do LED */

void Sensor_Init(void)
{
    /* Habilita clock de GPIOA e do AFIO (necessário para mapear EXTI -> pino) */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_AFIOEN;

    /* PA5 (D0 do sensor) como Input Floating */
    GPIOA->CRL &= ~(0xFu << 20);
    GPIOA->CRL |=  (0x4u << 20);

    /* Mapeia EXTI5 para a Porta A (EXTICR[1], bits [7:4] = 0000) */
    AFIO->EXTICR[1] &= ~(0xFu << 4);

    /* Habilita a linha EXTI5 e configura disparo na borda de descida */
    EXTI->IMR  |= EXTI_IMR_IM5;
    EXTI->FTSR |= EXTI_FTSR_TR5;

    /* Habilita a interrupção no NVIC (vetor compartilhado EXTI9_5) */
    NVIC_EnableIRQ(EXTI9_5_IRQn);
    NVIC_SetPriority(EXTI9_5_IRQn, 1);
}

void Sensor_TimerInit(uint32_t clock_hz)
{
    /* Configura o SysTick para interromper a cada 1 ms */
    SysTick_Config(clock_hz / 1000u);
    NVIC_SetPriority(SysTick_IRQn, 0); /* Prioridade máxima: a janela de tempo não pode atrasar */
}

uint8_t Sensor_DadoPronto(void)
{
    return s_dadoPronto;
}

uint32_t Sensor_LerPulsosUltimoSegundo(void)
{
    s_dadoPronto = 0;
    return s_pulsosUltimoSegundo;
}

uint32_t Sensor_GetMillis(void)
{
    return s_millis;
}

/* ---------------------------------------------------------------------- */
/* Handlers de interrupção (nomes fixados pelo startup/CMSIS)             */
/* ---------------------------------------------------------------------- */

void EXTI9_5_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR5)
    {
        s_contadorPulsos++;
        EXTI->PR = EXTI_PR_PR5; /* Limpa a flag escrevendo 1 */
    }
}

void SysTick_Handler(void)
{
    static uint32_t ms_ticks = 0;
    ms_ticks++;
    s_millis++;

    if (ms_ticks >= 1000u)
    {
        s_pulsosUltimoSegundo = s_contadorPulsos;
        s_contadorPulsos      = 0;
        s_dadoPronto          = 1;
        ms_ticks              = 0;
    }
}