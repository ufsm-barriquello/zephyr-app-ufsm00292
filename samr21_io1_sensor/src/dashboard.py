import tkinter as tk
import json
import paho.mqtt.client as mqtt

MQTT_BROKER = "localhost"
MQTT_TOPIC  = "samr21/sensors"

root = tk.Tk()
root.title("Dashboard SAMR21 - Temperatura e Luz")
root.geometry("350x180")
root.configure(bg="#DDDDDD")

temp_var = tk.StringVar(value="--- °C")
light_var = tk.StringVar(value="--- %")

tk.Label(root, text="Temperatura:", font=("Arial", 16), bg="#DDDDDD").pack(pady=5)
tk.Label(root, textvariable=temp_var, font=("Arial", 24, "bold"), bg="#DDDDDD").pack()

tk.Label(root, text="Luminosidade:", font=("Arial", 16), bg="#DDDDDD").pack(pady=5)
tk.Label(root, textvariable=light_var, font=("Arial", 24, "bold"), bg="#DDDDDD").pack()

def on_message(client, userdata, msg):
    data = json.loads(msg.payload.decode())
    temp_var.set(f"{data['temp']:.2f} °C")
    light_var.set(f"{data['light_pct']:.1f} %")

client = mqtt.Client()
client.on_message = on_message
client.connect(MQTT_BROKER)
client.subscribe(MQTT_TOPIC)

def mqtt_loop():
    client.loop(timeout=0.1)
    root.after(100, mqtt_loop)

mqtt_loop()
root.mainloop()
