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

static void writeLed(GPIO_TypeDef *gpio, uint32_t pin, bool on)
{
    if (on) {
        gpio->ODR |= (1U << pin);
    } else {
        gpio->ODR &= ~(1U << pin);
    }
}

static bool isButtonPressed(GPIO_TypeDef *gpio, uint32_t pin)
{
    return (gpio->IDR & (1U << pin)) == 0U;
}

int main(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN;

    configureInputPullUp(GPIOA, BUTTON_1);
    configureInputPullUp(GPIOA, BUTTON_2);
    configureInputPullUp(GPIOA, BUTTON_3);

    configureOutputPushPull(GPIOB, LED_1);
    configureOutputPushPull(GPIOB, LED_2);
    configureOutputPushPull(GPIOB, LED_3);

    while (1) {
        writeLed(GPIOB, LED_1, isButtonPressed(GPIOA, BUTTON_1));
        writeLed(GPIOB, LED_2, isButtonPressed(GPIOA, BUTTON_2));
        writeLed(GPIOB, LED_3, isButtonPressed(GPIOA, BUTTON_3));
    }
}
