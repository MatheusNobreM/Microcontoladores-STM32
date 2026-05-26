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

static volatile uint8_t systick_flag = 0;

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

// SysTick dispara a cada 1s; main consome o flag e avanca a cor.
void SysTick_Handler(void)
{
    systick_flag = 1;
}

} // extern "C"

int main(void)
{
    // Habilita clock do GPIOB
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;

    configureOutputPushPull(GPIOB, LED_R);
    configureOutputPushPull(GPIOB, LED_G);
    configureOutputPushPull(GPIOB, LED_B);

    /* Configuracao do SysTick (estilo BM_SYSTICK)
     * Clock padrao do Bluepill: HSI = 8 MHz -> AHB = 8 MHz
     * CLKSOURCE = 0 -> SysTick usa AHB/8 = 1 MHz
     * Para 1 segundo: ticks = 1_000_000 (cabe em 24 bits: < 0xFFFFFF)
     */
    const uint32_t ticks = 1000000U;
    if ((ticks - 1UL) <= 0xFFFFFFUL) {
        SysTick->LOAD = (uint32_t)(ticks - 1UL); // valor de recarga
        SysTick->VAL  = 0UL;                     // zera o contador
        SysTick->CTRL = (0U << 2) |  // CLKSOURCE = 0 -> AHB/8
                        (1U << 1) |  // TICKINT   = 1 -> habilita IRQ
                        (1U << 0);   // ENABLE    = 1 -> liga o contador
    }

    uint8_t index = 0;
    applyColor(COLORS[index]); // mostra "Apagado" como estado inicial

    while (1) {
        if (systick_flag) {
            systick_flag = 0;
            index = (index + 1U) % NUM_COLORS;
            applyColor(COLORS[index]);
        }
        __WFI(); // dorme ate a proxima interrupcao
    }
}