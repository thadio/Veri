"""
ESP32-CAM MicroPython firmware that captures images and forwards them to a
vision service capable of returning natural-language descriptions.

Usage flow
----------
1. Define Wi-Fi credentials and the vision endpoint URL in `config.py` (or
   `config_local.py`).
2. Flash this project to a MicroPython-enabled ESP32-CAM build that ships with
   camera support.
3. Power the device; it will connect to Wi-Fi, capture frames at a fixed
   interval, and print textual descriptions over the serial console.
"""

import gc
import time

import camera

try:
    import urequests as requests
except ImportError:  # pragma: no cover - fallback for CPython tests
    import requests  # type: ignore

try:
    from config_local import VISION_ENDPOINT, VISION_API_KEY  # type: ignore
except ImportError:
    from config import VISION_ENDPOINT, VISION_API_KEY


# Interval between captures in seconds.
CAPTURE_INTERVAL_S = 12


def init_camera() -> None:
    """Initialise the camera with conservative defaults for stability."""
    # Reset any previous state before initialising.
    try:
        camera.deinit()
    except OSError:
        pass

    sensor_id = 0  # 0 selects the default pin mapping for ESP32-CAM-MB boards.
    camera.init(sensor_id, format=camera.JPEG)

    # Tune image settings for balanced quality vs. size. Adjust if required.
    camera.framesize(camera.FRAME_QVGA)
    camera.speffect(camera.EFFECT_NONE)
    camera.whitebalance(camera.WB_AUTO)
    camera.saturation(0)
    camera.brightness(0)
    camera.contrast(0)
    camera.quality(10)  # Lower is higher quality; keep within 10-15 for balance.


def capture_frame() -> bytes:
    """Capture a JPEG frame from the camera, retrying to avoid transient errors."""
    attempts = 3
    for attempt in range(attempts):
        buf = camera.capture()
        if buf:
            return buf
        time.sleep(0.2 * (attempt + 1))
    raise RuntimeError("Failed to capture image from camera.")


def describe_frame(image_bytes: bytes) -> str:
    """Send the JPEG image to the vision endpoint and return the description."""
    headers = {
        "Content-Type": "application/octet-stream",
        "Accept": "application/json",
    }
    if VISION_API_KEY:
        headers["X-Api-Key"] = VISION_API_KEY

    try:
        response = requests.post(VISION_ENDPOINT, data=image_bytes, headers=headers)
    except Exception as exc:
        raise RuntimeError("Failed to reach vision service: {}".format(exc))

    if getattr(response, "status_code", None) not in (200, None):  # type: ignore[arg-type]
        try:
            response.close()
        except Exception:
            pass
        raise RuntimeError("Vision service returned status {}".format(response.status_code))

    try:
        payload = response.json()
    finally:
        response.close()

    if not isinstance(payload, dict) or "description" not in payload:
        raise RuntimeError("Vision service returned unexpected payload: {}".format(payload))

    return payload["description"]


def main() -> None:
    init_camera()

    while True:
        gc.collect()
        print("Capturing frame...")
        frame = capture_frame()
        print("Captured {} bytes".format(len(frame)))

        try:
            description = describe_frame(frame)
        except Exception as exc:
            print("Vision request failed:", exc)
        else:
            print("Vision description:", description)

        time.sleep(CAPTURE_INTERVAL_S)


if __name__ == "__main__":
    main()
