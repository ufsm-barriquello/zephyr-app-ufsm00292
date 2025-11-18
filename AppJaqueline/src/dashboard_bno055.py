import serial
import threading
from collections import deque
from dash import Dash, html, dcc
import plotly.graph_objs as go
from dash.dependencies import Input, Output

# ---- CONFIGURAÇÃO SERIAL ----
SERIAL_PORT = "COM3"  # troque pela porta correta
BAUD_RATE = 115200

# Abre a serial com timeout para não travar
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)

# ---- BUFFERS PARA ARMAZENAR OS DADOS ----
buffer_size = 200

euler_h = deque(maxlen=buffer_size)
euler_r = deque(maxlen=buffer_size)
euler_p = deque(maxlen=buffer_size)

acc_x = deque(maxlen=buffer_size)
acc_y = deque(maxlen=buffer_size)
acc_z = deque(maxlen=buffer_size)

gyro_x = deque(maxlen=buffer_size)
gyro_y = deque(maxlen=buffer_size)
gyro_z = deque(maxlen=buffer_size)

temp_c = [0]

# ---- THREAD PARA LEITURA SERIAL ----
def serial_thread():
    while True:
        try:
            line = ser.readline().decode(errors='ignore').strip()
            parts = line.split(",")

            if len(parts) < 2:
                continue

            if parts[0] == "EULER":
                euler_h.append(int(parts[1]))
                euler_r.append(int(parts[2]))
                euler_p.append(int(parts[3]))

            elif parts[0] == "ACCEL":
                acc_x.append(int(parts[1]))
                acc_y.append(int(parts[2]))
                acc_z.append(int(parts[3]))

            elif parts[0] == "GYRO":
                gyro_x.append(int(parts[1]))
                gyro_y.append(int(parts[2]))
                gyro_z.append(int(parts[3]))

            elif parts[0] == "MAG":
                # Se quiser adicionar magnetômetro
                pass

            elif parts[0] == "TEMP":
                temp_c[0] = int(parts[1])

        except Exception as e:
            # Apenas ignora linhas mal formatadas
            print("Erro ao ler serial:", e)
            continue

threading.Thread(target=serial_thread, daemon=True).start()

# ---- DASHBOARD (WEB) ----
app = Dash(__name__)

app.layout = html.Div(children=[
    html.H1("BNO055 Dashboard em Tempo Real", style={"text-align": "center"}),

    dcc.Graph(id="euler_graph"),
    dcc.Graph(id="accel_graph"),
    dcc.Graph(id="gyro_graph"),
    dcc.Graph(id="temp_gauge"),

    dcc.Interval(
        id="interval",
        interval=500,  # Atualização a cada 500ms
        n_intervals=0
    )
])

# ---- CALLBACKS ----
@app.callback(
    Output("euler_graph", "figure"),
    Output("accel_graph", "figure"),
    Output("gyro_graph", "figure"),
    Output("temp_gauge", "figure"),
    Input("interval", "n_intervals")
)
def update_graph(n):
    # Euler
    fig_euler = go.Figure()
    fig_euler.add_trace(go.Scatter(y=list(euler_h), name="Heading"))
    fig_euler.add_trace(go.Scatter(y=list(euler_r), name="Roll"))
    fig_euler.add_trace(go.Scatter(y=list(euler_p), name="Pitch"))
    fig_euler.update_layout(title="Euler Angles (°)")

    # Aceleração
    fig_acc = go.Figure()
    fig_acc.add_trace(go.Scatter(y=list(acc_x), name="Ax"))
    fig_acc.add_trace(go.Scatter(y=list(acc_y), name="Ay"))
    fig_acc.add_trace(go.Scatter(y=list(acc_z), name="Az"))
    fig_acc.update_layout(title="Aceleração (mg)")

    # Giroscópio
    fig_gyro = go.Figure()
    fig_gyro.add_trace(go.Scatter(y=list(gyro_x), name="Gx"))
    fig_gyro.add_trace(go.Scatter(y=list(gyro_y), name="Gy"))
    fig_gyro.add_trace(go.Scatter(y=list(gyro_z), name="Gz"))
    fig_gyro.update_layout(title="Giroscópio (°/s)")

    # Temperatura Gauge
    fig_temp = go.Figure(go.Indicator(
        mode="gauge+number",
        value=temp_c[0],
        title={"text": "Temperatura (°C)"},
        gauge={
            "axis": {"range": [-10, 60]},
            "bar": {"color": "red"}
        }
    ))

    return fig_euler, fig_acc, fig_gyro, fig_temp

# ---- INICIAR SERVIDOR ----
if __name__ == "__main__":
    print("Dashboard rodando em: http://127.0.0.1:8050")
    app.run(debug=True)
