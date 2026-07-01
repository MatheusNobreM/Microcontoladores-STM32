#include <stdint.h>

#if !defined(__SOFT_FP__) && defined(__ARM_FP)
  #warning "FPU is not initialized..."
#endif

#include <stm32f103xb.h>
#include "anemometro_sensor.h"
#include "anemometro_calculo.h"
#include "led_feedback.h"
#include "uart_debug.h"

#define CLOCK_SISTEMA_HZ 8000000u

int main(void)
{
    AnemometroLeitura_t leitura = {0};

    Sensor_Init();
    Sensor_TimerInit(CLOCK_SISTEMA_HZ);
    Led_Init();
    Uart_Init();

    while (1)
    {
        Led_Atualizar(Sensor_GetMillis());

        if (Sensor_DadoPronto())
        {
            uint32_t pulsos = Sensor_LerPulsosUltimoSegundo();
            leitura = Anemometro_Calcular(pulsos);

            /* LED usa a velocidade em ponto fixo (km/h * 1000) */
            Led_SetVelocidade(leitura.velocidade_kmh_x1000);

            /* Envia dados brutos (frame binário) pelo cabo TTL.
            Uart_EnviarFrame((uint16_t)leitura.pulsos,
                             (uint16_t)leitura.rpm,
                             (uint16_t)(leitura.velocidade_kmh_x1000 / 10u));
        }
    }
}
