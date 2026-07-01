#include "Anemometro_sensor.h"
#include "stm32f1xx.h"

static volatile uint32_t s_pulsosUltimoSegundo = 0;
static volatile uint8_t  s_dadoPronto          = 0;
static volatile uint32_t s_millis              = 0; 
static uint16_t          s_cntAnterior         = 0; 

void Sensor_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    GPIOA->CRL &= ~(0xFu << 0);
    GPIOA->CRL |=  (0x4u << 0);   /* MODE0=00 (input), CNF0=01 */

    /* --- TIM2 em modo clock externo 2 (conta pulsos na entrada ETR/PA0) --- */
    TIM2->SMCR &= ~(TIM_SMCR_ETP | TIM_SMCR_ECE | TIM_SMCR_ETPS | TIM_SMCR_ETF);

    TIM2->SMCR |= TIM_SMCR_ETP;

    /* Filtro de entrada MÍNIMO: ETF = 0001 -> fSAMPLING = fCK_INT, N = 2. */
    TIM2->SMCR |= (0x1u << TIM_SMCR_ETF_Pos);

    TIM2->SMCR |= TIM_SMCR_ECE;

    TIM2->PSC = 0;                   
    TIM2->ARR = 0xFFFFu;  
    TIM2->CNT = 0;
    s_cntAnterior = 0;

    TIM2->CR1 |= TIM_CR1_CEN; 
}

void Sensor_TimerInit(uint32_t clock_hz)
{
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

void SysTick_Handler(void)
{
    static uint32_t ms_ticks = 0;
    ms_ticks++;
    s_millis++;

    if (ms_ticks >= 1000u)
    {
        uint16_t cntAtual = (uint16_t)TIM2->CNT;
        s_pulsosUltimoSegundo = (uint16_t)(cntAtual - s_cntAnterior);
        s_cntAnterior         = cntAtual;

        s_dadoPronto = 1;
        ms_ticks     = 0;
    }
}
