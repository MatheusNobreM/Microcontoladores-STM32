/**
 ******************************************************************************
 * @file    anemometro_calculo.c
 * @brief   Implementação do cálculo de RPM e velocidade do vento.
 ******************************************************************************
 */

#include "Anemometro_calculo.h"

#define ANEM_PI 3.14159f

AnemometroLeitura_t Anemometro_Calcular(uint32_t pulsos)
{
    AnemometroLeitura_t leitura;

    /* RPM: pulsos por segundo -> voltas por minuto */
    leitura.rpm = ((float)pulsos * 60.0f) / (float)ANEM_N_FUROS;

    /* Velocidade tangencial na ponta da hélice (m/s) */
    leitura.velocidade_ms = ((float)pulsos * 2.0f * ANEM_PI * ANEM_RAIO_HELICE)
                             / (float)ANEM_N_FUROS;

    /* Aplica o fator de perda/calibração e converte para km/h */
    leitura.velocidade_ms  *= ANEM_FATOR_K;
    leitura.velocidade_kmh  = leitura.velocidade_ms * 3.6f;

    return leitura;
}