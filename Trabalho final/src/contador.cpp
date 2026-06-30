#include "contador.h"
#include "config.h"

#define MS_POR_MINUTO_F   60000.0f

float Contador_RPM(uint32_t pulsos, uint32_t janela_ms)
{
    if (janela_ms == 0U) {
        return 0.0f;                       /* evita divisao por zero */
    }
    const float voltas = (float)pulsos / (float)PULSOS_POR_VOLTA;
    return voltas * (MS_POR_MINUTO_F / (float)janela_ms);
}

uint32_t Contador_Voltas(uint32_t pulsos)
{
    return pulsos / PULSOS_POR_VOLTA;
}
