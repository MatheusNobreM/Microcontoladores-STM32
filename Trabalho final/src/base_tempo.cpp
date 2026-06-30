#include "base_tempo.h"
#include "config.h"
#include "stm32f1xx.h"

/* Sinalizado pela ISR do TIM2, consumido pela aplicacao. */
static volatile uint8_t s_janela_pronta = 0U;

void BaseTempo_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;   /* clock do TIM2 */

    TIM2->PSC  = TIM_PRESCALER;           /* 8 MHz / 8000 = 1 kHz       */
    TIM2->ARR  = TIM_AUTORELOAD;          /* conta JANELA_MS ticks de 1ms */
    TIM2->CNT  = 0U;
    TIM2->SR   = 0U;                      /* limpa flags pendentes      */
    TIM2->DIER |= TIM_DIER_UIE;           /* interrupcao no overflow    */
    TIM2->CR1  |= TIM_CR1_CEN;            /* habilita o contador        */

    NVIC_EnableIRQ(TIM2_IRQn);
}

uint8_t BaseTempo_JanelaPronta(void)
{
    if (s_janela_pronta) {
        s_janela_pronta = 0U;
        return 1U;
    }
    return 0U;
}

/* ISR de fim de janela: apenas sinaliza. */
extern "C" void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR &= ~TIM_SR_UIF;          /* limpa o flag de update */
        s_janela_pronta = 1U;
    }
}
