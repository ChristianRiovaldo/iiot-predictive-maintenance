# IIoT Multi-Node LoRaWAN Gateway (Raspberry Pi 5 + WM1302 + ChirpStack v4 + EMQX)

Panduan lengkap instalasi, konfigurasi, dan pengoperasian sistem *Industrial IoT (IIoT)* berbasis **LoRaWAN (AS923)** menggunakan **Raspberry Pi 5**, modul concentrator **WM1302 (SPI)**, **ChirpStack v4**, dan **EMQX Broker**.

## Arsitektur Sistem

```
[LilyGO T3 Multi-Sensor Nodes] 
       │  (9-Byte Binary Payload via LoRa AS923 - Unconfirmed Uplink)
       ▼
[WM1302 SPI Concentrator + Raspberry Pi 5]
       │  (Semtech Packet Forwarder -> UDP 1700)
       ▼
[ChirpStack Gateway Bridge -> Local Mosquitto MQTT]
       │
       ▼
[ChirpStack v4 Network/App Server + JS Payload Codec]
       │  (Topic: application/+/device/+/event/up)
       ▼
[Python Multi-Node Forwarder (gateway_forwarder.py)]
       │  (JSON Standardized Payload via Tailscale)
       ▼
[EMQX Broker (100.95.51.126)] -> Topics: /iiot/node1, /iiot/node2, /iiot/node3

```

## 1. Persiapan Sistem & Aktivasi SPI (Raspberry Pi 5)

Raspberry Pi 5 menggunakan chip I/O **RP1** yang memetakan jalur GPIO ke `gpiochip4`.

```
# 1. Update sistem & instal dependensi build, GPIO, MQTT, dan Python
sudo apt update && sudo apt upgrade -y
sudo apt install -y git build-essential libgpiod-dev gpiod mosquitto mosquitto-clients python3-paho-mqtt curl wget

# 2. Aktifkan SPI dan I2C melalui raspi-config (non-interaktif)
sudo raspi-config nonint do_spi 0
sudo raspi-config nonint do_i2c 0

# 3. Buat direktori kerja project
mkdir -p /home/iiotgw/iiot_gateway

# 4. Reboot untuk menerapkan antarmuka SPI
sudo reboot

```

Setelah Raspberry Pi menyala kembali, pastikan perangkat SPI aktif:

```
ls -l /dev/spidev0.0

```

## 2. Clone & Build `sx1302_hal` (Semtech Packet Forwarder)

```
cd /home/iiotgw
git clone https://github.com/Lora-net/sx1302_hal.git
cd /home/iiotgw/sx1302_hal
make clean && make all

```

### Konfigurasi Pin Reset WM1302 untuk Raspberry Pi 5 (`reset_lgw.sh`)

Raspberry Pi 5 menggunakan `gpioset` pada `gpiochip4` (pengganti *sysfs* `/sys/class/gpio`). Timpa file `reset_lgw.sh` dengan perintah berikut:

```
cat << 'EOF' > /home/iiotgw/sx1302_hal/packet_forwarder/reset_lgw.sh
#!/bin/sh
# Pin Reset WM1302 Pi HAT (umumnya GPIO 17 atau GPIO 25)
SX1302_RESET_PIN=17
GPIO_CHIP="gpiochip4"

case "$1" in
    start)
        gpioset $GPIO_CHIP $SX1302_RESET_PIN=1
        sleep 0.1
        gpioset $GPIO_CHIP $SX1302_RESET_PIN=0
        sleep 0.1
        ;;
    stop)
        gpioset $GPIO_CHIP $SX1302_RESET_PIN=0
        ;;
    *)
        echo "Usage: $0 {start|stop}"
        exit 1
        ;;
esac
exit 0
EOF

chmod +x /home/iiotgw/sx1302_hal/packet_forwarder/reset_lgw.sh

```

### Setup Konfigurasi Frekuensi AS923 (`global_conf.json`)

```
cd /home/iiotgw/sx1302_hal/packet_forwarder
cp global_conf.json.sx1250.AS923.SPI global_conf.json

# Generate Gateway EUI otomatis dari MAC address eth0 (sisipan FFFE)
MAC_ADDR=$(cat /sys/class/net/eth0/address | tr -d ':')
GW_EUI="${MAC_ADDR:0:6}fffe${MAC_ADDR:6:6}"
echo "Gateway EUI Anda: $GW_EUI"

# Sesuaikan port UDP lokal & jalur SPI di global_conf.json
sed -i 's/"com_path": ".*"/"com_path": "\/dev\/spidev0.0"/' global_conf.json
sed -i 's/"server_address": ".*"/"server_address": "127.0.0.1"/' global_conf.json
sed -i 's/"serv_port_up": [0-9]*/"serv_port_up": 1700/' global_conf.json
sed -i 's/"serv_port_down": [0-9]*/"serv_port_down": 1700/' global_conf.json
sed -i "s/\"gateway_ID\": \".*\"/\"gateway_ID\": \"$GW_EUI\"/" global_conf.json

```

*(Catat output `Gateway EUI Anda` di atas untuk didaftarkan ke Web UI ChirpStack).*

## 3. Instalasi ChirpStack v4 & Gateway Bridge (Debian ARM64)

```
# 1. Instal dependensi database & Redis
sudo apt install -y postgresql redis-server

# 2. Buat database dan user PostgreSQL untuk ChirpStack
sudo -u postgres psql -c "create role chirpstack with login password 'chirpstack';"
sudo -u postgres psql -c "create database chirpstack with owner chirpstack;"
sudo -u postgres psql -d chirpstack -c "create extension pg_trgm;"

# 3. Tambahkan repositori resmi ChirpStack v4
sudo mkdir -p /etc/apt/keyrings
sudo curl -fsSL https://artifacts.chirpstack.io/packages/chirpstack.key -o /etc/apt/keyrings/chirpstack.asc
echo "deb [signed-by=/etc/apt/keyrings/chirpstack.asc] https://artifacts.chirpstack.io/packages/4.x/deb stable main" | sudo tee /etc/apt/sources.list.d/chirpstack.list

# 4. Instal ChirpStack & ChirpStack Gateway Bridge
sudo apt update
sudo apt install -y chirpstack chirpstack-gateway-bridge

```

### Sinkronisasi Konfigurasi Regional AS923

Pastikan topik regional pada `chirpstack-gateway-bridge` dan `chirpstack` menggunakan prefix **`as923`**:

```
# 1. Konfigurasi topik AS923 di ChirpStack Gateway Bridge
sudo sed -i 's/event_topic_template=".*"/event_topic_template="as923\/gateway\/{{ .GatewayID }}\/event\/{{ .EventType }}"/' /etc/chirpstack-gateway-bridge/chirpstack-gateway-bridge.toml
sudo sed -i 's/state_topic_template=".*"/state_topic_template="as923\/gateway\/{{ .GatewayID }}\/state\/{{ .StateType }}"/' /etc/chirpstack-gateway-bridge/chirpstack-gateway-bridge.toml
sudo sed -i 's/command_topic_template=".*"/command_topic_template="as923\/gateway\/{{ .GatewayID }}\/command\/{{ .CommandType }}"/' /etc/chirpstack-gateway-bridge/chirpstack-gateway-bridge.toml

# 2. Aktifkan region as923 di ChirpStack Server
sudo sed -i 's/enabled_regions=\[.*\]/enabled_regions=["as923"]/' /etc/chirpstack/chirpstack.toml

# 3. Jalankan dan aktifkan semua layanan dasar
sudo systemctl enable --now mosquitto postgresql redis-server chirpstack-gateway-bridge chirpstack

```

## 4. Konfigurasi Web UI ChirpStack & Payload Codec

Buka browser dan akses **`http://<IP_RASPI>:8080`** (login default: `admin` / `admin`):

1. **Gateways** $\rightarrow$ **Add Gateway:** Masukkan `Gateway ID` (16 karakter hex dari langkah 2). Status akan berubah menjadi **Online (hijau)** setelah service Packet Forwarder dijalankan.

2. **Device Profiles** $\rightarrow$ **Add Device Profile:**

   * **General:** Region = `AS923`, MAC version = `LoRaWAN 1.0.3` / `1.0.4`, Regional parameters = `RP002-1.0.3`.

   * **Join (OTAA/ABP):** Aktifkan **Device supports OTAA**.

   * **Codec:** Pilih **JavaScript functions** dan masukkan kode decoder 9-byte berikut:

### Struktur 9-Byte Binary Payload

| Byte | Variabel | Tipe | Keterangan | 
| ----- | ----- | ----- | ----- | 
| `0` | `sensor_id` | `uint8` | `0`=None, `1`=BME688, `2`=CO, `3`=PMS7003T, `4`=CO2, `5`=H2S | 
| `1–2` | `val1` | `int16` | Nilai utama sensor (Suhu $\times 10$, PM2.5, CO2 PPM, Raw ADC) | 
| `3–4` | `val2` | `uint16` | Nilai kedua sensor (Kelembapan $\times 10$, PM10, Tegangan H2S $\times 100$) | 
| `5–6` | `v_bus` | `uint16` | INA226 Bus Voltage (Volt $\times 100$) | 
| `7–8` | `i_bus` | `int16` | INA226 Current (mA $\times 10$) | 

### JavaScript Payload Codec (ChirpStack v4)

```
function decodeUplink(input) {
  var bytes = input.bytes;

  if (bytes.length < 9) {
    return { data: { error: "Payload kurang dari 9 byte" } };
  }

  var sensor_id = bytes[0];

  var val1 = (bytes[1] << 8) | bytes[2];
  if (val1 & 0x8000) val1 |= 0xFFFF0000;

  var val2 = (bytes[3] << 8) | bytes[4];

  var v_bus_raw = (bytes[5] << 8) | bytes[6];
  var voltage_v = v_bus_raw / 100.0;

  var i_bus_raw = (bytes[7] << 8) | bytes[8];
  if (i_bus_raw & 0x8000) i_bus_raw |= 0xFFFF0000;
  var current_ma = i_bus_raw / 10.0;

  return {
    data: {
      sensor_id: sensor_id,
      val1: val1,
      val2: val2,
      voltage_v: voltage_v,
      current_ma: current_ma
    }
  };
}

```

3. **Applications** $\rightarrow$ **Add Device:** Daftarkan Node 1, Node 2, dan Node 3 dengan `DevEUI`, `JoinEUI`, dan `AppKey` masing-masing (pastikan `Uplink` dan `Downlink Frame Counter` dimulai dari **`0`** saat sesi baru).

4. **Catatan Firmware Node:** Pastikan pengiriman di node menggunakan *Unconfirmed Uplink* (`node.send(payload, sizeof(payload), 1)`) agar tidak memicu error `DOWNLINK_GATEWAY`.

## 5. Instalasi Skrip Python Multi-Node Forwarder

Buat skrip `/home/iiotgw/iiot_gateway/gateway_forwarder.py`:

```
cat << 'EOF' > /home/iiotgw/iiot_gateway/gateway_forwarder.py
#!/usr/bin/env python3
import json
import os
import sys
import paho.mqtt.client as mqtt

# Tambahkan DevEUI node baru (huruf kecil) di dictionary ini
NODE_TOPIC_MAP = {
    "737fa3925c5c2bba": "/iiot/node1",
    "e02d6ee6c5f39cfd": "/iiot/node2",
    "b5770a367b5cba7d": "/iiot/node3"
}

CHIRPSTACK_BROKER = os.getenv("CHIRPSTACK_BROKER", "localhost")
CHIRPSTACK_PORT = int(os.getenv("CHIRPSTACK_PORT", "1883"))
CHIRPSTACK_TOPIC = "application/+/device/+/event/up"

EMQX_BROKER = os.getenv("EMQX_BROKER", "100.95.51.126")
EMQX_PORT = int(os.getenv("EMQX_PORT", "1883"))
EMQX_USER = os.getenv("EMQX_USER", "")
EMQX_PASS = os.getenv("EMQX_PASS", "")

sub_client = mqtt.Client(client_id="chirpstack_all_nodes_sub")
pub_client = mqtt.Client(client_id="emqx_all_nodes_pub")

ENV_SENSOR_MAP = {
    0: "ID_NONE",
    1: "ID_BME688",
    2: "ID_CO_I2C",
    3: "ID_PMS7003T",
    4: "ID_CO2_UART",
    5: "ID_H2S_SEN0568"
}

def parse_env_sensor(sensor_id, val1, val2):
    if sensor_id == 1:
        return {"temperature": round(val1 / 10.0, 2), "humidity": round(val2 / 10.0, 2)}, "°C, %"
    elif sensor_id == 2:
        return {"co_ppm": round(val1 / 10.0, 2)}, "PPM"
    elif sensor_id == 3:
        return {"pm2_5": val1, "pm10": val2}, "ug/m3"
    elif sensor_id == 4:
        return {"co2": val1}, "PPM"
    elif sensor_id == 5:
        return {"adc_raw": val1, "voltage": round(val2 / 100.0, 2)}, "V"
    else:
        return {"status": "no_env_sensor"}, "-"

def on_sub_connect(client, userdata, flags, rc):
    if rc == 0:
        print(f"[ChirpStack] Terhubung ke {CHIRPSTACK_BROKER}:{CHIRPSTACK_PORT}")
        client.subscribe(CHIRPSTACK_TOPIC, qos=1)
    else:
        print(f"[ChirpStack] Gagal terhubung, rc: {rc}")

def on_pub_connect(client, userdata, flags, rc):
    if rc == 0:
        print(f"[EMQX] Terhubung ke {EMQX_BROKER}:{EMQX_PORT}")
    else:
        print(f"[EMQX] Gagal terhubung, rc: {rc}")

def on_chirpstack_message(client, userdata, msg):
    try:
        data_json = json.loads(msg.payload.decode("utf-8"))
        dev_info = data_json.get("deviceInfo", {})
        dev_eui = dev_info.get("devEui", "").lower()
        dev_name = dev_info.get("deviceName", "unknown-node")
        f_cnt = data_json.get("fCnt", 0)

        if dev_eui not in NODE_TOPIC_MAP:
            return

        target_topic = NODE_TOPIC_MAP[dev_eui]
        decoded_object = data_json.get("object", {})
        if not decoded_object:
            return

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
                "current_ma": current_ma
            }
        }

        json_output = json.dumps(emqx_payload, indent=2)
        res = pub_client.publish(target_topic, json_output, qos=1)
        if res.rc == mqtt.MQTT_ERR_SUCCESS:
            print(f"\n[Forward -> EMQX] DevEUI: {dev_eui} | Topik: {target_topic}")
            print(json_output)
            print("-" * 55)
    except Exception as e:
        print(f"[Error] {e}")

if __name__ == "__main__":
    pub_client.on_connect = on_pub_connect
    if EMQX_USER and EMQX_PASS:
        pub_client.username_pw_set(EMQX_USER, EMQX_PASS)

    pub_client.connect(EMQX_BROKER, EMQX_PORT, 60)
    pub_client.loop_start()

    sub_client.on_connect = on_sub_connect
    sub_client.on_message = on_chirpstack_message
    sub_client.connect(CHIRPSTACK_BROKER, CHIRPSTACK_PORT, 60)
    try:
        sub_client.loop_forever()
    except KeyboardInterrupt:
        pub_client.loop_stop()
        pub_client.disconnect()
        sub_client.disconnect()
EOF

chmod +x /home/iiotgw/iiot_gateway/gateway_forwarder.py

```

## 6. Otomatisasi Background Service (`systemd`)

Buat dua unit *service* agar **Packet Forwarder (WM1302)** dan **Python EMQX Forwarder** otomatis berjalan saat Raspberry Pi 5 menyala:

```
# 1. Service Packet Forwarder WM1302
sudo tee /etc/systemd/system/lora-pkt-fwd.service > /dev/null << 'EOF'
[Unit]
Description=Semtech SX1302 LoRa Packet Forwarder
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=root
WorkingDirectory=/home/iiotgw/sx1302_hal/packet_forwarder
ExecStartPre=/home/iiotgw/sx1302_hal/packet_forwarder/reset_lgw.sh start
ExecStart=/home/iiotgw/sx1302_hal/packet_forwarder/lora_pkt_fwd
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF

# 2. Service Python Forwarder ke EMQX
sudo tee /etc/systemd/system/lorawan-forwarder.service > /dev/null << 'EOF'
[Unit]
Description=ChirpStack to EMQX Multi-Node MQTT Forwarder
After=network-online.target mosquitto.service chirpstack.service
Wants=network-online.target

[Service]
Type=simple
User=iiotgw
WorkingDirectory=/home/iiotgw/iiot_gateway
ExecStart=/usr/bin/python3 -u /home/iiotgw/iiot_gateway/gateway_forwarder.py
Restart=always
RestartSec=5

Environment="CHIRPSTACK_BROKER=localhost"
Environment="EMQX_BROKER=100.95.51.126"

[Install]
WantedBy=multi-user.target
EOF

# 3. Reload daemon & jalankan kedua service
sudo systemctl daemon-reload
sudo systemctl enable --now lora-pkt-fwd.service
sudo systemctl enable --now lorawan-forwarder.service

```

## 7. Referensi Perintah Pemantauan & Pemeliharaan

```
# Cek status seluruh layanan gateway
sudo systemctl status lora-pkt-fwd.service lorawan-forwarder.service chirpstack.service

# Pantau log penerusan data JSON ke EMQX secara real-time
sudo journalctl -u lorawan-forwarder.service -f

# Pantau log radio penerimaan paket WM1302 secara real-time
sudo journalctl -u lora-pkt-fwd.service -f

# Restart forwarder setelah menambah DevEUI node baru di NODE_TOPIC_MAP
sudo systemctl restart lorawan-forwarder.service

# Matikan sementara forwarder
sudo systemctl stop lorawan-forwarder.service

# Reset hardware WM1302 secara manual
sudo /home/iiotgw/sx1302_hal/packet_forwarder/reset_lgw.sh start

```