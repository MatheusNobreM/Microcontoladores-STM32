#ifndef CONTADOR_H
#define CONTADOR_H

#include <stdint.h>

/* ===========================================================================
 * contador - Converte pulsos do sensor em grandezas da helice
 *
 * Modulo puramente matematico (sem hardware): facilita o teste e o reuso.
 * Usa PULSOS_POR_VOLTA (config.h) para relacionar pulsos e voltas.
 * ===========================================================================*/

/* Rotacao em RPM a partir dos pulsos contados numa janela de 'janela_ms'.
 * RPM = (pulsos / PULSOS_POR_VOLTA) * (60000 / janela_ms) */
float Contador_RPM(uint32_t pulsos, uint32_t janela_ms);

/* Numero inteiro de voltas completas contidas em 'pulsos'. */
uint32_t Contador_Voltas(uint32_t pulsos);

#endif /* CONTADOR_H */
