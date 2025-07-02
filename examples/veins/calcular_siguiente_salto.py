import sys
import math
import re

print("=== Python recibio los vecinos ===", file=sys.stderr)

lines = list(sys.stdin)
for line in lines:
    print(f"[VECINO RECIBIDO] {line.strip()}", file=sys.stderr)



# Leer parámetros
origen = int(sys.argv[1])
destino = int(sys.argv[2])
dest_x = float(sys.argv[3])
dest_y = float(sys.argv[4])
destino_coord = (dest_x, dest_y)

# Leer vecinos desde stdin
vecinos = {}

for line in lines:
    if "ID:" in line:
        try:
            match = re.search(r'ID:(\d+)\s+Coord:\(([^,]+),([^,]+),', line)
            if match:
                id_vehiculo = int(match.group(1))
                x = float(match.group(2))
                y = float(match.group(3))
                vecinos[id_vehiculo] = (x, y)
        except Exception as e:
            print(f"[ERROR] {line.strip()} -> {e}", file=sys.stderr)

# Función de distancia
def distancia(a, b):
    return math.sqrt((a[0] - b[0])**2 + (a[1] - b[1])**2)

# Elegir mejor vecino
if vecinos:
    next_hop = min(vecinos.items(), key=lambda v: distancia(v[1], destino_coord))[0]
    print(next_hop)  # <- Lo leerá OMNeT++
else:
    print("-1")  # Si no hay vecinos

