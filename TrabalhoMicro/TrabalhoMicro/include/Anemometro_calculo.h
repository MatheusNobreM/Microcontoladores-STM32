/**
 ******************************************************************************
 * @file    anemometro_calculo.h
 * @brief   Cálculo de RPM e velocidade do vento a partir da contagem de
 *          pulsos do disco encoder, usando SOMENTE aritmética inteira
 *          (ponto fixo). Sem float -> assembly enxuto, sem uso da FPU/libc.
 ******************************************************************************
 */

#ifndef ANEMOMETRO_CALCULO_H
#define ANEMOMETRO_CALCULO_H

#include <stdint.h>

/* ---------------------------------------------------------------------- */
/* Parâmetros físicos fixos (AJUSTAR após calibração de campo)            */
/* ---------------------------------------------------------------------- */
#define ANEM_N_FUROS          5u    /* Furos do disco encoder (PPR)                 */

/* RPM = (pulsos * 60) / N_FUROS. Para N_FUROS = 5 => pulsos * 12 (exato).          */
#define ANEM_RPM_POR_PULSO    12u

/* Velocidade do vento: modelo LINEAR PELA ORIGEM, geral para TODA a faixa:
 *
 *   V_kmh = FATOR * pulsos_por_s        (0 pulsos -> 0 km/h)
 *
 * O ventilador serve só como referência para fixar o FATOR. Ancorado no ponto
 * de maior confiança (anemômetro comercial: 7.9 m/s = 28.44 km/h na velocidade
 * máxima, ~1250 pulsos/s):
 *
 *   FATOR = 28.44 / 1250 = 0.02275 km/h por (pulso/s)  ->  x1000 ~= 23
 *
 * LIMITAÇÃO: os 3 pontos do ventilador NÃO são proporcionais (a hélice
 * responde com um pequeno "offset"), então um fator único casa bem perto do
 * ponto ancorado (topo) e SUBESTIMA um pouco as velocidades baixas. Para
 * precisão em toda a faixa, calibre também com vento fraco e reajuste o FATOR.
 *
 * REANCORAR: FATOR_x1000 = round(1000 * V_ref_kmh / pulsos_no_ref).
 */
#define ANEM_FATOR_KMH_X1000   23u

/* Leitura já convertida. Tudo inteiro; a velocidade guarda km/h * 1000
 * (ex.: 12345 => 12.345 km/h). */
typedef struct
{
    uint32_t pulsos;               /* Pulsos brutos da janela de 1s          */
    uint32_t rpm;                  /* Rotações por minuto (inteiro)          */
    uint32_t velocidade_kmh_x1000; /* Velocidade em km/h * 1000              */
} AnemometroLeitura_t;

/**
 * @brief Converte a contagem de pulsos de 1 segundo em RPM e velocidade
 *        do vento (km/h * 1000), usando apenas multiplicações inteiras.
 * @param pulsos Quantidade de pulsos contados na última janela de 1s
 * @return Estrutura com pulsos, rpm e velocidade_kmh_x1000
 */
AnemometroLeitura_t Anemometro_Calcular(uint32_t pulsos);

#endif /* ANEMOMETRO_CALCULO_H */
