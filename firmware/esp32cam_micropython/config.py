"""
Configuration values for the ESP32-CAM MicroPython firmware.

Copy this file to `config_local.py` before flashing and edit the fields to match
your Wi-Fi network and the HTTP endpoint that will process captured images.
"""

WIFI_SSID = "REPLACE_WITH_SSID"
WIFI_PASSWORD = "REPLACE_WITH_PASSWORD"

# URL of the HTTP endpoint that accepts JPEG images and responds with a JSON
# payload containing a `description` field. Typically this is hosted on a local
# machine or cloud server that has access to a vision model.
VISION_ENDPOINT = "http://192.168.0.10:8000/analyze"

# Optional API key that will be sent in the `X-Api-Key` header when talking to
# the vision endpoint. Leave empty if your service does not require it.
VISION_API_KEY = ""
