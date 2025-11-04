"""
Wi-Fi bootstrap for the ESP32-CAM running MicroPython.

The script attempts to connect to the configured Wi-Fi network before the main
application is started. Connection is retried until success to ensure the
camera firmware is online before capturing images.
"""

import network
import time

try:
    from config_local import WIFI_SSID, WIFI_PASSWORD  # type: ignore
except ImportError:
    from config import WIFI_SSID, WIFI_PASSWORD


def connect_wifi() -> None:
    sta = network.WLAN(network.STA_IF)
    if not sta.active():
        sta.active(True)

    if sta.isconnected():
        return

    sta.connect(WIFI_SSID, WIFI_PASSWORD)

    # Retry connection attempts with exponential backoff.
    delay_s = 0.5
    max_delay_s = 8
    while not sta.isconnected():
        time.sleep(delay_s)
        delay_s = min(max_delay_s, delay_s * 1.5)


connect_wifi()
