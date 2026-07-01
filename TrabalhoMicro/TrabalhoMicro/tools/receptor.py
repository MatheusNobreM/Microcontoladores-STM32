#!/usr/bin/env python3
"""
Receptor serial do anemômetro STM32 (Blue Pill).

Lê os frames BINÁRIOS crus enviados pela UART/TTL (sem texto/printf) e
grava cada leitura persistida em JSON Lines (um objeto JSON por linha),
que é o formato ideal para dados que chegam continuamente.

Formato do frame (9 bytes, little-endian):

    [0] 0xAA          -> sync 0
    [1] 0x55          -> sync 1
    [2..3] pulsos     -> uint16
    [4..5] rpm        -> uint16
    [6..7] vel_kmh_x100 -> uint16  (km/h * 100)
    [8] checksum      -> XOR dos bytes [2..7]

Uso:
    python receptor.py [PORTA] [BAUD]
    ex.: python receptor.py COM3 9600

Dependência:
    pip install pyserial
"""

import sys
import json
import struct
from datetime import datetime, timezone

try:
    import serial  # pyserial
except ImportError:
    sys.exit("Falta a dependência 'pyserial'. Instale com: pip install pyserial")

SYNC0 = 0xAA
SYNC1 = 0x55
TAM_PAYLOAD = 6          # pulsos(2) + rpm(2) + vel(2)
ARQUIVO_SAIDA = "dados_anemometro.jsonl"


def ler_frame(ser):
    """Sincroniza em 0xAA 0x55, valida o checksum e devolve (pulsos, rpm, vel_x100).
    Retorna None quando a porta expira sem dados (timeout)."""
    while True:
        b = ser.read(1)
        if not b:
            return None                     # timeout
        if b[0] != SYNC0:
            continue                        # procura o sync 0
        b2 = ser.read(1)
        if not b2 or b2[0] != SYNC1:
            continue                        # sync 1 não veio: ressincroniza

        payload = ser.read(TAM_PAYLOAD)
        chk = ser.read(1)
        if len(payload) != TAM_PAYLOAD or len(chk) != 1:
            return None                     # frame incompleto (timeout)

        calc = 0
        for x in payload:
            calc ^= x
        if calc != chk[0]:
            continue                        # corrompido: descarta e ressincroniza

        pulsos, rpm, vel_x100 = struct.unpack("<HHH", payload)
        return pulsos, rpm, vel_x100


def main():
    porta = sys.argv[1] if len(sys.argv) > 1 else "COM3"
    baud = int(sys.argv[2]) if len(sys.argv) > 2 else 9600

    print(f"Abrindo {porta} @ {baud} bps -> gravando em '{ARQUIVO_SAIDA}'")
    print("Ctrl+C para encerrar.\n")

    with serial.Serial(porta, baud, timeout=2) as ser, \
         open(ARQUIVO_SAIDA, "a", encoding="utf-8") as f:
        while True:
            frame = ler_frame(ser)
            if frame is None:
                continue

            pulsos, rpm, vel_x100 = frame
            registro = {
                "timestamp": datetime.now(timezone.utc).isoformat(),
                "pulsos": pulsos,
                "rpm": rpm,
                "velocidade_kmh": vel_x100 / 100.0,
            }

            f.write(json.dumps(registro, ensure_ascii=False) + "\n")
            f.flush()  # garante persistência imediata a cada leitura

            print(f"{registro['timestamp']}  "
                  f"pulsos={pulsos:5d}  rpm={rpm:5d}  "
                  f"vel={registro['velocidade_kmh']:6.2f} km/h")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nEncerrado.")
    except serial.SerialException as e:
        sys.exit(f"Erro na porta serial: {e}")
