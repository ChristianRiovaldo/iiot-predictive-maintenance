#!/usr/bin/env python3
import base64
import json
import os
import sys

import paho.mqtt.client as mqtt

# ==============================================================================
# KONFIGURASI MAPPING NODE & TOPIK EMQX
# ==============================================================================
NODE_TOPIC_MAP = {
    "737fa3925c5c2bba": "/iiot/node1",
    "e02d6ee6c5f39cfd": "/iiot/node2",
    "b5770a367b5cba7d": "/iiot/node3",
}

# 1. ChirpStack MQTT Broker (Subscriber / Local Gateway)
CHIRPSTACK_BROKER = os.getenv("CHIRPSTACK_BROKER", "localhost")
CHIRPSTACK_PORT = int(os.getenv("CHIRPSTACK_PORT", "1883"))
CHIRPSTACK_TOPIC = "application/+/device/+/event/up"

# 2. EMQX MQTT Broker (Publisher Target / Remote Server)
EMQX_BROKER = os.getenv("EMQX_BROKER", "100.95.51.126")
EMQX_PORT = int(os.getenv("EMQX_PORT", "1883"))
EMQX_USER = os.getenv("EMQX_USER", "")
EMQX_PASS = os.getenv("EMQX_PASS", "")

CLIENT_ID_SUB = "chirpstack_all_nodes_sub"
CLIENT_ID_PUB = "emqx_all_nodes_pub"

sub_client = mqtt.Client(client_id=CLIENT_ID_SUB)
pub_client = mqtt.Client(client_id=CLIENT_ID_PUB)

# MAPPING SENSOR ID SESUAI C++ ENUM
ENV_SENSOR_MAP = {
    0: "ID_NONE",
    1: "ID_BME688",
    2: "ID_CO_I2C",
    3: "ID_PMS7003T",
    4: "ID_CO2_UART",
    5: "ID_H2S_SEN0568",
}


# ==============================================================================
# PARSER MODULAR SENSOR LINGKUNGAN
# ==============================================================================
def parse_env_sensor(sensor_id, val1, val2):
    if sensor_id == 1:  # ID_BME688
        return {
            "temperature": round(val1 / 10.0, 2),
            "humidity": round(val2 / 10.0, 2),
        }, "°C, %"
    elif sensor_id == 2:  # ID_CO_I2C
        return {
            "co_ppm": round(val1 / 10.0, 2),
        }, "PPM"
    elif sensor_id == 3:  # ID_PMS7003T
        return {
            "pm2_5": val1,
            "pm10": val2,
        }, "ug/m3"
    elif sensor_id == 4:  # ID_CO2_UART
        return {
            "co2": val1,
        }, "PPM"
    elif sensor_id == 5:  # ID_H2S_SEN0568
        return {
            "adc_raw": val1,
            "voltage": round(val2 / 100.0, 2),
        }, "V"
    else:  # ID_NONE (0)
        return {
            "status": "no_env_sensor",
        }, "-"


# ==============================================================================
# HELPER
# ==============================================================================
def decode_raw_payload(data_json):
    raw_data = data_json.get("data", "")
    if not raw_data:
        return b"", ""
    try:
        raw = base64.b64decode(raw_data)
        text = raw.decode("utf-8", errors="replace")
        return raw, text
    except Exception:
        return b"", ""


def is_printable_text(raw: bytes) -> bool:
    if not raw:
        return False
    for b in raw:
        if b in (9, 10, 13):
            continue
        if b < 32 or b > 126:
            return False
    return True


# ==============================================================================
# CALLBACK FUNCTIONS
# ==============================================================================
def on_sub_connect(client, userdata, flags, rc, properties=None):
    rc_value = getattr(rc, "value", rc)
    if rc_value == 0:
        print(f"[ChirpStack] Terhubung ke {CHIRPSTACK_BROKER}:{CHIRPSTACK_PORT}")
        client.subscribe(CHIRPSTACK_TOPIC, qos=1)
        print(f"[ChirpStack] Subscribed ke wildcard topik: {CHIRPSTACK_TOPIC}")
    else:
        print(f"[ChirpStack] Gagal terhubung, error code: {rc}")


def on_pub_connect(client, userdata, flags, rc, properties=None):
    rc_value = getattr(rc, "value", rc)
    if rc_value == 0:
        print(f"[EMQX] Terhubung ke {EMQX_BROKER}:{EMQX_PORT}")
    else:
        print(f"[EMQX] Gagal terhubung ke EMQX, error code: {rc}")


def on_chirpstack_message(client, userdata, msg):
    try:
        raw_payload = msg.payload.decode("utf-8")
        data_json = json.loads(raw_payload)

        dev_info = data_json.get("deviceInfo", {})
        dev_eui = dev_info.get("devEui", "").lower()
        dev_name = dev_info.get("deviceName", "unknown-node")
        f_cnt = data_json.get("fCnt", 0)
        f_port = data_json.get("fPort", 0)

        # Filter hanya DevEUI yang terdaftar
        if dev_eui not in NODE_TOPIC_MAP:
            return

        target_topic = NODE_TOPIC_MAP[dev_eui]
        decoded_object = data_json.get("object") or {}

        raw, raw_text = decode_raw_payload(data_json)
        hex_str = raw.hex()

        # ------------------------------------------------------------------
        # 1. Pesan teks (node2 "Hello from E77")
        # ------------------------------------------------------------------
        message = None
        if isinstance(decoded_object, dict):
            message = decoded_object.get("message")

        # Kalau codec ChirpStack belum aktif, decode data mentah sebagai teks
        if message is None and is_printable_text(raw):
            message = raw_text

        is_text_payload = (
            message is not None
            and not (isinstance(decoded_object, dict) and "sensor_id" in decoded_object)
        )

        if is_text_payload:
            emqx_payload = {
                "device_name": dev_name,
                "packet_id": f_cnt,
                "sensor_type": "TEXT",
                "val": {
                    "message": message,
                },
                "unit": "-",
                "fPort": f_port,
                "hex": decoded_object.get("hex", hex_str),
                "length": decoded_object.get("length", len(raw)),
            }
        else:
            # --------------------------------------------------------------
            # 2. Payload sensor lama (node1/node3)
            # --------------------------------------------------------------
            sensor_id = decoded_object.get("sensor_id", 0)
            val1 = decoded_object.get("val1", 0)
            val2 = decoded_object.get("val2", 0)
            voltage_v = round(float(decoded_object.get("voltage_v", 0.0)), 2)
            current_ma = round(float(decoded_object.get("current_ma", 0.0)), 1)

            val_data, unit_str = parse_env_sensor(sensor_id, val1, val2)

            emqx_payload = {
                "device_name": dev_name,
                "packet_id": f_cnt,
                "sensor_type": ENV_SENSOR_MAP.get(sensor_id, "UNKNOWN"),
                "val": val_data,
                "unit": unit_str,
                "power": {
                    "voltage_v": voltage_v,
                    "current_ma": current_ma,
                },
                "fPort": f_port,
            }

        json_output = json.dumps(emqx_payload, indent=2)

        res = pub_client.publish(target_topic, json_output, qos=1)
        if res.rc == mqtt.MQTT_ERR_SUCCESS:
            print(f"\n[Forward -> EMQX] DevEUI: {dev_eui} | Topik: {target_topic}")
            print(json_output)
            print("-" * 55)
        else:
            print(f"[Error EMQX] Gagal publish untuk {dev_eui}, status code: {res.rc}")

    except json.JSONDecodeError:
        print("[Error] Pesan dari ChirpStack bukan JSON valid.")
    except Exception as e:
        print(f"[Error] Kesalahan saat memproses pesan: {e}")


# ==============================================================================
# MAIN EXECUTION
# ==============================================================================
if __name__ == "__main__":
    pub_client.on_connect = on_pub_connect
    if EMQX_USER and EMQX_PASS:
        pub_client.username_pw_set(EMQX_USER, EMQX_PASS)

    print("[System] Menghubungkan ke EMQX Broker...")
    try:
        pub_client.connect(EMQX_BROKER, EMQX_PORT, 60)
        pub_client.loop_start()
    except Exception as e:
        print(f"[Fatal] Gagal koneksi ke EMQX: {e}")
        sys.exit(1)

    sub_client.on_connect = on_sub_connect
    sub_client.on_message = on_chirpstack_message

    print("[System] Menghubungkan ke ChirpStack Broker...")
    try:
        sub_client.connect(CHIRPSTACK_BROKER, CHIRPSTACK_PORT, 60)
        sub_client.loop_forever()
    except KeyboardInterrupt:
        print("\n[System] Forwarder dihentikan oleh pengguna.")
        pub_client.loop_stop()
        pub_client.disconnect()
        sub_client.disconnect()
    except Exception as e:
        print(f"[Fatal] Gagal koneksi ke ChirpStack: {e}")