#include "stm32f1xx.h"

static constexpr uint32_t BUTTON_1 = 1U; // PA1
static constexpr uint32_t BUTTON_2 = 2U; // PA2
static constexpr uint32_t BUTTON_3 = 3U; // PA3

static constexpr uint32_t LED_1 = 5U; // PB5
static constexpr uint32_t LED_2 = 1U; // PB1
static constexpr uint32_t LED_3 = 6U; // PB6

static void configureInputPullUp(GPIO_TypeDef *gpio, uint32_t pin)
{
    const uint32_t shift = pin * 4U;

    gpio->CRL &= ~(0xFU << shift);
    gpio->CRL |= (0x8U << shift); // CNF=10, MODE=00: input with pull-up/pull-down
    gpio->ODR |= (1U << pin);     // ODR=1 selects pull-up
}

static void configureOutputPushPull(GPIO_TypeDef *gpio, uint32_t pin)
{
    const uint32_t shift = pin * 4U;

    gpio->CRL &= ~(0xFU << shift);
    gpio->CRL |= (0x2U << shift); // CNF=00, MODE=10: output push-pull, 2 MHz
    gpio->ODR &= ~(1U << pin);
}

// Configures EXTI for a GPIOA pin: falling-edge trigger, interrupt unmasked.
// AFIO_EXTICR index = pin/4, shift = (pin%4)*4; value 0x0 selects GPIOA.
static void configureExti(uint32_t pin)
{
    const uint32_t index = pin / 4U;
    const uint32_t shift = (pin % 4U) * 4U;

    AFIO->EXTICR[index] &= ~(0xFU << shift); // select GPIOA (0x0)

    EXTI->RTSR &= ~(1U << pin); // disable rising-edge trigger
    EXTI->FTSR |=  (1U << pin); // enable falling-edge trigger (button pull-up → low when pressed)

    EXTI->PR  =  (1U << pin); // clear any pending flag before enabling
    EXTI->IMR |= (1U << pin); // unmask interrupt line
}

extern "C" {

void EXTI1_IRQHandler(void)
{
    if (EXTI->PR & (1U << BUTTON_1)) {
        EXTI->PR = (1U << BUTTON_1);   // clear pending flag (write 1 to clear)
        GPIOB->ODR ^= (1U << LED_1);   // XOR: toggle LED 1
    }
}

void EXTI2_IRQHandler(void)
{
    if (EXTI->PR & (1U << BUTTON_2)) {
        EXTI->PR = (1U << BUTTON_2);
        GPIOB->ODR ^= (1U << LED_2);   // XOR: toggle LED 2
    }
}

void EXTI3_IRQHandler(void)
{
    if (EXTI->PR & (1U << BUTTON_3)) {
        EXTI->PR = (1U << BUTTON_3);
        GPIOB->ODR ^= (1U << LED_3);   // XOR: toggle LED 3
    }
}

} // extern "C"

int main(void)
{
    // Enable clocks: GPIOA, GPIOB and AFIO (required for EXTI mux)
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN | RCC_APB2ENR_AFIOEN;

    configureInputPullUp(GPIOA, BUTTON_1);
    configureInputPullUp(GPIOA, BUTTON_2);
    configureInputPullUp(GPIOA, BUTTON_3);

    configureOutputPushPull(GPIOB, LED_1);
    configureOutputPushPull(GPIOB, LED_2);
    configureOutputPushPull(GPIOB, LED_3);

    // Wire each GPIOA pin to its EXTI line and arm the trigger
    configureExti(BUTTON_1); // PA1 → EXTI1
    configureExti(BUTTON_2); // PA2 → EXTI2
    configureExti(BUTTON_3); // PA3 → EXTI3

    // Enable the three NVIC channels
    NVIC_EnableIRQ(EXTI1_IRQn);
    NVIC_EnableIRQ(EXTI2_IRQn);
    NVIC_EnableIRQ(EXTI3_IRQn);

    while (1) {
        __WFI(); // sleep until the next interrupt
    }
}
