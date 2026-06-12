#include "stm32f1xx.h"

/* ===========================================================================
 * Pratica 5 - LED RGB com PWM
 *
 * Objetivo: acender e apagar LENTAMENTE (fade) cada combinacao de cores de um
 * LED RGB -> {Vermelho, Verde, Azul, Amarelo, Ciano, Roxo, Branco}.
 *
 * - TIM3 e TIM4 geram o PWM que controla o brilho de cada cor primaria.
 * - TIM2 (timer dedicado) gera interrupcoes periodicas que variam a razao
 *   ciclica (duty cycle), produzindo o efeito de fade. NAO ha uso de delay.
 *
 * Pinagem do LED RGB (mantida da pratica anterior):
 *   PB5 -> Vermelho (R) -> TIM3_CH2 (partial remap do TIM3 via AFIO)
 *   PB1 -> Verde    (G) -> TIM3_CH4
 *   PB6 -> Azul     (B) -> TIM4_CH1
 *
 * Clock: HSI = 8 MHz (Bluepill em CMSIS, sem PLL) -> TIM_CLK = 8 MHz.
 * ===========================================================================
 */

// Brilho maximo = ARR dos timers de PWM (resolucao de 8 bits)
static constexpr int32_t MAX_BRIGHTNESS = 255;

// Sequencia de cores: bit0=R, bit1=G, bit2=B
static constexpr uint8_t COLORS[] = {
    0b001, // Vermelho   (R)
    0b010, // Verde      (G)
    0b100, // Azul       (B)
    0b011, // Amarelo    (R+G)
    0b110, // Ciano      (G+B)
    0b101, // Roxo       (R+B)
    0b111, // Branco     (R+G+B)
};
static constexpr uint8_t NUM_COLORS = sizeof(COLORS) / sizeof(COLORS[0]);

// Estado do fade, atualizado pela ISR do TIM2.
static volatile int32_t brightness  = 0;  // 0 .. MAX_BRIGHTNESS
static volatile int32_t direction   = 1;  // +1 acendendo, -1 apagando
static volatile uint8_t color_index = 0;

// Aplica a cor atual com o brilho atual nas razoes ciclicas do PWM.
// Cada canal recebe o brilho se a cor o utiliza; caso contrario recebe 0.
static void applyPwm(uint8_t color, int32_t value)
{
    TIM3->CCR2 = (color & 0b001) ? static_cast<uint16_t>(value) : 0U; // R (PB5)
    TIM3->CCR4 = (color & 0b010) ? static_cast<uint16_t>(value) : 0U; // G (PB1)
    TIM4->CCR1 = (color & 0b100) ? static_cast<uint16_t>(value) : 0U; // B (PB6)
}

extern "C" {

// TIM2: a cada interrupcao, avanca um passo na variacao da razao ciclica.
void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR &= ~TIM_SR_UIF; // limpa flag de update

        brightness += direction;

        if (brightness >= MAX_BRIGHTNESS) {
            brightness = MAX_BRIGHTNESS; // chegou no topo: comeca a apagar
            direction  = -1;
        } else if (brightness <= 0) {
            brightness = 0;              // apagou: avanca para a proxima cor
            direction  = 1;
            color_index = (color_index + 1U) % NUM_COLORS;
        }

        applyPwm(COLORS[color_index], brightness);
    }
}

} // extern "C"

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
    /* --- Clocks --- */
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;  // GPIOB (PB5, PB1, PB6)
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;  // AFIO (remap do TIM3)
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;  // TIM3 (PWM: R e G)
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;  // TIM4 (PWM: B)
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;  // TIM2 (variacao da razao ciclica)

    /* Partial remap do TIM3 (MAPR bits 11:10 = 10):
     * CH2 -> PB5 (CH4 permanece em PB1). Mesma tecnica do exemplo
     * BM_TIM2_PWM, que remapeia o TIM2 para usar PB11. */
    AFIO->MAPR = (AFIO->MAPR & ~AFIO_MAPR_TIM3_REMAP) | AFIO_MAPR_TIM3_REMAP_1;

    /* --- GPIO: saidas dos canais de PWM --- */
    configureAltPushPull(GPIOB, 5U); // TIM3_CH2 -> Vermelho
    configureAltPushPull(GPIOB, 1U); // TIM3_CH4 -> Verde
    configureAltPushPull(GPIOB, 6U); // TIM4_CH1 -> Azul

    /* --- TIM3: PWM do Vermelho (CH2) e Verde (CH4) ---
     * PSC = 0   -> f_CK_CNT = 8 MHz
     * ARR = 255 -> f_PWM = 8 MHz / 256 ~= 31,25 kHz (sem cintilacao)
     * CCRx define a razao ciclica (0..255) = brilho de cada cor.
     */
    TIM3->PSC = 0U;
    TIM3->ARR = MAX_BRIGHTNESS;

    // PWM modo 1 (OCxM = 110) + preload (OCxPE)
    TIM3->CCMR1 |= TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2PE;  // CH2
    TIM3->CCMR2 |= TIM_CCMR2_OC4M_2 | TIM_CCMR2_OC4M_1 | TIM_CCMR2_OC4PE;  // CH4

    TIM3->CCER |= TIM_CCER_CC2E | TIM_CCER_CC4E; // habilita saidas

    TIM3->CCR2 = 0U;
    TIM3->CCR4 = 0U;

    TIM3->CR1 |= TIM_CR1_ARPE; // preload do ARR
    TIM3->EGR |= TIM_EGR_UG;   // carrega registradores de preload
    TIM3->CR1 |= TIM_CR1_CEN;  // habilita o contador

    /* --- TIM4: PWM do Azul (CH1) --- mesma configuracao do TIM3 */
    TIM4->PSC = 0U;
    TIM4->ARR = MAX_BRIGHTNESS;

    TIM4->CCMR1 |= TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE;  // CH1
    TIM4->CCER  |= TIM_CCER_CC1E;

    TIM4->CCR1 = 0U;

    TIM4->CR1 |= TIM_CR1_ARPE;
    TIM4->EGR |= TIM_EGR_UG;
    TIM4->CR1 |= TIM_CR1_CEN;

    /* --- TIM2: variacao da razao ciclica (fade) ---
     * PSC = 7999 -> f_CK_CNT = 8 MHz / 8000 = 1 kHz
     * ARR = 3    -> periodo = 4 / 1000 = 4 ms -> ~250 interrupcoes/s
     * Cada passo altera o brilho em 1: subida (256*4ms) + descida ~= 2 s por cor.
     */
    TIM2->PSC   = 7999U;
    TIM2->ARR   = 3U;
    TIM2->CNT   = 0U;
    TIM2->SR    = 0U;
    TIM2->DIER |= TIM_DIER_UIE;  // habilita interrupcao de update
    TIM2->CR1  |= TIM_CR1_CEN;   // habilita o contador

    NVIC_EnableIRQ(TIM2_IRQn);

    // Estado inicial: primeira cor, apagada, comecando a acender.
    applyPwm(COLORS[color_index], brightness);

    while (1) {
        __WFI(); // dorme; todo o trabalho ocorre na ISR do TIM2
    }
}
