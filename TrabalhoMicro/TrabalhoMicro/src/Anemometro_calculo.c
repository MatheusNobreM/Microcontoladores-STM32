/**
 ******************************************************************************
 * @file    anemometro_calculo.c
 * @brief   Cálculo de RPM e velocidade do vento em ponto fixo (só inteiros).
 ******************************************************************************
 */

#include "Anemometro_calculo.h"

AnemometroLeitura_t Anemometro_Calcular(uint32_t pulsos)
{
    AnemometroLeitura_t leitura;

    leitura.pulsos = pulsos;

    /* RPM: pulsos/s -> voltas/min. (pulsos*60)/N_FUROS; o compilador
     * reduz para pulsos*12 quando N_FUROS = 5. */
    leitura.rpm = (pulsos * 60u) / ANEM_N_FUROS;

    /* Velocidade em km/h * 1000: reta pela origem (0 pulsos -> 0 km/h),
     * geral para toda a faixa de vento. Uma única multiplicação inteira. */
    leitura.velocidade_kmh_x1000 = pulsos * ANEM_FATOR_KMH_X1000;

    return leitura;
}
