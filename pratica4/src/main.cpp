#include "stm32f1xx.h"

static constexpr uint32_t LED_R = 5U; // PB5
static constexpr uint32_t LED_G = 1U; // PB1
static constexpr uint32_t LED_B = 6U; // PB6

// Sequencia de cores: bit0=R, bit1=G, bit2=B
// {Apagado, Vermelho, Verde, Azul, Amarelo, Ciano, Roxo, Branco}
static constexpr uint8_t COLORS[] = {
    0b000, // Apagado
    0b001, // Vermelho      (R)
    0b010, // Verde         (G)
    0b100, // Azul          (B)
    0b011, // Amarelo       (R+G)
    0b110, // Ciano         (G+B)
    0b101, // Roxo          (R+B)
    0b111, // Branco        (R+G+B)
};
static constexpr uint8_t NUM_COLORS = sizeof(COLORS) / sizeof(COLORS[0]);

// Cada cor pisca 2 vezes: 4 fases por cor (ON, OFF, ON, OFF)
static constexpr uint8_t PHASES_PER_COLOR = 4U;

static volatile uint8_t timer_flag = 0;

static void configureOutputPushPull(GPIO_TypeDef *gpio, uint32_t pin)
{
    const uint32_t shift = pin * 4U;

    gpio->CRL &= ~(0xFU << shift);
    gpio->CRL |= (0x2U << shift); // CNF=00, MODE=10: output push-pull, 2 MHz
    gpio->ODR &= ~(1U << pin);
}

static void applyColor(uint8_t color)
{
    const uint32_t mask = (1U << LED_R) | (1U << LED_G) | (1U << LED_B);
    uint32_t bits = 0U;

    if (color & 0b001) bits |= (1U << LED_R);
    if (color & 0b010) bits |= (1U << LED_G);
    if (color & 0b100) bits |= (1U << LED_B);

    GPIOB->ODR = (GPIOB->ODR & ~mask) | bits;
}

extern "C" {

// TIM2 dispara a cada 250 ms; main consome o flag e avanca a fase.
void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR &= ~TIM_SR_UIF; // limpa flag de update
        timer_flag = 1;
    }
}

} // extern "C"

int main(void)
{
    // Habilita clock do GPIOB (APB2) e do TIM2 (APB1)
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;

    configureOutputPushPull(GPIOB, LED_R);
    configureOutputPushPull(GPIOB, LED_G);
    configureOutputPushPull(GPIOB, LED_B);

    /* Configuracao do TIM2
     * Clock padrao do Bluepill: HSI = 8 MHz -> APB1 = 8 MHz -> TIM_CLK = 8 MHz
     * PSC = 7999 -> f_CK_CNT = 8 MHz / 8000 = 1 kHz
     * ARR = 249  -> periodo  = 250 / 1000  = 250 ms
     */
    TIM2->PSC   = 7999U;
    TIM2->ARR   = 249U;
    TIM2->CNT   = 0U;
    TIM2->SR    = 0U;            // garante flags limpas
    TIM2->DIER |= TIM_DIER_UIE;  // habilita IRQ de update
    TIM2->CR1  |= TIM_CR1_CEN;   // habilita o contador

    NVIC_EnableIRQ(TIM2_IRQn);

    uint8_t color_index = 0U;
    uint8_t phase = 0U; // 0..PHASES_PER_COLOR-1; fases pares = ON, impares = OFF
    applyColor(COLORS[color_index]); // estado inicial: primeira cor acesa

    while (1) {
        if (timer_flag) {
            timer_flag = 0;

            phase = (phase + 1U) % PHASES_PER_COLOR;
            if (phase == 0U) {
                // Concluiu 2 piscadas; avanca para a proxima cor
                color_index = (color_index + 1U) % NUM_COLORS;
            }

            if ((phase & 1U) == 0U) {
                applyColor(COLORS[color_index]); // fase ON
            } else {
                applyColor(0U);                  // fase OFF
            }
        }
        __WFI(); // dorme ate a proxima interrupcao
    }
}
