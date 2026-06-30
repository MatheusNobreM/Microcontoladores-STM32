#include <stdint.h>
#include <stdio.h> // Adicionado para snprintf

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
  #warning "FPU is not initialized..."
#endif

#include <stm32f103xb.h>
#include "anemometro_sensor.h"
#include "anemometro_calculo.h"
#include "led_feedback.h"
#include "uart_debug.h" // Adicionado o módulo UART

#define CLOCK_SISTEMA_HZ 8000000u

int main(void)
{
    AnemometroLeitura_t leitura = {0};
    char buffer[64]; // Buffer para formatar a string de saída

    /* Inicialização dos módulos */
    Sensor_Init();
    Sensor_TimerInit(CLOCK_SISTEMA_HZ);
    Led_Init();
    Uart_Init(); // Inicializa a comunicação serial

    Uart_SendString("\r\n--- Anemometro Inicializado ---\r\n");

    while (1)
    {
        Led_Atualizar(Sensor_GetMillis());

        if (Sensor_DadoPronto())
        {
            uint32_t pulsos = Sensor_LerPulsosUltimoSegundo();
            leitura = Anemometro_Calcular(pulsos);
            Led_SetVelocidade(leitura.velocidade_kmh);

            /* Formata os dados em texto e envia pelo cabo TTL */
            snprintf(buffer, sizeof(buffer), "Pulsos: %lu | RPM: %.1f | Vel: %.2f km/h\r\n", 
                     pulsos, leitura.rpm, leitura.velocidade_kmh);
            
            Uart_SendString(buffer);
        }
    }
}