/* ============================================================================
 * Anemometro Digital - Firmware bare-metal (CMSIS / register-level)
 * Alvo : STM32F103C8 "Bluepill"  | Sensor: FC-03 (optoacoplador + LM393)
 *
 * Fluxo: vento -> rotor/disco encoder -> FC-03 DO -> PA0/EXTI0 -> contagem
 *        -> janela de 1 s (TIM2) -> calculo RPM/velocidade -> UART1 (debug)
 *
 * Estilo: tudo em registradores. As configuracoes de hardware e o modelo
 * fisico ficam em MACROS (foco baixo nivel), e a logica em funcoes curtas
 * com responsabilidade unica (driver FC-03 / servico Anemometer / app).
 * ==========================================================================*/

#include "stm32f1xx.h"

/* ----------------------------------------------------------------------------
 * 1. MACROS DE CONFIGURACAO DE HARDWARE (pinos, clocks, periodos)
 * --------------------------------------------------------------------------*/

/* Sensor FC-03: DO ligado em PA0 (linha EXTI0) */
#define SENSOR_PORT          GPIOA
#define SENSOR_PIN           0U
#define SENSOR_EXTI_LINE     0U                 /* EXTI0 atende PA0 */

/* UART de debug: USART1 TX em PA9 */
#define DBG_USART            USART1
#define DBG_TX_PIN           9U
#define DBG_BAUD             115200U
#define PCLK2_HZ             8000000U           /* HSI 8 MHz (sem reconfig de clock) */

/* Base de tempo da janela de amostragem (TIM2 em APB1) */
#define TIM_CLK_HZ           8000000U           /* TIM2_CLK = 8 MHz no Bluepill default */
#define SAMPLE_WINDOW_MS     1000U              /* janela fixa de 1 s */
#define TIM_PRESCALER        7999U              /* 8 MHz / 8000 = 1 kHz (tick de 1 ms) */
#define TIM_AUTORELOAD       ((SAMPLE_WINDOW_MS) - 1U)

/* Habilitacao de clocks (RCC) */
#define RCC_ENABLE_GPIOA()   (RCC->APB2ENR |= RCC_APB2ENR_IOPAEN)
#define RCC_ENABLE_AFIO()    (RCC->APB2ENR |= RCC_APB2ENR_AFIOEN)
#define RCC_ENABLE_USART1()  (RCC->APB2ENR |= RCC_APB2ENR_USART1EN)
#define RCC_ENABLE_TIM2()    (RCC->APB1ENR |= RCC_APB1ENR_TIM2EN)

/* Helper de configuracao de pino: 4 bits por pino (CNF[1:0]:MODE[1:0]).
 * Pinos 0..7 ficam em CRL, 8..15 em CRH; ver RM0008 9.2.1/9.2.2.   */
#define GPIO_CFG_PIN(port, pin, cfg)                                       \
    do {                                                                  \
        volatile uint32_t *reg = ((pin) < 8U) ? &(port)->CRL              \
                                              : &(port)->CRH;             \
        const uint32_t sh = ((pin) & 7U) * 4U;                            \
        *reg = (*reg & ~(0xFU << sh)) | ((uint32_t)(cfg) << sh);          \
    } while (0)

#define CFG_INPUT_FLOATING   0x4U   /* MODE=00 (input), CNF=01 (floating)        */
#define CFG_AF_PUSHPULL_50M  0xBU   /* MODE=11 (50MHz), CNF=10 (alt. push-pull)  */

/* Formula do BRR (USARTDIV em 1/16): BRR = fck / baud, com arredondamento */
#define USART_BRR_VALUE(pclk, baud)  (((pclk) + ((baud) / 2U)) / (baud))

/* ----------------------------------------------------------------------------
 * 2. MACROS DO MODELO FISICO (calibracao do anemometro)
 * --------------------------------------------------------------------------*/

#define ENCODER_SLOTS        20U      /* fendas/dentes por volta do disco encoder  */
#define CUP_RADIUS_M         0.070f   /* raio do braco do copo ao eixo [m]         */
#define CALIBRATION_K        2.5f     /* fator empirico (literatura 2,0..3,0)      */
#ifndef PI_F
#define PI_F                 3.14159265f
#endif

/* RPM = (pulsos / fendas) * (60000 / janela_ms) */
#define MS_PER_MIN_F         60000.0f
/* V_linear = 2*pi*r*(rpm/60) ; V_vento = V_linear * K ; km/h = m/s * 3.6 */
#define SEC_PER_MIN_F        60.0f
#define MS_TO_KMH_F          3.6f

/* ----------------------------------------------------------------------------
 * 3. ESTADO COMPARTILHADO ISR <-> MAIN
 * --------------------------------------------------------------------------*/

static volatile uint32_t g_pulse_count = 0U;  /* incrementado so na ISR do sensor */
static volatile uint8_t  g_window_done = 0U;  /* sinalizado pela ISR do TIM2       */

/* ----------------------------------------------------------------------------
 * 4. DRIVER FC-03 (conta pulsos; nenhum calculo na ISR)
 * --------------------------------------------------------------------------*/

static void FC03_Init(void)
{
    RCC_ENABLE_GPIOA();
    RCC_ENABLE_AFIO();

    /* PA0 como entrada flutuante (saida LM393 e' limpa, push-pull) */
    GPIO_CFG_PIN(SENSOR_PORT, SENSOR_PIN, CFG_INPUT_FLOATING);

    /* Liga a linha EXTI0 ao PORTA (campo de 4 bits, valor 0 = PA) */
    AFIO->EXTICR[SENSOR_EXTI_LINE / 4U] &=
        ~(0xFU << ((SENSOR_EXTI_LINE % 4U) * 4U));

    /* Borda de subida: fenda obstruida -> DO em nivel ALTO (relatorio 3.2) */
    EXTI->RTSR |= (1U << SENSOR_EXTI_LINE);
    EXTI->IMR  |= (1U << SENSOR_EXTI_LINE);   /* desmascara a interrupcao */

    NVIC_EnableIRQ(EXTI0_IRQn);
}

/* Le e zera a contagem de forma atomica (mitiga race condition, bug #2) */
static uint32_t FC03_TakePulseCount(void)
{
    __disable_irq();
    const uint32_t pulses = g_pulse_count;
    g_pulse_count = 0U;
    __enable_irq();
    return pulses;
}

/* ----------------------------------------------------------------------------
 * 5. SERVICO ANEMOMETER (modelo fisico)
 * --------------------------------------------------------------------------*/

static float Anemometer_CalculateRPM(uint32_t pulses, uint32_t window_ms)
{
    const float rev = (float)pulses / (float)ENCODER_SLOTS;
    return rev * (MS_PER_MIN_F / (float)window_ms);
}

static float Anemometer_WindSpeedMS(float rpm)
{
    const float v_lin = 2.0f * PI_F * CUP_RADIUS_M * (rpm / SEC_PER_MIN_F);
    return v_lin * CALIBRATION_K;
}

/* ----------------------------------------------------------------------------
 * 6. UART DE DEBUG (sem printf/float -> adequado a F103 sem FPU, bug #6)
 * --------------------------------------------------------------------------*/

static void UART_Init(void)
{
    RCC_ENABLE_GPIOA();
    RCC_ENABLE_USART1();

    /* PA9 como funcao alternativa push-pull (TX) -> fica no CRH */
    GPIO_CFG_PIN(GPIOA, DBG_TX_PIN, CFG_AF_PUSHPULL_50M);

    DBG_USART->BRR = USART_BRR_VALUE(PCLK2_HZ, DBG_BAUD);
    DBG_USART->CR1 = USART_CR1_TE | USART_CR1_UE;     /* TX habilitado, USART ligada */
}

static void UART_PutChar(char c)
{
    while (!(DBG_USART->SR & USART_SR_TXE)) { /* espera buffer livre */ }
    DBG_USART->DR = (uint8_t)c;
}

static void UART_PutStr(const char *s)
{
    while (*s) UART_PutChar(*s++);
}

/* Imprime um valor com 'decimals' casas a partir de um float positivo,
 * usando apenas aritmetica inteira na formatacao. */
static void UART_PutFixed(float value, uint8_t decimals)
{
    if (value < 0.0f) { UART_PutChar('-'); value = -value; }

    uint32_t scale = 1U;
    for (uint8_t i = 0U; i < decimals; ++i) scale *= 10U;

    const uint32_t scaled = (uint32_t)(value * (float)scale + 0.5f);
    const uint32_t ip = scaled / scale;   /* parte inteira  */
    uint32_t fp = scaled % scale;          /* parte decimal  */

    /* parte inteira */
    char buf[12];
    uint8_t n = 0U;
    uint32_t v = ip;
    do { buf[n++] = (char)('0' + (v % 10U)); v /= 10U; } while (v);
    while (n) UART_PutChar(buf[--n]);

    if (decimals == 0U) return;

    UART_PutChar('.');
    for (uint8_t i = decimals; i > 0U; --i) {
        uint32_t d = scale / 10U;
        UART_PutChar((char)('0' + (fp / d)));
        fp %= d;
        scale = d;
    }
}

/* ----------------------------------------------------------------------------
 * 7. BASE DE TEMPO (TIM2) -> sinaliza fim de cada janela de amostragem
 * --------------------------------------------------------------------------*/

static void Timer_Init(void)
{
    RCC_ENABLE_TIM2();

    TIM2->PSC   = TIM_PRESCALER;
    TIM2->ARR   = TIM_AUTORELOAD;
    TIM2->CNT   = 0U;
    TIM2->SR    = 0U;
    TIM2->DIER |= TIM_DIER_UIE;
    TIM2->CR1  |= TIM_CR1_CEN;

    NVIC_EnableIRQ(TIM2_IRQn);
}

/* ----------------------------------------------------------------------------
 * 8. ISRs
 * --------------------------------------------------------------------------*/

extern "C" {

/* Pulso do FC-03: apenas conta (isolamento de ISR, relatorio §6) */
void EXTI0_IRQHandler(void)
{
    if (EXTI->PR & (1U << SENSOR_EXTI_LINE)) {
        EXTI->PR = (1U << SENSOR_EXTI_LINE);  /* limpa escrevendo 1 */
        g_pulse_count++;
    }
}

/* Fim da janela de 1 s */
void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR &= ~TIM_SR_UIF;
        g_window_done = 1U;
    }
}

} // extern "C"

/* ----------------------------------------------------------------------------
 * 9. APP - boot, init e laco principal de aquisicao
 * --------------------------------------------------------------------------*/

int main(void)
{
    UART_Init();
    FC03_Init();
    Timer_Init();

    UART_PutStr("Anemometro Digital - bare-metal STM32F103\r\n");

    while (1) {
        if (g_window_done) {
            g_window_done = 0U;

            const uint32_t pulses = FC03_TakePulseCount();
            const float rpm  = Anemometer_CalculateRPM(pulses, SAMPLE_WINDOW_MS);
            const float vkmh = Anemometer_WindSpeedMS(rpm) * MS_TO_KMH_F;

            UART_PutStr("RPM:");
            UART_PutFixed(rpm, 1U);
            UART_PutStr(" V:");
            UART_PutFixed(vkmh, 2U);
            UART_PutStr(" km/h\r\n");
        }
        __WFI();  /* dorme ate a proxima interrupcao */
    }
}
