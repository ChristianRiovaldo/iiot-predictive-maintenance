# Setup LoRaWAN Gateway (Raspberry Pi 5 + WM1302 + ChirpStack v4 Docker)

Panduan ringkas instalasi dan konfigurasi **LoRaWAN Gateway (AS923)** pada **Raspberry Pi 5** menggunakan modul concentrator **WM1302 (SPI)** dan **ChirpStack v4 (Docker)**.

## 1. Mengatur SPI pada Raspberry Pi 5

Raspberry Pi 5 menggunakan chip I/O **RP1** yang memetakan jalur GPIO ke `gpiochip4`.

```
# 1. Update sistem & instal dependensi build serta GPIO
sudo apt update && sudo apt upgrade -y
sudo apt install -y git build-essential libgpiod-dev gpiod curl

# 2. Aktifkan SPI dan I2C secara non-interaktif
sudo raspi-config nonint do_spi 0
sudo raspi-config nonint do_i2c 0

# 3. Reboot untuk menerapkan konfigurasi antarmuka
sudo reboot

```

Setelah menyala kembali, pastikan device SPI sudah muncul:

```
ls -l /dev/spidev0.0

```

## 2. Clone & Build `sx1302_hal` (Packet 9)

```
# 1. Clone repositori resmi Semtech sx1302_hal
cd ~
git clone https://github.com/Lora-net/sx1302_hal.git
cd ~/sx1302_hal

# 2. Kompilasi driver HAL dan packet forwarder
make clean && make all

```

### Sesuaikan Skrip Reset WM1302 (`reset_lgw.sh`) untuk Raspberry Pi 5

Karena Raspberry Pi 5 tidak mendukung *sysfs* `/sys/class/gpio`, gunakan `gpioset` pada `gpiochip4`:

```
cat << 'EOF' > ~/sx1302_hal/packet_forwarder/reset_lgw.sh
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

chmod +x ~/sx1302_hal/packet_forwarder/reset_lgw.sh

```

## 3. Setup ChirpStack v4 via Docker

```
# 1. Instal Docker & Docker Compose
curl -fsSL https://get.docker.com | sh
sudo usermod -aG docker $USER
newgrp docker

# 2. Clone repositori resmi ChirpStack Docker
cd ~
git clone https://github.com/chirpstack/chirpstack-docker.git
cd ~/chirpstack-docker

```

## 4. Setup Frekuensi AS923 pada File Konfigurasi

### A. Konfigurasi Packet Forwarder (`global_conf.json`)

Salin template konfigurasi SPI untuk regional AS923 dan atur `gateway_ID`:

```
cd ~/sx1302_hal/packet_forwarder
cp global_conf.json.sx1250.AS923.SPI global_conf.json

# Generate Gateway EUI (16 hex) dari MAC address eth0
MAC_ADDR=$(cat /sys/class/net/eth0/address | tr -d ':')
GW_EUI="${MAC_ADDR:0:6}fffe${MAC_ADDR:6:6}"
echo "Gateway EUI Anda: $GW_EUI"

# Sesuaikan parameter SPI, UDP port 1700, dan Gateway ID
sed -i 's/"com_path": ".*"/"com_path": "\/dev\/spidev0.0"/' global_conf.json
sed -i 's/"server_address": ".*"/"server_address": "127.0.0.1"/' global_conf.json
sed -i 's/"serv_port_up": [0-9]*/"serv_port_up": 1700/' global_conf.json
sed -i 's/"serv_port_down": [0-9]*/"serv_port_down": 1700/' global_conf.json
sed -i "s/\"gateway_ID\": \".*\"/\"gateway_ID\": \"$GW_EUI\"/" global_conf.json

```

*(Simpan nilai `Gateway EUI Anda` untuk didaftarkan di Web UI ChirpStack).*

### B. Konfigurasi ChirpStack Docker (`AS923`)

Sesuaikan file konfigurasi di dalam folder `~/chirpstack-docker/configuration/`:

1. **Atur Topic Prefix di `chirpstack-gateway-bridge/chirpstack-gateway-bridge.toml`:**

   ```
   sed -i 's/event_topic_template=".*"/event_topic_template="as923\/gateway\/{{ .GatewayID }}\/event\/{{ .EventType }}"/' ~/chirpstack-docker/configuration/chirpstack-gateway-bridge/chirpstack-gateway-bridge.toml
   sed -i 's/state_topic_template=".*"/state_topic_template="as923\/gateway\/{{ .GatewayID }}\/state\/{{ .StateType }}"/' ~/chirpstack-docker/configuration/chirpstack-gateway-bridge/chirpstack-gateway-bridge.toml
   sed -i 's/command_topic_template=".*"/command_topic_template="as923\/gateway\/{{ .GatewayID }}\/command\/{{ .CommandType }}"/' ~/chirpstack-docker/configuration/chirpstack-gateway-bridge/chirpstack-gateway-bridge.toml
   
   ```

2. **Aktifkan Region AS923 di `chirpstack/chirpstack.toml`:**
   Pastikan baris `enabled_regions` mencakup `"as923"`:

   ```
   enabled_regions=[
     "as923",
   ]
   
   ```

3. **Jalankan Container Docker & Packet Forwarder:**

   ```
   # Jalankan seluruh container ChirpStack
   cd ~/chirpstack-docker
   docker compose up -d
   
   # Jalankan Packet Forwarder WM1302
   cd ~/sx1302_hal/packet_forwarder
   sudo ./reset_lgw.sh start
   sudo ./lora_pkt_fwd
   
   ```

## 5. Mendaftarkan Gateway dan Node di Web UI ChirpStack

Buka browser dan akses **`http://<IP_RASPBERRY_PI>:8080`** (login default: **`admin`** / **`admin`**).

### A. Mendaftarkan Gateway

1. Pilih menu **Gateways** di bilah kiri $\rightarrow$ klik **Add gateway**.

2. Isi **Name** (misal: `raspi5-wm1302`).

3. Isi **Gateway ID** dengan **16 digit hex `Gateway EUI`** yang didapat pada Langkah 4A.

4. Klik **Submit**. Tunggu beberapa detik hingga indikator **Last seen** berubah menjadi **Online (hijau)**.

### B. Membuat Device Profile (AS923)

1. Pilih menu **Device profiles** $\rightarrow$ klik **Add device profile**.

2. Pada tab **General**:

   * **Name:** `AS923-OTAA-Profile`

   * **Region:** `AS923`

   * **MAC version:** `LoRaWAN 1.0.3` (atau `1.0.4` sesuai library node)

   * **Regional parameters revision:** `RP002-1.0.3` (atau `A`/`B`)

3. Pada tab **Join (OTAA/ABP)**:

   * Pastikan opsi **Device supports OTAA** dalam keadaan aktif (*checked*).

4. *(Opsional)* Pada tab **Codec**, pilih **JavaScript functions** jika ingin membedah binary payload langsung di ChirpStack, lalu klik **Submit**.

### C. Mendaftarkan Node (End-Device)

1. Pilih menu **Applications** $\rightarrow$ klik **Add application** (buat nama aplikasi baru, lalu buka aplikasi tersebut).

2. Di dalam aplikasi, klik **Add device**:

   * **Name:** Nama node (misal: `node-1`).

   * **Device EUI (DevEUI):** Masukkan 16 digit hex `devEUI` sesuai yang tertulis di firmware node.

   * **Join EUI (AppEUI):** Masukkan 16 digit hex `joinEUI` sesuai firmware node.

   * **Device profile:** Pilih `AS923-OTAA-Profile` yang baru saja dibuat.

   * Klik **Submit**.

3. Anda akan diarahkan ke tab **Keys (OTAA)**:

   * Masukkan 32 digit hex **Application key (`AppKey`)** sesuai firmware node, lalu klik **Submit**.

4. Nyalakan/reset node Anda. Pantau tab **Events** atau **LoRaWAN frames** untuk memastikan event **`join`** dan **`up`** masuk.