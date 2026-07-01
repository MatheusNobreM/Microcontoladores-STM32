#include "Anemometro_calculo.h"

AnemometroLeitura_t Anemometro_Calcular(uint32_t pulsos)
{
    AnemometroLeitura_t leitura;

    leitura.pulsos = pulsos;

    /* RPM: pulsos/s -> voltas/min. (pulsos*60)/N_FUROS; o compilador, N_FUROS = 5. */
    leitura.rpm = (pulsos * 60u) / ANEM_N_FUROS;

    /* Velocidade em km/h * 1000: reta pela origem (0 pulsos -> 0 km/h) */
    leitura.velocidade_kmh_x1000 = pulsos * ANEM_FATOR_KMH_X1000;

    return leitura;
}
