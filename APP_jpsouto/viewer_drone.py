import json
import numpy as np
import serial
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from mpl_toolkits.mplot3d.art3d import Poly3DCollection

# Porta serial
ser = serial.Serial("/dev/ttyACM0", 115200, timeout=1)

# ==========================
# Quaternion → Matriz
# ==========================
def quat_to_rot_matrix(qw, qx, qy, qz):
    r11 = 1 - 2*(qy*qy + qz*qz)
    r12 = 2*(qx*qy - qz*qw)
    r13 = 2*(qx*qz + qy*qw)

    r21 = 2*(qx*qy + qz*qw)
    r22 = 1 - 2*(qx*qx + qz*qz)
    r23 = 2*(qy*qz - qx*qw)

    r31 = 2*(qx*qz - qy*qw)
    r32 = 2*(qy*qz + qx*qw)
    r33 = 1 - 2*(qx*qx + qy*qy)

    return np.array([[r11,r12,r13],[r21,r22,r23],[r31,r32,r33]])


# ==========================
# Modelo 3D – Drone Quadricóptero
# ==========================

def build_drone_model():
    vertices = []
    faces = []

    # Corpo central (quadrado)
    body = np.array([
        [-0.5, -0.5, 0],
        [ 0.5, -0.5, 0],
        [ 0.5,  0.5, 0],
        [-0.5,  0.5, 0]
    ])
    
    body_top = body + np.array([0,0,0.1])

    # Faces do corpo
    faces.extend([
        [0,1,2], [0,2,3],          # bottom
        [4,5,6], [4,6,7],          # top
        [0,1,5], [0,5,4],          # side
        [1,2,6], [1,6,5],
        [2,3,7], [2,7,6],
        [3,0,4], [3,4,7],
    ])

    vertices.extend(body.tolist() + body_top.tolist())

    base_idx = 8  # next index

    # Braços (4 cilindros simplificados)
    arm_length = 2.0
    motor_size = 0.3

    arms = []
    motors = []
    propellers = []

    arm_dirs = [
        np.array([ 1,  1, 0]),
        np.array([-1,  1, 0]),
        np.array([-1, -1, 0]),
        np.array([ 1, -1, 0])
    ]

    arm_dirs = [d/np.linalg.norm(d) for d in arm_dirs]

    for d in arm_dirs:
        # ponta do braço
        p1 = d * 0.6
        p2 = d * arm_length

        # adicionar vertices do braço
        v_start = np.array([-0.1, -0.1, 0])
        v_end   = np.array([ 0.1,  0.1, 0])

        arm_vertices = [
            p1 + v_start, p1 + v_end,
            p1 - v_start, p1 - v_end,
            p2 + v_start, p2 + v_end,
            p2 - v_start, p2 - v_end
        ]

        start_idx = len(vertices)
        vertices.extend([v.tolist() for v in arm_vertices])

        # faces do braço (retângulo extrudado)
        faces.extend([
            [start_idx,   start_idx+1, start_idx+5],
            [start_idx,   start_idx+5, start_idx+4],
            [start_idx+1, start_idx+3, start_idx+7],
            [start_idx+1, start_idx+7, start_idx+5],
            [start_idx+3, start_idx+2, start_idx+6],
            [start_idx+3, start_idx+6, start_idx+7],
            [start_idx+2, start_idx,   start_idx+4],
            [start_idx+2, start_idx+4, start_idx+6]
        ])

        # Motor (cilindro simples)
        motor_center = p2 + np.array([0,0,0.2])
        m1 = motor_center + np.array([-motor_size, -motor_size, 0])
        m2 = motor_center + np.array([ motor_size, -motor_size, 0])
        m3 = motor_center + np.array([ motor_size,  motor_size, 0])
        m4 = motor_center + np.array([-motor_size,  motor_size, 0])
        mtop = motor_center + np.array([0,0,0.2])

        idx_m = len(vertices)
        vertices.extend([m1.tolist(), m2.tolist(), m3.tolist(), m4.tolist(), mtop.tolist()])

        # Faces do motor
        faces.extend([
            [idx_m, idx_m+1, idx_m+4],
            [idx_m+1, idx_m+2, idx_m+4],
            [idx_m+2, idx_m+3, idx_m+4],
            [idx_m+3, idx_m,   idx_m+4]
        ])

    return np.array(vertices), faces


vertices, faces = build_drone_model()

# ==========================
# Matplotlib Config
# ==========================

fig = plt.figure()
ax = fig.add_subplot(projection='3d')
ax.set_xlim([-3, 3])
ax.set_ylim([-3, 3])
ax.set_zlim([-3, 3])

ax.set_xlabel("X")
ax.set_ylabel("Y")
ax.set_zlabel("Z")

poly = None

# ==========================
# UPDATE
# ==========================

def update(frame):
    global poly

    raw = ser.readline().decode(errors="ignore").strip()

    if raw == "" or not raw.startswith("{"):
        return poly

    try:
        data = json.loads(raw)
    except:
        return poly

    qw, qx, qy, qz = data["qw"], data["qx"], data["qy"], data["qz"]
    R = quat_to_rot_matrix(qw, qx, qy, qz)

    rotated = (R @ vertices.T).T

    tris = []
    for f in faces:
        tris.append([rotated[f[0]], rotated[f[1]], rotated[f[2]]])

    if poly:
        poly.remove()

    poly = Poly3DCollection(
        tris,
        facecolors="lightgray",
        edgecolors="black",
        linewidths=0.3,
        alpha=0.9
    )

    ax.add_collection3d(poly)
    return poly


ani = FuncAnimation(fig, update, interval=50)
plt.show()
