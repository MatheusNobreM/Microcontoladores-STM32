/**
 ******************************************************************************
 * @file    anemometro_sensor.c
 * @brief   Contagem de pulsos do sensor óptico por HARDWARE (TIM2 em modo
 *          clock externo / ETR no pino PA0) e janela de tempo de 1 s via
 *          SysTick.
 *
 *  Por que timer em vez de EXTI?
 *   - O timer conta cada pulso em HARDWARE: zero interrupções por pulso
 *     (a EXTI antiga executava a ISR a cada furo).
 *   - Elimina o bug de corrida do reset: a ISR do SysTick (prioridade 0)
 *     podia preemptar o "s_contadorPulsos++" da EXTI (prioridade 1) no meio
 *     do read-modify-write e perder o zeramento. Agora o SysTick só LÊ um
 *     registrador de hardware (TIM2->CNT), que é atômico, e calcula o delta.
 *
 *  IMPORTANTE (hardware): mover o fio D0 do sensor de PA5 para PA0.
 *  Se a contagem cair/zerar em alta rotação, o problema é a subida lenta da
 *  saída coletor-aberto do LM393: adicione um pull-up de ~1 kΩ de D0 p/ 3V3.
 *  Nenhum ajuste de firmware recupera bordas que o sinal analógico não gera.
 ******************************************************************************
 */

#include "Anemometro_sensor.h"
#include "stm32f1xx.h"

/* Variáveis internas do módulo (não expostas no .h) */
static volatile uint32_t s_pulsosUltimoSegundo = 0;
static volatile uint8_t  s_dadoPronto          = 0;
static volatile uint32_t s_millis              = 0; /* Contador livre de ms, base de tempo do LED */
static uint16_t          s_cntAnterior         = 0; /* Última leitura do TIM2->CNT (p/ delta) */

void Sensor_Init(void)
{
    /* Clocks: GPIOA (PA0) e TIM2 */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    /* PA0 (ETR / entrada de contagem) como Input Floating.
     * O sensor já tem pull-up no módulo; se precisar acelerar a subida em
     * alta rotação, use um pull-up externo mais forte (~1 kΩ) no hardware. */
    GPIOA->CRL &= ~(0xFu << 0);
    GPIOA->CRL |=  (0x4u << 0);   /* MODE0=00 (input), CNF0=01 (floating) */

    /* --- TIM2 em modo clock externo 2 (conta pulsos na entrada ETR/PA0) --- */
    TIM2->SMCR &= ~(TIM_SMCR_ETP | TIM_SMCR_ECE | TIM_SMCR_ETPS | TIM_SMCR_ETF);

    /* ETP = 1: conta na borda de DESCIDA (idêntico à EXTI antiga: sensor
     * ocioso em nível alto, cai a cada furo). */
    TIM2->SMCR |= TIM_SMCR_ETP;

    /* Filtro de entrada MÍNIMO: ETF = 0001 -> fSAMPLING = fCK_INT, N = 2.
     * Rejeita apenas glitches sub-microssegundo. NÃO aumente muito: filtro
     * pesado descarta pulsos curtos de alta rotação (justamente o sintoma de
     * "para de contar em alta velocidade"). */
    TIM2->SMCR |= (0x1u << TIM_SMCR_ETF_Pos);

    /* ETPS = 00: sem prescaler no ETR (conta TODOS os pulsos). */

    /* ECE = 1: habilita o modo clock externo 2 (o contador avança a cada
     * pulso filtrado do ETR). */
    TIM2->SMCR |= TIM_SMCR_ECE;

    TIM2->PSC = 0;        /* Sem divisão: 1 contagem por pulso                 */
    TIM2->ARR = 0xFFFFu;  /* Contador livre de 16 bits (o delta trata o wrap)  */
    TIM2->CNT = 0;
    s_cntAnterior = 0;

    TIM2->CR1 |= TIM_CR1_CEN; /* Liga o contador */
}

void Sensor_TimerInit(uint32_t clock_hz)
{
    /* Configura o SysTick para interromper a cada 1 ms (base de tempo/janela) */
    SysTick_Config(clock_hz / 1000u);
    NVIC_SetPriority(SysTick_IRQn, 0);
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
/* Handler de interrupção (nome fixado pelo startup/CMSIS)                */
/* ---------------------------------------------------------------------- */

void SysTick_Handler(void)
{
    static uint32_t ms_ticks = 0;
    ms_ticks++;
    s_millis++;

    if (ms_ticks >= 1000u)
    {
        /* Lê o contador de hardware e calcula o delta da janela.
         * A subtração em uint16 trata o "wrap" de 0xFFFF -> 0 sozinha,
         * então o contador nunca precisa ser zerado (sem corrida). */
        uint16_t cntAtual = (uint16_t)TIM2->CNT;
        s_pulsosUltimoSegundo = (uint16_t)(cntAtual - s_cntAnterior);
        s_cntAnterior         = cntAtual;

        s_dadoPronto = 1;
        ms_ticks     = 0;
    }
}
