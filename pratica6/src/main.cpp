#include "stm32f1xx.h"

/* ===========================================================================
 * Pratica 6 - LED RGB controlado por 3 potenciometros (ADC + DMA + PWM)
 *
 * Objetivo: usar 3 canais analogicos distintos para controlar a intensidade
 * do brilho das cores primarias (R, G, B) de um LED RGB.
 *
 * Estrategia (conforme as sugestoes / exemplos da pasta):
 *   - PWM controla o brilho de cada cor (BM_TIM2_PWM, BM_ADC_*).
 *   - O conversor A/D le os 3 potenciometros e o resultado ajusta a razao
 *     ciclica (duty cycle) de cada canal de PWM (BM_ADC_1 / BM_ADC_2).
 *   - O ADC trabalha em modo SCAN + CONTINUO e o DMA transfere automaticamente
 *     cada resultado para um vetor na memoria (EX_DMA). Assim as 3 leituras
 *     sao feitas sozinhas, sem CPU e sem espera por EOC.
 *
 * Potenciometros (entradas analogicas):
 *   PA0 -> ADC1_IN0 -> controla Vermelho (R)
 *   PA1 -> ADC1_IN1 -> controla Verde    (G)
 *   PA2 -> ADC1_IN2 -> controla Azul     (B)
 *
 * LED RGB (saidas de PWM, mesma pinagem da pratica anterior):
 *   PB5 -> Vermelho (R) -> TIM3_CH2 (partial remap do TIM3 via AFIO)
 *   PB1 -> Verde    (G) -> TIM3_CH4
 *   PB6 -> Azul     (B) -> TIM4_CH1
 *
 * Clock: HSI = 8 MHz. ADCPRE = /2 (default) -> ADC_CLK = 4 MHz (< 14 MHz OK).
 * ===========================================================================
 */

// Resolucao do PWM = ARR dos timers. Usamos 4095 para casar com os 12 bits do
// ADC -> o resultado da conversao (0..4095) vira a razao ciclica diretamente.
static constexpr uint16_t PWM_MAX = 4095U;

// Numero de canais analogicos / potenciometros.
static constexpr uint32_t NUM_CH = 3U;

// Vetor preenchido automaticamente pelo DMA com as 3 leituras do ADC.
// [0] = PA0 (R), [1] = PA1 (G), [2] = PA2 (B).
static volatile uint16_t adc_data[NUM_CH];

// Configura um pino como saida em funcao alternada push-pull, 50 MHz
// (CNF=10, MODE=11 -> nibble 0xB) usado para as saidas de PWM.
static void configureAltPushPull(GPIO_TypeDef *gpio, uint32_t pin)
{
    if (pin < 8U) {
        const uint32_t shift = pin * 4U;
        gpio->CRL &= ~(0xFU << shift);
        gpio->CRL |= (0xBU << shift);
    } else {
        const uint32_t shift = (pin - 8U) * 4U;
        gpio->CRH &= ~(0xFU << shift);
        gpio->CRH |= (0xBU << shift);
    }
}

int main(void)
{
    /* =====================================================================
     * 1) Clocks
     * ===================================================================== */
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;  // GPIOA (PA0..PA2 - potenciometros)
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;  // GPIOB (PB5, PB1, PB6 - LED RGB)
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;  // AFIO  (remap do TIM3)
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;  // ADC1
    RCC->AHBENR  |= RCC_AHBENR_DMA1EN;   // DMA1 (transfere o resultado do ADC)
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;  // TIM3 (PWM: R e G)
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;  // TIM4 (PWM: B)

    /* =====================================================================
     * 2) GPIO
     * ===================================================================== */
    // PA0, PA1, PA2 como entrada analogica (CNF=00, MODE=00 -> nibble 0).
    GPIOA->CRL &= ~0x00000FFFU;

    // Partial remap do TIM3 (MAPR bits 11:10 = 10): CH2 -> PB5, CH4 -> PB1.
    AFIO->MAPR = (AFIO->MAPR & ~AFIO_MAPR_TIM3_REMAP) | AFIO_MAPR_TIM3_REMAP_1;

    // Saidas dos canais de PWM.
    configureAltPushPull(GPIOB, 5U); // TIM3_CH2 -> Vermelho
    configureAltPushPull(GPIOB, 1U); // TIM3_CH4 -> Verde
    configureAltPushPull(GPIOB, 6U); // TIM4_CH1 -> Azul

    /* =====================================================================
     * 3) DMA1 - Canal 1 (associado ao ADC1)
     *
     * A cada conversao, o DMA copia ADC1->DR para a posicao seguinte do
     * vetor adc_data. Em modo circular (CIRC) ele recomeca apos as 3
     * leituras, mantendo o vetor sempre atualizado. (Vide EX_DMA.)
     * ===================================================================== */
    DMA1_Channel1->CCR = 0U;                          // desabilita p/ configurar
    DMA1_Channel1->CPAR  = (uint32_t)&ADC1->DR;        // origem: registrador do ADC
    DMA1_Channel1->CMAR  = (uint32_t)adc_data;         // destino: vetor na memoria
    DMA1_Channel1->CNDTR = NUM_CH;                     // 3 transferencias por ciclo
    // MSIZE=16bit, PSIZE=16bit, MINC=1, CIRC=1, DIR=periferico->memoria.
    DMA1_Channel1->CCR = DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 |
                         DMA_CCR_MINC | DMA_CCR_CIRC;
    DMA1_Channel1->CCR |= DMA_CCR_EN;                  // habilita o canal

    /* =====================================================================
     * 4) ADC1 - modo SCAN + CONTINUO, disparado por software, com DMA
     * ===================================================================== */
    // Sequencia regular: 3 conversoes -> L = NUM_CH - 1.
    ADC1->SQR1 = (NUM_CH - 1U) << ADC_SQR1_L_Pos;

    // Ordem da varredura: 1a=canal0 (PA0), 2a=canal1 (PA1), 3a=canal2 (PA2).
    ADC1->SQR3 = (0U << ADC_SQR3_SQ1_Pos) |
                 (1U << ADC_SQR3_SQ2_Pos) |
                 (2U << ADC_SQR3_SQ3_Pos);

    // Tempo de amostragem alto (239,5 ciclos) para os canais 0, 1 e 2.
    ADC1->SMPR2 = (7U << ADC_SMPR2_SMP0_Pos) |
                  (7U << ADC_SMPR2_SMP1_Pos) |
                  (7U << ADC_SMPR2_SMP2_Pos);

    // SCAN: percorre toda a sequencia. CONT: recomeca sozinho ao terminar.
    ADC1->CR1 |= ADC_CR1_SCAN;
    ADC1->CR2 |= ADC_CR2_CONT | ADC_CR2_DMA;

    // Trigger por software (EXTSEL = 111 = SWSTART) habilitado.
    ADC1->CR2 |= (7U << ADC_CR2_EXTSEL_Pos) | ADC_CR2_EXTTRIG;

    // Liga o ADC e faz a calibracao (melhora a precisao da conversao).
    ADC1->CR2 |= ADC_CR2_ADON;
    ADC1->CR2 |= ADC_CR2_RSTCAL;
    while (ADC1->CR2 & ADC_CR2_RSTCAL) { }
    ADC1->CR2 |= ADC_CR2_CAL;
    while (ADC1->CR2 & ADC_CR2_CAL) { }

    // Dispara a primeira conversao; em modo continuo nao precisa repetir.
    ADC1->CR2 |= ADC_CR2_ADON;
    ADC1->CR2 |= ADC_CR2_SWSTART;

    /* =====================================================================
     * 5) TIM3 / TIM4 - PWM das 3 cores
     *   PSC = 0    -> f_CK_CNT = 8 MHz
     *   ARR = 4095 -> f_PWM = 8 MHz / 4096 ~= 1,95 kHz (sem cintilacao)
     *   CCRx = leitura do ADC (0..4095) = brilho da cor.
     * ===================================================================== */
    TIM3->PSC = 0U;
    TIM3->ARR = PWM_MAX;
    // PWM modo 1 (OCxM = 110) + preload (OCxPE)
    TIM3->CCMR1 |= TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2PE; // CH2 (R)
    TIM3->CCMR2 |= TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4PE; // CH4 (G)
    TIM3->CCER  |= TIM_CCER_CC2E | TIM_CCER_CC4E;
    TIM3->CCR2 = 0U;
    TIM3->CCR4 = 0U;
    TIM3->CR1 |= TIM_CR1_ARPE;
    TIM3->EGR |= TIM_EGR_UG;
    TIM3->CR1 |= TIM_CR1_CEN;

    TIM4->PSC = 0U;
    TIM4->ARR = PWM_MAX;
    TIM4->CCMR1 |= TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE; // CH1 (B)
    TIM4->CCER  |= TIM_CCER_CC1E;
    TIM4->CCR1 = 0U;
    TIM4->CR1 |= TIM_CR1_ARPE;
    TIM4->EGR |= TIM_EGR_UG;
    TIM4->CR1 |= TIM_CR1_CEN;

    /* =====================================================================
     * 6) Laco principal
     *
     * O ADC + DMA atualizam adc_data[] sozinhos. Aqui so copiamos cada
     * leitura para a razao ciclica do canal de PWM correspondente.
     * ===================================================================== */
    while (1) {
        TIM3->CCR2 = adc_data[0]; // PA0 -> Vermelho
        TIM3->CCR4 = adc_data[1]; // PA1 -> Verde
        TIM4->CCR1 = adc_data[2]; // PA2 -> Azul
    }
}
