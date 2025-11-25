import serial
import json
import paho.mqtt.client as mqtt

SERIAL_PORT = "/dev/ttyACM0"
BAUD = 115200

MQTT_BROKER = "localhost"
MQTT_TOPIC = "samr21/sensors"

ser = serial.Serial(SERIAL_PORT, BAUD)

client = mqtt.Client()
client.connect(MQTT_BROKER)

print("SAMR21 → USB → MQTT rodando...")

while True:
    line = ser.readline().decode(errors="ignore").strip()
    print("[SERIAL]", line)  # DEBUG

    if not line.startswith("{"):
        continue

    try:
        data = json.loads(line)
        print("Publicando:", data)
        client.publish(MQTT_TOPIC, json.dumps(data))
    except Exception as e:
        print("Erro:", e)
