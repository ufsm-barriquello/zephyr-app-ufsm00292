import json
import numpy as np
import serial
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

# ============================================================
# CONFIGURAÇÃO DA SERIAL
# Ajuste para a porta da sua SAMR21
# No Windows: "COM3", "COM4" ...
# No Linux: "/dev/ttyACM0"
# ============================================================

ser = serial.Serial("COM4", 115200, timeout=1)

# ============================================================
# FUNÇÕES DE CONVERSÃO QUATERNION → MATRIZ DE ROTAÇÃO
# ============================================================

def quat_to_rot_matrix(qw, qx, qy, qz):
    """
    Converte quaternion normalizado para matriz 3x3
    """
    r11 = 1 - 2*(qy**2 + qz**2)
    r12 = 2*(qx*qy - qz*qw)
    r13 = 2*(qx*qz + qy*qw)

    r21 = 2*(qx*qy + qz*qw)
    r22 = 1 - 2*(qx**2 + qz**2)
    r23 = 2*(qy*qz - qx*qw)

    r31 = 2*(qx*qz - qy*qw)
    r32 = 2*(qy*qz + qx*qw)
    r33 = 1 - 2*(qx**2 + qy**2)

    return np.array([[r11, r12, r13],
                     [r21, r22, r23],
                     [r31, r32, r33]])

# ============================================================
# CUBO 3D BASE
# ============================================================

cube_definition = np.array([
    [-1, -1, -1],
    [-1, -1,  1],
    [-1,  1,  1],
    [-1,  1, -1],
    [ 1, -1, -1],
    [ 1, -1,  1],
    [ 1,  1,  1],
    [ 1,  1, -1]
], dtype=float)

edges = [
    (0,1), (1,2), (2,3), (3,0),
    (4,5), (5,6), (6,7), (7,4),
    (0,4), (1,5), (2,6), (3,7)
]

# ============================================================
# MATPLOTLIB CONFIG
# ============================================================

fig = plt.figure()
ax = fig.add_subplot(projection='3d')

ax.set_xlim([-2, 2])
ax.set_ylim([-2, 2])
ax.set_zlim([-2, 2])

ax.set_xlabel('X')
ax.set_ylabel('Y')
ax.set_zlabel('Z')

lines = []
for edge in edges:
    line, = ax.plot([], [], [], 'b')
    lines.append(line)

# ============================================================
# FUNÇÃO DE ATUALIZAÇÃO
# ============================================================

def update(frame):
    try:
        line = ser.readline().decode().strip()
        if not line:
            return lines

        data = json.loads(line)

        qw = data["qw"]
        qx = data["qx"]
        qy = data["qy"]
        qz = data["qz"]

        R = quat_to_rot_matrix(qw, qx, qy, qz)
        
        rotated = (R @ cube_definition.T).T

        for idx, edge in enumerate(edges):
            p1 = rotated[edge[0]]
            p2 = rotated[edge[1]]
            lines[idx].set_data([p1[0], p2[0]], [p1[1], p2[1]])
            lines[idx].set_3d_properties([p1[2], p2[2]])

    except Exception as e:
        print("Erro:", e)

    return lines

ani = FuncAnimation(fig, update, interval=50)
plt.show()
