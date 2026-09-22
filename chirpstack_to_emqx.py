import struct
import json
import base64
import paho.mqtt.client as mqtt

# Broker ChirpStack
CHIRPSTACK_BROKER = "localhost"
CHIRPSTACK_PORT = 1883
CHIRPSTACK_TOPIC = "application/+/device/+/event/up"
CHIRPSTACK_USER = ""
CHIRPSTACK_PASS = ""

# Broker EMQX Docker
EMQX_BROKER = "localhost"
EMQX_PORT = 1884
EMQX_USER = ""              # Kalau mau pakai autentikasi
EMQX_PASS = ""

emqx_client = mqtt.Client(client_id="ChirpStack_Forwarder_Python")

def decode_binary_payload(payload_bytes):
    if len(payload_bytes) < 11:
        print(f"[Error] Payload kurang dari 11 byte! Terbaca: {len(payload_bytes)} byte")
        return None

    raw_id = payload_bytes[0]
    temp_raw, hum_raw, co2_raw, co_raw, no2_raw = struct.unpack(">hHHHH", payload_bytes[1:11])

    return {
        "id": raw_id,
        "temperature": round(temp_raw / 10.0, 1),
        "humidity": round(hum_raw / 10.0, 1),
        "co2": co2_raw,
        "co": round(co_raw / 10.0, 1),
        "no2": round(no2_raw / 100.0, 2)
    }


def send_to_emqx(dev_eui, sensor_data):
    if not sensor_data:
        return

    topic_mapping = {
        "temperature": "/sensor/temp",
        "humidity": "/sensor/humidity",
        "co2": "/sensor/co2",
        "co": "/sensor/co",
        "no2": "/sensor/no2"
    }

    print(f"\n[EMQX] Mempublikasikan data dari Device ({dev_eui}) per topik:")

    for key, topic in topic_mapping.items():
        if key in sensor_data:
            val = sensor_data[key]
            
            payload = json.dumps({
                "value": val,
                "dev_eui": dev_eui
            })

            res = emqx_client.publish(topic, payload, qos=1)
            
            if res.rc == mqtt.MQTT_ERR_SUCCESS:
                print(f"  ✔ Topik '{topic}': {payload}")
            else:
                print(f"  ✖ Gagal kirim ke '{topic}' (Error code: {res.rc})")

def on_chirpstack_connect(client, userdata, flags, rc):
    if rc == 0:
        print(f"[ChirpStack] Terhubung ke Broker ChirpStack (Port {CHIRPSTACK_PORT})!")
        client.subscribe(CHIRPSTACK_TOPIC)
        print(f"[ChirpStack] Subscribed ke topik: {CHIRPSTACK_TOPIC}")
    else:
        print(f"[ChirpStack] Gagal terhubung, kode error: {rc}")

def on_chirpstack_message(client, userdata, msg):
    try:
        chirpstack_json = json.loads(msg.payload.decode('utf-8'))
        
        dev_eui = chirpstack_json.get("deviceInfo", {}).get("devEui", "unknown_device")
        dev_name = chirpstack_json.get("deviceInfo", {}).get("deviceName", "Unknown")
        
        print("\n==================================================")
        print(f"[Event] Paket Diterima dari Device: {dev_name} ({dev_eui})")

        sensor_data = None

        if "data" in chirpstack_json:
            raw_bytes = base64.b64decode(chirpstack_json["data"])
            sensor_data = decode_binary_payload(raw_bytes)
        elif "object" in chirpstack_json:
            sensor_data = chirpstack_json["object"]

        if sensor_data:
            send_to_emqx(dev_eui, sensor_data)

    except Exception as e:
        print(f"[Error] Gagal memproses pesan: {e}")

if __name__ == "__main__":
    if EMQX_USER and EMQX_PASS:
        emqx_client.username_pw_set(EMQX_USER, EMQX_PASS)

    try:
        emqx_client.connect(EMQX_BROKER, EMQX_PORT, 60)
        emqx_client.loop_start()
        print(f"[EMQX] Terhubung ke EMQX Broker Docker pada port {EMQX_PORT}!")
    except Exception as e:
        print(f"[EMQX Error] Gagal koneksi ke EMQX Docker: {e}")

    chirpstack_client = mqtt.Client(client_id="ChirpStack_Subscriber_Python")
    chirpstack_client.on_connect = on_chirpstack_connect
    chirpstack_client.on_message = on_chirpstack_message

    if CHIRPSTACK_USER and CHIRPSTACK_PASS:
        chirpstack_client.username_pw_set(CHIRPSTACK_USER, CHIRPSTACK_PASS)

    print("[System] Menjalankan Forwarder ChirpStack -> EMQX...")
    chirpstack_client.connect(CHIRPSTACK_BROKER, CHIRPSTACK_PORT, 60)
    chirpstack_client.loop_forever()