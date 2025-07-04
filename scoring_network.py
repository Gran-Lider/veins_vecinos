import numpy as np

# Parámetros de simulación
N_samples   = 20000        # cuántos orígenes-destino simulo
max_vecinos = 8            # número máximo plausible de vecinos en tu red
X, y = [], []              # listas para features y labels

for _ in range(N_samples):
    # Genero origen y destino
    origen  = np.random.rand(2)*1000
    destino = np.random.rand(2)*1000

    # Número aleatorio de vecinos entre 1 y max_vecinos
    n_vec = np.random.randint(1, max_vecinos+1)
    coords = np.random.rand(n_vec, 2)*1000

    # Calculo distancias al destino, elijo el índice ganador
    dists = np.linalg.norm(coords - destino, axis=1)
    ganador = np.argmin(dists)

    # Para cada vecino, creo una muestra (features, label)
    for i, (vx, vy) in enumerate(coords):
        dx, dy = vx - origen[0], vy - origen[1]
        vx2, vy2 = destino[0] - origen[0], destino[1] - origen[1]
        dot      = dx*vx2 + dy*vy2                    # opcional
        
        feats = [dx, dy, vx2, vy2, dot]               # escoge las features que quieras
        X.append(feats)
        y.append(1 if i == ganador else 0)            # 1 si es el elegido, 0 resto

X = np.array(X, dtype=np.float32)
y = np.array(y, dtype=np.float32)

