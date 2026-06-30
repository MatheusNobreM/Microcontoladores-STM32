#include "sensor_helice.h"
#include "config.h"
#include "stm32f1xx.h"

/* ---------------------------------------------------------------------------
 * Estado compartilhado: escrito SO pela ISR, lido pela aplicacao.
 * 'volatile' impede que o compilador "cacheie" o valor fora da ISR.
 * -------------------------------------------------------------------------*/
static volatile uint32_t s_contagem = 0U;

/* ---------------------------------------------------------------------------
 * D0 (PA0) como entrada digital + interrupcao externa (EXTI0)
 * -------------------------------------------------------------------------*/
static void D0_InitEXTI(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;   /* clock do GPIOA            */
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;   /* clock do AFIO (mux EXTI)  */

    /* PA0 como entrada flutuante (MODE=00, CNF=01 -> nibble 0x4).
     * A saida do LM393 e' push-pull, dispensa pull-up/down interno. */
    SENSOR_PORT->CRL &= ~(0xFU << (SENSOR_D0_PIN * 4U));
    SENSOR_PORT->CRL |=  (0x4U << (SENSOR_D0_PIN * 4U));

    /* Conecta a linha EXTI0 ao PORTA (campo de 4 bits = 0 -> PA). */
    AFIO->EXTICR[SENSOR_D0_EXTI_LINE / 4U] &=
        ~(0xFU << ((SENSOR_D0_EXTI_LINE % 4U) * 4U));

    /* Dispara na borda de subida (feixe cortado -> D0 vai a nivel ALTO). */
    EXTI->RTSR |= (1U << SENSOR_D0_EXTI_LINE);
    EXTI->IMR  |= (1U << SENSOR_D0_EXTI_LINE);   /* desmascara a linha */

    NVIC_EnableIRQ(EXTI0_IRQn);
}

/* ---------------------------------------------------------------------------
 * A0 (PA1) como entrada analogica do ADC1 (canal SENSOR_A0_ADC_CH)
 * -------------------------------------------------------------------------*/
static void A0_InitADC(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;   /* clock do GPIOA */
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;   /* clock do ADC1  */

    /* PA1 como entrada analogica (MODE=00, CNF=00 -> nibble 0x0). */
    SENSOR_PORT->CRL &= ~(0xFU << (SENSOR_A0_PIN * 4U));

    /* Sequencia regular com 1 conversao do canal de A0. */
    ADC1->SQR1  = 0U;                                      /* L = 0 (1 conv.) */
    ADC1->SQR3  = (SENSOR_A0_ADC_CH << ADC_SQR3_SQ1_Pos);
    ADC1->SMPR2 = (7U << (SENSOR_A0_ADC_CH * 3U));         /* 239,5 ciclos    */

    /* Disparo por software (EXTSEL=111=SWSTART) habilitado. */
    ADC1->CR2 |= (7U << ADC_CR2_EXTSEL_Pos) | ADC_CR2_EXTTRIG;

    /* Liga o ADC e executa a calibracao (melhora a precisao). */
    ADC1->CR2 |= ADC_CR2_ADON;
    ADC1->CR2 |= ADC_CR2_RSTCAL;  while (ADC1->CR2 & ADC_CR2_RSTCAL) { }
    ADC1->CR2 |= ADC_CR2_CAL;     while (ADC1->CR2 & ADC_CR2_CAL)    { }
}

void SensorHelice_Init(void)
{
    D0_InitEXTI();
    A0_InitADC();
}

uint32_t SensorHelice_LerContagem(void)
{
    return s_contagem;
}

uint32_t SensorHelice_PegarEZerar(void)
{
    __disable_irq();
    const uint32_t n = s_contagem;
    s_contagem = 0U;
    __enable_irq();
    return n;
}

void SensorHelice_Zerar(void)
{
    __disable_irq();
    s_contagem = 0U;
    __enable_irq();
}

uint16_t SensorHelice_LerAnalogico(void)
{
    ADC1->SR &= ~ADC_SR_EOC;            /* garante flag de fim limpa */
    ADC1->CR2 |= ADC_CR2_SWSTART;       /* dispara 1 conversao       */
    while (!(ADC1->SR & ADC_SR_EOC)) { }
    return (uint16_t)ADC1->DR;          /* a leitura limpa o EOC     */
}

/* ---------------------------------------------------------------------------
 * ISR do D0: cada corte do feixe gera +1. Nada de calculo pesado aqui.
 * (extern "C" para casar com o nome do vetor de interrupcao do startup.)
 * -------------------------------------------------------------------------*/
extern "C" void EXTI0_IRQHandler(void)
{
    if (EXTI->PR & (1U << SENSOR_D0_EXTI_LINE)) {
        EXTI->PR = (1U << SENSOR_D0_EXTI_LINE);   /* limpa escrevendo 1 */
        s_contagem++;
    }
}
