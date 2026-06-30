/**
 ******************************************************************************
 * @file    anemometro_calculo.h
 * @brief   Cálculo de RPM e velocidade do vento a partir da contagem de
 *          pulsos do disco encoder.
 ******************************************************************************
 */

#ifndef ANEMOMETRO_CALCULO_H
#define ANEMOMETRO_CALCULO_H

#include <stdint.h>

/* ---------------------------------------------------------------------- */
/* Parâmetros físicos fixos (valores iniciais - AJUSTAR após calibração)  */
/* ---------------------------------------------------------------------- */
#define ANEM_N_FUROS      20      /* Número de furos do disco encoder (PPR)       */
#define ANEM_RAIO_HELICE  0.04f   /* Raio do cooler até a ponta da pá, em metros  */
#define ANEM_FATOR_K      1.2f    /* Fator de calibração aerodinâmica (provisório)*/

typedef struct
{
    float rpm;
    float velocidade_ms;
    float velocidade_kmh;
} AnemometroLeitura_t;

/**
 * @brief Converte a contagem de pulsos de 1 segundo em RPM e velocidade
 *        do vento (m/s e km/h), usando os parâmetros fixos definidos acima.
 * @param pulsos Quantidade de pulsos contados na última janela de 1s
 * @return Estrutura com rpm, velocidade_ms e velocidade_kmh calculados
 */
AnemometroLeitura_t Anemometro_Calcular(uint32_t pulsos);

#endif /* ANEMOMETRO_CALCULO_H */