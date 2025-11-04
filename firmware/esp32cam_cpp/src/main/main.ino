#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <esp_camera.h>
#include <ArduinoJson.h>

#if __has_include("app_config_local.h")
#include "app_config_local.h"
#else
#include "app_config.h"
#endif

namespace {

constexpr uint32_t kWiFiConnectTimeoutMs = 20000;
constexpr uint32_t kInferenceIntervalMs = 8000;
constexpr uint32_t kHttpTimeoutMs = 15000;

uint32_t lastInferenceMs = 0;

void logError(const char *message) {
  Serial.println();
  Serial.print(F("[ERRO] "));
  Serial.println(message);
}

bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5;
  config.pin_d1 = 18;
  config.pin_d2 = 19;
  config.pin_d3 = 21;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 0;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn = 32;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_QVGA;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("[ERRO] esp_camera_init falhou: 0x%x\n", err);
    return false;
  }

  sensor_t *sensor = esp_camera_sensor_get();
  if (sensor == nullptr) {
    logError("Falha ao obter sensor da câmera");
    return false;
  }
  sensor->set_brightness(sensor, 0);
  sensor->set_contrast(sensor, 0);
  sensor->set_saturation(sensor, 0);
  sensor->set_framesize(sensor, FRAMESIZE_QVGA);
  sensor->set_whitebal(sensor, 1);
  sensor->set_exposure_ctrl(sensor, 1);
  return true;
}

bool ensureWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  Serial.printf("[WIFI] Conectando a %s ...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < kWiFiConnectTimeoutMs) {
    delay(250);
    Serial.print('.');
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println();
    logError("Não foi possível conectar ao Wi-Fi");
    return false;
  }

  Serial.println();
  Serial.print(F("[WIFI] Conectado. IP: "));
  Serial.println(WiFi.localIP());
  return true;
}

bool sendFrameForInference(camera_fb_t *frameBuffer, String &responseBody) {
  WiFiClient client;
  if (!client.connect(INFERENCE_HOST, INFERENCE_PORT)) {
    logError("Conexão com o serviço de inferência falhou");
    return false;
  }

  client.setTimeout(kHttpTimeoutMs / 1000);
  const String path = INFERENCE_PATH;
  const String host = INFERENCE_HOST;
  client.printf("POST %s HTTP/1.1\r\n", path.c_str());
  client.printf("Host: %s:%d\r\n", host.c_str(), INFERENCE_PORT);
  client.println("Content-Type: image/jpeg");
  client.printf("Content-Length: %u\r\n", frameBuffer->len);
  client.println("Connection: close");
  client.println();
  client.write(frameBuffer->buf, frameBuffer->len);
  client.flush();

  uint32_t start = millis();
  while (client.connected() && !client.available()) {
    if (millis() - start > kHttpTimeoutMs) {
      logError("Timeout aguardando resposta do servidor");
      client.stop();
      return false;
    }
    delay(10);
  }

  String response = client.readString();
  client.stop();

  int headerEnd = response.indexOf("\r\n\r\n");
  if (headerEnd < 0) {
    logError("Resposta HTTP inválida");
    return false;
  }
  responseBody = response.substring(headerEnd + 4);
  return true;
}

bool parseInference(const String &responseBody, String &summary, String &bestLabel, float &bestConfidence) {
  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, responseBody);
  if (err) {
    Serial.printf("[ERRO] JSON inválido: %s\n", err.c_str());
    return false;
  }

  JsonArray objects = doc["objects"].as<JsonArray>();
  if (objects.isNull() || objects.empty()) {
    logError("Nenhum objeto identificado");
    return false;
  }

  float maxConfidence = -1.0f;
  String topLabel;
  String detected = "Objetos detectados: ";
  bool first = true;

  for (JsonObject obj : objects) {
    const char *candidate = obj["label"] | "";
    float score = obj["confidence"] | 0.0f;
    if (score > maxConfidence) {
      maxConfidence = score;
      topLabel = candidate;
    }

    if (!first) {
      detected += ", ";
    }
    detected += candidate;
    detected += " (";
    detected += String(score * 100.0f, 1);
    detected += "%)";
    first = false;
  }

  if (topLabel.isEmpty()) {
    logError("Objeto sem rótulo");
    return false;
  }

  bestLabel = topLabel;
  bestConfidence = maxConfidence;
  summary = detected;
  return true;
}

void performInference() {
  camera_fb_t *frameBuffer = esp_camera_fb_get();
  if (!frameBuffer) {
    logError("Falha ao capturar frame da câmera");
    return;
  }

  String body;
  bool sent = sendFrameForInference(frameBuffer, body);
  esp_camera_fb_return(frameBuffer);
  if (!sent) {
    return;
  }

  String summary;
  String bestLabel;
  float confidence = 0.0f;
  if (!parseInference(body, summary, bestLabel, confidence)) {
    return;
  }

  Serial.println();
  Serial.println(F("[INFERÊNCIA]"));
  Serial.println(summary);
  Serial.printf("Mais provável: %s (%.1f%%)\n", bestLabel.c_str(), confidence * 100.0f);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println(F("ESP32-CAM Visão e Voz"));

  if (!initCamera()) {
    logError("Inicialização da câmera falhou. Reiniciando em 5s.");
    delay(5000);
    ESP.restart();
  }
}

void loop() {
  if (!ensureWiFi()) {
    delay(2000);
    return;
  }

  const uint32_t now = millis();
  if (now - lastInferenceMs > kInferenceIntervalMs) {
    lastInferenceMs = now;
    performInference();
  }
}
