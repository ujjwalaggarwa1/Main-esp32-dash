# Master Helper (ESP32-C3 ESP-IDF Core)

A foundation firmware for the **ESP32-C3** (`esp32-c3-devkitm-1`) built on the **ESP-IDF** framework. It provides persistent NVS configuration management, Wi-Fi with automatic SoftAP fallback, a modern responsive Web Control Center, and an automated GitHub-based OTA updater.

---

## 🚀 Key Features

- **Dual OTA Partitioning**: Safe A/B rollback and partition swapping (`ota_0` & `ota_1`, 1.5MB each).
- **Persistent NVS Configuration**:
  - Wi-Fi SSID & Password
  - Device Name
  - GitHub Update Manifest URL (`version.json`)
  - Running Firmware Version & Git Commit Hash
- **Smart Wi-Fi Manager**:
  - Attempts Station (STA) connection to saved Wi-Fi.
  - Automatically falls back to **SoftAP Mode** (`MasterHelper-AP`, pass: `12345678`) on `http://192.168.4.1` if Wi-Fi credentials are unset or connection fails.
- **Embedded Web Control Center**:
  - Real-time status monitoring (IP, RSSI, Free Heap, Uptime, Active Partition).
  - Wi-Fi & Configuration settings form (update Wi-Fi without USB cables).
  - GitHub OTA Updater tab with version comparison and live flashing progress bar.
  - Direct `.bin` manual upload tab for offline flashing.
  - One-click device restart button.
- **GitHub OTA Updater Engine**:
  - Fetches and parses `version.json` over HTTPS (using ESP-IDF certificate bundle).
  - **Version Update Rule**: Checks remote version and commit hash; **only triggers an update if the remote version number has changed AND the commit hash does not match**.
  - Automatically swaps the boot partition and reboots upon successful flashing.

---

## 📁 Project Architecture

```
Main/
├── partitions.csv             # 4MB Dual-OTA partition table (nvs, otadata, phy, ota_0, ota_1)
├── platformio.ini             # PlatformIO config for esp32-c3-devkitm-1 (ESP-IDF framework)
├── sdkconfig.esp32-c3-devkitm-1 # ESP-IDF configuration (mbedTLS cert bundle, custom partitions)
├── version.json.template      # Example GitHub manifest file
└── src/
    ├── app_config.h           # System constants, defaults, and data models
    ├── config_manager.h/.c    # NVS read/write manager for credentials & build info
    ├── wifi_manager.h/.c      # Wi-Fi STA connection handler + SoftAP fallback
    ├── ota_engine.h/.c        # HTTPS client, GitHub version checker & OTA flasher
    ├── web_ui.h               # Responsive Single Page Application (HTML/CSS/JS)
    ├── web_server.h/.c        # ESP-IDF HTTP server & REST API endpoints
    └── main.c                 # Application entry point & supervisory loop
```

---

## 🌐 Web Control Center & REST API

| Endpoint | Method | Description |
| :--- | :--- | :--- |
| `/` | `GET` | Serves the responsive Web UI |
| `/api/status` | `GET` | Returns system status (version, hash, partition, IP, RSSI, heap, uptime) |
| `/api/config` | `GET` | Returns current device configuration |
| `/api/config` | `POST` | Saves new Wi-Fi credentials, device name, and GitHub manifest URL |
| `/api/ota/check` | `POST` | Fetches GitHub `version.json` and checks if an update is available |
| `/api/ota/update` | `POST` | Initiates background OTA download and flashing from GitHub |
| `/api/ota/status` | `GET` | Polls current OTA progress percentage and status message |
| `/api/ota/upload` | `POST` | Streams a local `.bin` file directly into flash |
| `/api/restart` | `POST` | Schedules a graceful device reboot |

---

## 🔄 GitHub OTA Manifest (`version.json`)

To enable remote updates, commit a `version.json` file to your GitHub repository (e.g. in your `main` branch or Releases):

```json
{
  "version": "1.0.1",
  "hash": "a1b2c3d4e5f67890",
  "bin_url": "https://raw.githubusercontent.com/<user>/<repo>/main/build/firmware.bin",
  "notes": "Added life assistant endpoints and performance improvements"
}
```

### Update Logic
1. The ESP32 compares `version` against its currently running version.
2. The ESP32 compares `hash` against its currently running build hash.
3. If `version != current_version` **AND** `hash != current_hash`, the update is flagged as available and can be flashed with one click.
4. If the version is unchanged, it reports `Firmware is up to date`.

---

## 🛠️ Getting Started

### 1. Build and Flash
Run the following from your terminal:

```bash
# Build firmware
pio run

# Flash to board and monitor output
pio run -t upload -t monitor
```

### 2. Initial Setup
1. If no Wi-Fi credentials are saved, connect to the access point:
   - **SSID**: `MasterHelper-AP`
   - **Password**: `12345678`
2. Open your browser and navigate to:
   - `http://192.168.4.1`
3. Enter your home/office Wi-Fi details and click **Save & Reconnect**.
4. Once connected, access the Web UI from your local network IP printed in the serial monitor or shown on the dashboard.
