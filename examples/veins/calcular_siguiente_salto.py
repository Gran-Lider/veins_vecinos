import sys
import re
import torch
import torch.nn as nn

# === CONFIGURACIÓN ===
MAX_VECINOS = 5
PKL_PATH = "modelo_nexthop_k5.pkl"
INPUT_SIZE = 2 * MAX_VECINOS + 2
NUM_CLASSES = MAX_VECINOS

# === MODELO ENTRENADO ===
class NextHopNN(nn.Module):
    def __init__(self, input_size, num_classes):
        super().__init__()
        self.net = nn.Sequential(
            nn.Linear(input_size, 32),
            nn.ReLU(),
            nn.Linear(32, 16),
            nn.ReLU(),
            nn.Linear(16, num_classes)
        )

    def forward(self, x):
        return self.net(x)

# === DEBUG ===
print("=== Python recibió los vecinos ===", file=sys.stderr)
lines = list(sys.stdin)
for line in lines:
    print(f"[VECINO RECIBIDO] {line.strip()}", file=sys.stderr)

# === ARGUMENTOS ===
origen   = int(sys.argv[1])
destino  = int(sys.argv[2])
dest_x   = float(sys.argv[3])
dest_y   = float(sys.argv[4])
destino_coord = (dest_x, dest_y)

prevHop = None
if len(sys.argv) > 5:
    prevHop = int(sys.argv[5])
    print(f"[INFO] PrevHop recibido: {prevHop}", file=sys.stderr)

# === PARSEAR VECINOS ===
vecinos = {}
for line in lines:
    match = re.search(r'ID:(\d+)\s+Coord:\(([^,]+),([^,]+),', line)
    if match:
        idv = int(match.group(1))
        x   = float(match.group(2))
        y   = float(match.group(3))
        vecinos[idv] = (x, y)

if prevHop is not None and prevHop in vecinos:
    del vecinos[prevHop]
    print(f"[INFO] Eliminado prevHop {prevHop} de candidatos", file=sys.stderr)

# === LISTA ORDENADA DE VECINOS ===
vecinos_items = list(vecinos.items())
ids_vecinos = [v[0] for v in vecinos_items]
coords_vecinos = [v[1] for v in vecinos_items]

# Rellenar con (0.0, 0.0) si hay menos de MAX_VECINOS
while len(coords_vecinos) < MAX_VECINOS:
    coords_vecinos.append((0.0, 0.0))
    ids_vecinos.append(-1)  # ID inválido para vecinos vacíos

# Si no hay ningún vecino real, devolvemos -1
if all(x == (0.0, 0.0) for x in coords_vecinos):
    print(-1)
    sys.exit()

# === INPUT PARA LA RED NEURONAL ===
features = []
for coord in coords_vecinos:
    features.extend([coord[0], coord[1]])
features.extend([dest_x, dest_y])
input_tensor = torch.tensor([features], dtype=torch.float32)

# === CARGAR MODELO Y PREDECIR ===
model = NextHopNN(INPUT_SIZE, NUM_CLASSES)
model.load_state_dict(torch.load(PKL_PATH, map_location=torch.device('cpu')))
model.eval()

with torch.no_grad():
    pred_idx = model(input_tensor).argmax().item()

# === MAPEAR A ID DEL VECINO ===
next_hop = ids_vecinos[pred_idx]
print(next_hop)


