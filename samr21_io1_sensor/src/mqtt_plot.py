import json
import paho.mqtt.client as mqtt
import matplotlib.pyplot as plt
from datetime import datetime
import csv

MQTT_BROKER = "localhost"
MQTT_TOPIC  = "samr21/sensors"

times = []
temps = []
lights = []

def on_message(client, userdata, msg):
    global times, temps, lights

    data = json.loads(msg.payload.decode())

    t = datetime.now()
    temp = data["temp"]
    light = data["light_pct"]

    print(f"[MQTT] {t}  Temp={temp:.2f}C  Light={light:.1f}%")

    times.append(t)
    temps.append(temp)
    lights.append(light)

    # salva em CSV
    with open("leituras.csv", "a", newline="") as f:
        csv.writer(f).writerow([t, temp, light])

    # atualiza gráfico
    plt.clf()
    plt.subplot(2,1,1)
    plt.plot(times, temps, label="Temperatura (°C)")
    plt.legend()

    plt.subplot(2,1,2)
    plt.plot(times, lights, label="Luminosidade (%)", color="orange")
    plt.legend()

    plt.pause(0.01)

client = mqtt.Client()
client.on_message = on_message

client.connect(MQTT_BROKER)
client.subscribe(MQTT_TOPIC)

print("📈 Coletando dados MQTT… Salvando em leituras.csv…")

plt.ion()
plt.show()

client.loop_forever()
