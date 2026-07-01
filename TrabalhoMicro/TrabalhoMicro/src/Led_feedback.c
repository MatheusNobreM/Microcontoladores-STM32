#include "led_feedback.h"
#include "stm32f1xx.h"

#define LED_PERIODO_MAX_MS   1000u  /* Vento ~0 km/h  -> pisca devagar (1 Hz)   */
#define LED_PERIODO_MIN_MS   50u    /* Vento forte    -> pisca rápido (limite)  */

static volatile uint32_t s_velKmhX1000     = 0;   /* km/h * 1000 */
static uint32_t          s_ultimoToggleMs  = 0;
static uint8_t           s_ledAceso        = 0;

void Led_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    GPIOC->CRH &= ~(0xFu << 20);
    GPIOC->CRH |=  (0x2u << 20);


    GPIOC->ODR |= (1u << 13);
    s_ledAceso = 0;
}

void Led_SetVelocidade(uint32_t velocidade_kmh_x1000)
{
    s_velKmhX1000 = velocidade_kmh_x1000;
}

void Led_Atualizar(uint32_t millis_atual)
{
    /* velocidade, menor o período. Equivale a MAX / (1 + v_kmh), pois:
     *   MAX / (1 + v)  =  (MAX * 1000) / (1000 + v*1000)
     * O numerador (1000 * 1000 = 1.000.000) cabe folgado em uint32. */
    uint32_t periodo_ms = (LED_PERIODO_MAX_MS * 1000u) / (1000u + s_velKmhX1000);

    if (periodo_ms < LED_PERIODO_MIN_MS)
    {
        periodo_ms = LED_PERIODO_MIN_MS;
    }

    if ((millis_atual - s_ultimoToggleMs) >= periodo_ms)
    {
        GPIOC->ODR ^= (1u << 13); 
        s_ledAceso       = !s_ledAceso;
        s_ultimoToggleMs = millis_atual;
    }
}