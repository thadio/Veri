#pragma once

// Copy this file to app_config_local.h and fill in your values.
// The build will prefer app_config_local.h if present.

// Wi-Fi credentials
#define WIFI_SSID     "REPLACE_WITH_SSID"
#define WIFI_PASSWORD "REPLACE_WITH_PASSWORD"

// Inference service endpoint (HTTP)
#define INFERENCE_HOST "192.168.0.100"
#define INFERENCE_PORT 8000
#define INFERENCE_PATH "/infer"

// Audio playback hardware configuration
#define I2S_BCK_PIN   12
#define I2S_WS_PIN    13
#define I2S_DATA_PIN  15
