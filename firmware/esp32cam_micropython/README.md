## ESP32-CAM MicroPython Vision Firmware

This folder contains a complete MicroPython firmware bundle for the `ESP32-CAM-MB`
that captures still images, forwards them to a vision model and prints natural
language descriptions over the serial console.

The solution is split in two parts:
- `boot.py`, `main.py` and the config file you flash to the ESP32-CAM.
- A small FastAPI relay (`vision_service/`) that runs on a computer or cloud
  instance and calls OpenAI's multimodal models to generate the description.

### 1. Prerequisites

- MicroPython firmware build for ESP32-CAM that includes the `camera` module (e.g.
  the official nightly with camera support).
- `esptool.py` to flash MicroPython.
- Python 3.9+ on your workstation to run the vision relay.
- An OpenAI API key stored in `OPENAI_API_KEY`.

### 2. Configure the ESP32-CAM firmware

1. Copy `config.py` to `config_local.py` and edit:
   - `WIFI_SSID` / `WIFI_PASSWORD`
   - `VISION_ENDPOINT` pointing to the URL where the relay will run.
   - `VISION_API_KEY` if you want an additional shared secret check.
2. Flash MicroPython to the board (skip if already installed):
   ```bash
   esptool.py --chip esp32 --port /dev/ttyUSB0 erase_flash
   esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 460800 write_flash -z 0x1000 esp32cam-20240430-v1.22.1.bin
   ```
3. Use `mpremote` or `ampy` to copy `boot.py`, `main.py`, `config.py` (or your
   `config_local.py`) to the device:
   ```bash
   mpremote connect /dev/ttyUSB0 fs cp boot.py :
   mpremote connect /dev/ttyUSB0 fs cp main.py :
   mpremote connect /dev/ttyUSB0 fs cp config_local.py :config.py
   ```

### 3. Run the vision relay

1. Install dependencies and start the server:
   ```bash
   cd firmware/esp32cam_micropython/vision_service
   python -m venv .venv
   source .venv/bin/activate
   pip install -r requirements.txt
   export OPENAI_API_KEY=sk-...
   # Optional shared secret (must match VISION_API_KEY on the device)
   export VISION_SHARED_SECRET=top-secret
   uvicorn server:app --host 0.0.0.0 --port 8000
   ```
2. The firmware expects the `/analyze` endpoint to return:
   ```json
   {"description": "texto com a descrição da imagem"}
   ```

### 4. Operation

Once the ESP32-CAM boots it will:
- Connect to the configured Wi-Fi network (handled by `boot.py`).
- Capture a frame every 12 seconds.
- POST the JPEG data to the relay service.
- Print the Portuguese description over the serial REPL.

Adjust the capture interval or camera parameters inside `main.py` to match your
latency and quality requirements. The default `FRAME_QVGA` (320x240) keeps the
payload small enough for reliable transmission on MicroPython builds.

### 5. Troubleshooting

- If you see `Failed to capture image from camera`, reboot the board and ensure
  your MicroPython build includes camera support.
- `Vision request failed` indicates the relay is unreachable, the shared secret
  mismatched, or your OpenAI key is invalid. The relay logs will include details.
- To reduce bandwidth usage, increase `CAPTURE_INTERVAL_S` or raise
  `camera.quality()` (higher value = more compression).
