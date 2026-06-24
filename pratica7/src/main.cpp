#include "stm32f1xx.h"

/* ===========================================================================
 * Pratica 7 - LED RGB controlado por USART
 *
 * Objetivo: ajustar a razao ciclica (duty cycle) dos 3 canais de PWM que
 * acionam um LED RGB por meio de comandos recebidos pela USART. Tambem ha um
 * "modo de demonstracao" (fade), equivalente ao da pratica 5.
 *
 * Comandos (terminados por '\n' ou, no Windows, '\r'):
 *   rXX / RXX -> ajusta o Vermelho (R) para XX  {00..99}
 *   gXX / GXX -> ajusta o Verde    (G) para XX  {00..99}
 *   bXX / BXX -> ajusta o Azul     (B) para XX  {00..99}
 *   d   / D   -> TOGGLE do modo de demonstracao (fade automatico de cores)
 *
 * Durante o modo de demonstracao os comandos de cor sao IGNORADOS; apenas o
 * comando 'd'/'D' continua valido (para sair do modo).
 *
 * Pinagem do LED RGB (mantida das praticas 5 e 6):
 *   PB5 -> Vermelho (R) -> TIM3_CH2 (partial remap do TIM3 via AFIO)
 *   PB1 -> Verde    (G) -> TIM3_CH4
 *   PB6 -> Azul     (B) -> TIM4_CH1
 *
 * USART1 (8 MHz HSI, sem PLL -> PCLK2 = 8 MHz):
 *   PA9  -> USART1_TX (saida, eco/confirmacao)
 *   PA10 -> USART1_RX (entrada dos comandos)
 *   Baud rate: 9600 8N1
 *
 * Clock: HSI = 8 MHz -> TIM_CLK = 8 MHz, PCLK2 = 8 MHz.
 * ===========================================================================
 */

/* PWM com ARR = 99 -> contagem de 0 a 99 (100 passos).
 * Assim a razao ciclica em "porcentagem" XX {00..99} vai direto para o CCR.
 * f_PWM = 8 MHz / 100 = 80 kHz (sem cintilacao). */
static constexpr uint16_t PWM_MAX = 99U;

/* USART --------------------------------------------------------------------*/
static constexpr uint32_t PCLK2_HZ = 8000000U;
static constexpr uint32_t BAUD     = 9600U;
// BRR = fck / baud, com arredondamento (USARTDIV em 1/16).
static constexpr uint32_t BRR_VALUE = (PCLK2_HZ + BAUD / 2U) / BAUD;

/* Buffer de recepcao do comando atual. */
static constexpr uint32_t RX_BUF_SIZE = 8U;
static volatile char     rx_buf[RX_BUF_SIZE];
static volatile uint32_t rx_len = 0U;

/* Razoes ciclicas alvo definidas pelos comandos de cor (0..99). */
static volatile uint8_t duty_r = 0U;
static volatile uint8_t duty_g = 0U;
static volatile uint8_t duty_b = 0U;

/* Estado do modo de demonstracao (fade). */
static volatile bool demo_mode = false;

/* --- Modo de demonstracao (fade), equivalente a pratica 5 --- */
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

static volatile int32_t demo_brightness = 0;   // 0 .. PWM_MAX
static volatile int32_t demo_direction  = 1;   // +1 acendendo, -1 apagando
static volatile uint8_t demo_color      = 0;

/* Aplica diretamente os 3 valores de razao ciclica nos canais de PWM. */
static void applyPwm(uint16_t r, uint16_t g, uint16_t b)
{
    TIM3->CCR2 = r; // R (PB5)
    TIM3->CCR4 = g; // G (PB1)
    TIM4->CCR1 = b; // B (PB6)
}

/* Reaplica as razoes ciclicas definidas pelos comandos de cor. */
static void applyDuties(void)
{
    applyPwm(duty_r, duty_g, duty_b);
}

extern "C" {

/* TIM2: a cada interrupcao avanca um passo no fade (so atua em modo demo). */
void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR &= ~TIM_SR_UIF;

        if (!demo_mode) {
            return;
        }

        demo_brightness += demo_direction;

        if (demo_brightness >= PWM_MAX) {
            demo_brightness = PWM_MAX;   // chegou ao topo: comeca a apagar
            demo_direction  = -1;
        } else if (demo_brightness <= 0) {
            demo_brightness = 0;         // apagou: avanca para a proxima cor
            demo_direction  = 1;
            demo_color = (demo_color + 1U) % NUM_COLORS;
        }

        const uint8_t c = COLORS[demo_color];
        const uint16_t v = static_cast<uint16_t>(demo_brightness);
        applyPwm((c & 0b001) ? v : 0U,
                 (c & 0b010) ? v : 0U,
                 (c & 0b100) ? v : 0U);
    }
}

} // extern "C"

/* Envia uma string pela USART1 (espera o buffer de TX ficar livre). */
static void uart_puts(const char *s)
{
    while (*s) {
        while (!(USART1->SR & USART_SR_TXE)) { }
        USART1->DR = static_cast<uint8_t>(*s++);
    }
}

/* Liga/desliga o modo de demonstracao. */
static void toggleDemo(void)
{
    demo_mode = !demo_mode;

    if (demo_mode) {
        // (Re)inicia o fade a partir da primeira cor, apagada.
        demo_brightness = 0;
        demo_direction  = 1;
        demo_color      = 0;
        TIM2->CNT = 0U;
        TIM2->SR  = 0U;
        TIM2->CR1 |= TIM_CR1_CEN;   // habilita o contador do fade
        uart_puts("DEMO ON\r\n");
    } else {
        TIM2->CR1 &= ~TIM_CR1_CEN;  // para o fade
        applyDuties();              // volta para as cores definidas por comando
        uart_puts("DEMO OFF\r\n");
    }
}

/* Interpreta o comando completo acumulado em rx_buf (sem o terminador). */
static void parseCommand(void)
{
    if (rx_len == 0U) {
        return;
    }

    // Comando de toggle do modo demo: aceito mesmo durante a demonstracao.
    if (rx_len == 1U && (rx_buf[0] == 'd' || rx_buf[0] == 'D')) {
        toggleDemo();
        return;
    }

    // Durante o modo de demonstracao, os demais comandos sao ignorados.
    if (demo_mode) {
        uart_puts("(demo) ignorado\r\n");
        return;
    }

    // Comando de cor: letra + 2 digitos {00..99}.
    if (rx_len == 3U && rx_buf[1] >= '0' && rx_buf[1] <= '9' &&
                        rx_buf[2] >= '0' && rx_buf[2] <= '9') {
        const uint8_t value = static_cast<uint8_t>((rx_buf[1] - '0') * 10 +
                                                   (rx_buf[2] - '0'));
        switch (rx_buf[0]) {
            case 'r': case 'R': duty_r = value; applyDuties(); uart_puts("R ok\r\n"); return;
            case 'g': case 'G': duty_g = value; applyDuties(); uart_puts("G ok\r\n"); return;
            case 'b': case 'B': duty_b = value; applyDuties(); uart_puts("B ok\r\n"); return;
            default: break;
        }
    }

    uart_puts("comando invalido\r\n");
}

extern "C" {

/* USART1: recebe cada caractere e monta o comando ate o terminador. */
void USART1_IRQHandler(void)
{
    if (USART1->SR & USART_SR_RXNE) {
        const char ch = static_cast<char>(USART1->DR); // ler DR limpa RXNE

        if (ch == '\n' || ch == '\r') {
            parseCommand();
            rx_len = 0U;                 // pronto para o proximo comando
        } else if (rx_len < RX_BUF_SIZE) {
            rx_buf[rx_len++] = ch;
        } else {
            rx_len = 0U;                 // overflow: descarta comando atual
        }
    }
}

} // extern "C"

// Configura um pino como saida em funcao alternada push-pull, 50 MHz
// (CNF=10, MODE=11 -> nibble 0xB).
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
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;   // GPIOA (PA9/PA10 - USART1)
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;   // GPIOB (PB5, PB1, PB6 - LED RGB)
    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;   // AFIO  (remap do TIM3)
    RCC->APB2ENR |= RCC_APB2ENR_USART1EN; // USART1
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;   // TIM3 (PWM: R e G)
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;   // TIM4 (PWM: B)
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;   // TIM2 (fade do modo demo)

    /* =====================================================================
     * 2) GPIO
     * ===================================================================== */
    // Partial remap do TIM3 (MAPR bits 11:10 = 10): CH2 -> PB5, CH4 -> PB1.
    AFIO->MAPR = (AFIO->MAPR & ~AFIO_MAPR_TIM3_REMAP) | AFIO_MAPR_TIM3_REMAP_1;

    // Saidas dos canais de PWM.
    configureAltPushPull(GPIOB, 5U); // TIM3_CH2 -> Vermelho
    configureAltPushPull(GPIOB, 1U); // TIM3_CH4 -> Verde
    configureAltPushPull(GPIOB, 6U); // TIM4_CH1 -> Azul

    // USART1: PA9 = TX (AF push-pull 50 MHz, nibble 0xB).
    configureAltPushPull(GPIOA, 9U);
    // USART1: PA10 = RX (entrada flutuante: CNF=01, MODE=00 -> nibble 0x4).
    GPIOA->CRH &= ~(0xFU << ((10U - 8U) * 4U));
    GPIOA->CRH |=  (0x4U << ((10U - 8U) * 4U));

    /* =====================================================================
     * 3) USART1 - 9600 8N1, RX por interrupcao
     * ===================================================================== */
    USART1->BRR = BRR_VALUE;
    USART1->CR1 = USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE | USART_CR1_UE;
    NVIC_EnableIRQ(USART1_IRQn);

    /* =====================================================================
     * 4) TIM3 / TIM4 - PWM das 3 cores
     *   PSC = 0  -> f_CK_CNT = 8 MHz
     *   ARR = 99 -> f_PWM = 8 MHz / 100 = 80 kHz
     *   CCRx = razao ciclica (0..99).
     * ===================================================================== */
    TIM3->PSC = 0U;
    TIM3->ARR = PWM_MAX;
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
     * 5) TIM2 - base de tempo do fade (modo demo)
     *   PSC = 7999 -> f_CK_CNT = 8 MHz / 8000 = 1 kHz
     *   ARR = 9    -> ~100 interrupcoes/s -> ~2 s por cor (subida+descida).
     *   O contador so e habilitado quando o modo demo esta ativo.
     * ===================================================================== */
    TIM2->PSC   = 7999U;
    TIM2->ARR   = 9U;
    TIM2->CNT   = 0U;
    TIM2->SR    = 0U;
    TIM2->DIER |= TIM_DIER_UIE;
    NVIC_EnableIRQ(TIM2_IRQn);

    // Estado inicial: LED apagado, modo demo desligado.
    applyDuties();
    uart_puts("Pratica 7 - RGB via USART (9600 8N1)\r\n");

    /* =====================================================================
     * 6) Laco principal: tudo ocorre nas ISRs (USART e TIM2).
     * ===================================================================== */
    while (1) {
        __WFI();
    }
}
