#ifndef ANEMOMETRO_CALCULO_H
#define ANEMOMETRO_CALCULO_H

#include <stdint.h>

#define ANEM_N_FUROS          5u    /* Furos do disco encoder (PPR)                 */

#define ANEM_RPM_POR_PULSO    12u

#define ANEM_FATOR_KMH_X1000   23u

typedef struct
{
    uint32_t pulsos;               /* Pulsos brutos da janela de 1s          */
    uint32_t rpm;                  /* Rotações por minuto (inteiro)          */
    uint32_t velocidade_kmh_x1000; /* Velocidade em km/h * 1000              */
} AnemometroLeitura_t;

AnemometroLeitura_t Anemometro_Calcular(uint32_t pulsos);

#endif 
