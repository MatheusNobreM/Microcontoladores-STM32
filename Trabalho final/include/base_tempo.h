#ifndef BASE_TEMPO_H
#define BASE_TEMPO_H

#include <stdint.h>

/* ===========================================================================
 * base_tempo - Janela de amostragem com o TIM2
 *
 * Gera um evento periodico (JANELA_MS) que delimita o intervalo no qual os
 * pulsos da helice sao contados, permitindo o calculo de RPM.
 * ===========================================================================*/

/* Configura e inicia o TIM2. Chamar uma vez no boot. */
void BaseTempo_Init(void);

/* Retorna 1 (e limpa o aviso) quando uma janela acabou de fechar; senao 0. */
uint8_t BaseTempo_JanelaPronta(void);

#endif /* BASE_TEMPO_H */
