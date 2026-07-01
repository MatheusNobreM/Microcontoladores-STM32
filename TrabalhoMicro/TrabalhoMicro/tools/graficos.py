#!/usr/bin/env python3
"""
Gera gráficos a partir dos dados persistidos pelo receptor do anemômetro.

Lê o arquivo JSON Lines produzido por 'receptor.py' (uma leitura por linha)
e monta 3 gráficos ao longo do tempo:
  1. Velocidade (km/h)
  2. RPM
  3. Pulsos por segundo (dado mais cru) em onda quadrada (degraus),
     com eixo secundário em voltas/s (5 furos = 1 volta).

Por padrão o gráfico FOCA na região com movimento (corta os longos trechos
de vento zero no início/fim), o que deixa a visualização muito mais legível.
Use --completo para ver a medição inteira.

Uso:
    python graficos.py [ARQUIVO.jsonl] [SAIDA.png] [--completo]
    ex.: python graficos.py dados_anemometro.jsonl grafico_anemometro.png

Dependência:
    pip install matplotlib
"""

import sys
import json
from datetime import datetime

try:
    import matplotlib.pyplot as plt
except ImportError:
    sys.exit("Falta a dependência 'matplotlib'. Instale com: pip install matplotlib")

ARQUIVO_ENTRADA = "dados_anemometro.jsonl"
ARQUIVO_SAIDA = "grafico_anemometro.png"

# Disco encoder: 5 furos = 1 volta (1 ciclo). Deve casar com ANEM_N_FUROS no firmware.
FUROS_POR_VOLTA = 5


def carregar(caminho):
    """Lê o .jsonl e devolve listas paralelas: tempo(s), vel, rpm, pulsos."""
    registros = []
    with open(caminho, "r", encoding="utf-8") as f:
        for linha in f:
            linha = linha.strip()
            if not linha:
                continue
            try:
                registros.append(json.loads(linha))
            except json.JSONDecodeError:
                continue  # ignora linha corrompida/incompleta

    if not registros:
        sys.exit(f"Nenhum dado válido em '{caminho}'. Rode o receptor.py primeiro.")

    # Eixo X: segundos decorridos desde a primeira leitura (mais legível que a
    # data absoluta). Se o timestamp falhar, cai para o número da amostra.
    tempos, velocidades, rpms, pulsos = [], [], [], []
    t0 = None
    for i, r in enumerate(registros):
        try:
            t = datetime.fromisoformat(r["timestamp"])
            if t0 is None:
                t0 = t
            x = (t - t0).total_seconds()
        except (KeyError, ValueError):
            x = float(i)
        tempos.append(x)
        velocidades.append(r.get("velocidade_kmh", 0.0))
        rpms.append(r.get("rpm", 0))
        pulsos.append(r.get("pulsos", 0))

    return tempos, velocidades, rpms, pulsos


def recortar_ativo(tempos, velocidades, rpms, pulsos):
    """Corta os trechos de vento zero, mantendo só a(s) rajada(s) de verdade.

    Agrupa as amostras com pulso em 'eventos' (tolerando lacunas curtas) e
    descarta blips insignificantes (ex.: um toque isolado na hélice no
    começo). Assim um pico real no fim não fica espremido por causa de um
    ruído no início. Devolve as listas recortadas."""
    ativos = [i for i, p in enumerate(pulsos) if p > 0]
    if not ativos:
        return tempos, velocidades, rpms, pulsos  # sem movimento: mostra tudo

    GAP = 30  # amostras (~30 s) de zero que ainda contam como o mesmo evento

    # Agrupa índices ativos em eventos contíguos (com tolerância de lacuna).
    eventos = []
    grupo = [ativos[0]]
    for idx in ativos[1:]:
        if idx - grupo[-1] <= GAP:
            grupo.append(idx)
        else:
            eventos.append(grupo)
            grupo = [idx]
    eventos.append(grupo)

    # Mantém só os eventos relevantes (>= 2% do total de pulsos contados).
    massa = lambda g: sum(pulsos[i] for i in g)
    total = sum(pulsos)
    relevantes = [g for g in eventos if massa(g) >= 0.02 * total]
    if not relevantes:                       # tudo pequeno: fica com o maior
        relevantes = [max(eventos, key=massa)]

    inicio_evt = relevantes[0][0]
    fim_evt = relevantes[-1][-1]

    span = fim_evt - inicio_evt
    margem = max(5, int(span * 0.05))        # ~5% de folga de cada lado
    ini = max(0, inicio_evt - margem)
    fim = min(len(pulsos), fim_evt + 1 + margem)

    return (tempos[ini:fim], velocidades[ini:fim],
            rpms[ini:fim], pulsos[ini:fim])


def main():
    # Separa flags (--completo) dos argumentos posicionais (arquivos).
    posicionais = [a for a in sys.argv[1:] if not a.startswith("--")]
    flags = {a.lower() for a in sys.argv[1:] if a.startswith("--")}
    completo = "--completo" in flags or "--full" in flags

    entrada = posicionais[0] if len(posicionais) > 0 else ARQUIVO_ENTRADA
    saida = posicionais[1] if len(posicionais) > 1 else ARQUIVO_SAIDA

    tempos, velocidades, rpms, pulsos = carregar(entrada)
    total_amostras = len(tempos)

    if not completo:
        tempos, velocidades, rpms, pulsos = recortar_ativo(
            tempos, velocidades, rpms, pulsos)
        if len(tempos) < total_amostras:
            print(f"Focando na região ativa: {len(tempos)} de {total_amostras} "
                  f"amostras (use --completo para ver tudo).")

    n = len(velocidades)
    vmax = max(velocidades)
    vmed = sum(velocidades) / n
    rpmmax = max(rpms)
    pmax = max(pulsos)
    ptotal = sum(pulsos)

    # Muitos pontos -> tira os marcadores e afina a linha (evita "borrão").
    marcador = "." if n <= 150 else None
    lw = 1.2 if n <= 150 else 0.9

    # Eixo X em minutos quando a medição é longa (mais legível que milhares de s).
    if tempos and max(tempos) >= 300:
        tx = [t / 60.0 for t in tempos]
        rotulo_x = "Tempo (min)"
    else:
        tx = tempos
        rotulo_x = "Tempo (s)"

    fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(10, 9), sharex=True)
    titulo = "Anemômetro STM32 — Medições"
    if not completo and n < total_amostras:
        titulo += " (região ativa)"
    fig.suptitle(titulo, fontsize=14, fontweight="bold")

    # --- 1) Velocidade ---
    ax1.plot(tx, velocidades, color="#1f77b4", marker=marcador, linewidth=lw,
             label="Velocidade (km/h)")
    ax1.axhline(vmed, color="#ff7f0e", linestyle="--", linewidth=1,
                label=f"Média = {vmed:.2f} km/h")
    ax1.set_ylabel("Velocidade (km/h)")
    ax1.grid(True, alpha=0.3)
    ax1.legend(loc="upper right")
    ax1.text(0.01, 0.95,
             f"Amostras: {n}\nMáx: {vmax:.2f} km/h\nMédia: {vmed:.2f} km/h",
             transform=ax1.transAxes, va="top", fontsize=9,
             bbox=dict(boxstyle="round", facecolor="white", alpha=0.8))

    # --- 2) RPM ---
    ax2.plot(tx, rpms, color="#2ca02c", marker=marcador, linewidth=lw, label="RPM")
    ax2.set_ylabel("RPM")
    ax2.grid(True, alpha=0.3)
    ax2.legend(loc="upper right")
    ax2.text(0.01, 0.95, f"RPM máx: {rpmmax}",
             transform=ax2.transAxes, va="top", fontsize=9,
             bbox=dict(boxstyle="round", facecolor="white", alpha=0.8))

    # --- 3) Pulsos por segundo (dado mais cru), em onda quadrada (degraus) ---
    ax3.step(tx, pulsos, where="post", color="#9467bd", linewidth=1.4,
             label="Pulsos/s (bruto)")
    ax3.fill_between(tx, pulsos, step="post", color="#9467bd", alpha=0.15)
    ax3.set_ylabel("Pulsos por segundo")
    ax3.set_xlabel(rotulo_x)
    ax3.grid(True, alpha=0.3)
    ax3.legend(loc="upper right")
    ax3.text(0.01, 0.95, f"Pico: {pmax} pulsos/s\nTotal contado: {ptotal} pulsos",
             transform=ax3.transAxes, va="top", fontsize=9,
             bbox=dict(boxstyle="round", facecolor="white", alpha=0.8))

    # Eixo secundário (direita): voltas/s = pulsos / 20 (20 furos = 1 volta)
    eixo_voltas = ax3.secondary_yaxis(
        "right",
        functions=(lambda p: p / FUROS_POR_VOLTA, lambda v: v * FUROS_POR_VOLTA))
    eixo_voltas.set_ylabel(f"Voltas/s ({FUROS_POR_VOLTA} pulsos = 1 volta)")

    fig.tight_layout()
    fig.savefig(saida, dpi=150)
    print(f"Gráfico salvo em '{saida}' ({n} amostras).")

    plt.show()


if __name__ == "__main__":
    main()
