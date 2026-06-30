/**
 ******************************************************************************
 * @file    led_feedback.c
 * @brief   Implementação do piscar do LED proporcional à velocidade do vento.
 ******************************************************************************
 */

#include "led_feedback.h"
#include "stm32f1xx.h"

/* ---------------------------------------------------------------------- */
/* Parâmetros de mapeamento velocidade -> intervalo de piscar (AJUSTAR)   */
/* ---------------------------------------------------------------------- */
#define LED_PERIODO_MAX_MS   1000u  /* Vento ~0 km/h  -> pisca devagar (1 Hz)   */
#define LED_PERIODO_MIN_MS   50u    /* Vento forte    -> pisca rápido (limite)  */

static volatile float    s_velocidadeKmh   = 0.0f;
static uint32_t          s_ultimoToggleMs  = 0;
static uint8_t           s_ledAceso        = 0;

void Led_Init(void)
{
    /* Habilita clock da GPIOC */
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    /* PC13 como saída push-pull, máx 2 MHz */
    GPIOC->CRH &= ~(0xFu << 20);
    GPIOC->CRH |=  (0x2u << 20);

    /* LED apagado inicialmente (ativo em nível baixo na Blue Pill) */
    GPIOC->ODR |= (1u << 13);
    s_ledAceso = 0;
}

void Led_SetVelocidade(float velocidade_kmh)
{
    s_velocidadeKmh = velocidade_kmh;
}

void Led_Atualizar(uint32_t millis_atual)
{
    /* Calcula o intervalo de piscar: quanto maior a velocidade, menor o período.
     * Mapeamento simples e provisório (1 / (1 + v)) - recalibrar após os testes
     * de campo, junto com o FATOR_K do módulo de cálculo. */
    float periodo_f = (float)LED_PERIODO_MAX_MS / (1.0f + s_velocidadeKmh);

    if (periodo_f < (float)LED_PERIODO_MIN_MS)
    {
        periodo_f = (float)LED_PERIODO_MIN_MS;
    }

    uint32_t periodo_ms = (uint32_t)periodo_f;

    if ((millis_atual - s_ultimoToggleMs) >= periodo_ms)
    {
        GPIOC->ODR ^= (1u << 13); /* Alterna o estado do LED */
        s_ledAceso       = !s_ledAceso;
        s_ultimoToggleMs = millis_atual;
    }
}