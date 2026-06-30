/* ============================================================================
 * Contador de Helice - Firmware bare-metal (CMSIS / register-level)
 * Alvo : STM32F103C8 "Bluepill"
 * Sensor: LM393 + optoacoplador (modulo de velocidade tipo FC-03)
 *
 * Objetivo: contar as voltas de uma helice usando a saida digital (D0) do
 * sensor como INTERRUPCAO EXTERNA. A cada corte do feixe, a ISP do EXTI0
 * incrementa um contador; a cada 1 s (TIM2) calculam-se pulsos, RPM e voltas.
 *
 * Codigo modularizado (cada arquivo tem uma responsabilidade unica):
 *   config.h        -> pinos, clocks e constantes
 *   sensor_helice   -> driver do LM393 (D0/EXTI0 + A0/ADC) e contagem
 *   base_tempo      -> TIM2: janela de medicao de 1 s
 *   contador        -> conversao pulsos -> RPM / voltas
 *   uart            -> saida de debug (USART1 / PA9)
 *   main.cpp        -> orquestra os modulos (este arquivo)
 *
 * Ligacao do sensor:  VCC->3V3  GND->GND  D0->PA0  A0->PA1
 * ==========================================================================*/

#include "config.h"
#include "sensor_helice.h"
#include "base_tempo.h"
#include "contador.h"
#include "uart.h"
#include "stm32f1xx.h"

int main(void)
{
    UART_Init();
    SensorHelice_Init();
    BaseTempo_Init();

    UART_PutStr("Contador de Helice - LM393/optoacoplador (STM32F103)\r\n");

    uint32_t total_pulsos = 0U;   /* acumulado desde o boot */

    while (1) {
        /* A contagem ocorre por interrupcao (EXTI0). Aqui so processamos
         * o resultado quando a janela de 1 s do TIM2 fecha. */
        if (BaseTempo_JanelaPronta()) {
            const uint32_t pulsos = SensorHelice_PegarEZerar();
            total_pulsos += pulsos;

            const float    rpm    = Contador_RPM(pulsos, JANELA_MS);
            const uint32_t voltas = Contador_Voltas(total_pulsos);
            const uint16_t a0     = SensorHelice_LerAnalogico();

            UART_PutStr("Pulsos:");   UART_PutUint(pulsos);
            UART_PutStr("  RPM:");    UART_PutFixed(rpm, 1U);
            UART_PutStr("  Voltas:"); UART_PutUint(voltas);
            UART_PutStr("  A0:");     UART_PutUint(a0);
            UART_PutStr("\r\n");
        }

        __WFI();   /* dorme ate a proxima interrupcao (EXTI0 ou TIM2) */
    }
}
