#include <Arduino.h>
#include <WiFi.h>
#include <esp_camera.h>
#include <WebServer.h>

#if __has_include("app_config_local.h")
#include "app_config_local.h"
#else
#include "app_config.h"
#endif

namespace {

WebServer server(80);

constexpr uint32_t kWiFiTimeoutMs = 20000;

void handleStream() {
  WiFiClient client = server.client();
  if (!client) {
    return;
  }

  client.setTimeout(5);
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
  client.println("Connection: close");
  client.println();

  while (client.connected()) {
    camera_fb_t *frame = esp_camera_fb_get();
    if (!frame) {
      Serial.println("[ERRO] Falha ao capturar frame");
      break;
    }

    client.println("--frame");
    client.println("Content-Type: image/jpeg");
    client.printf("Content-Length: %u\r\n\r\n", frame->len);
    client.write(frame->buf, frame->len);
    client.println();

    esp_camera_fb_return(frame);

    if (!client.connected()) {
      break;
    }
    delay(10);
  }

  client.stop();
}

void handleRoot() {
  server.send(200, "text/html",
              "<html><head><title>ESP32-CAM Stream</title></head>"
              "<body><h2>ESP32-CAM</h2><img src=\"/stream\" /></body></html>");
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
  config.frame_size = FRAMESIZE_VGA;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 2;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("[ERRO] esp_camera_init falhou");
    return false;
  }
  return true;
}

bool connectWiFi() {
  Serial.printf("[WIFI] Conectando a %s...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < kWiFiTimeoutMs) {
    delay(500);
    Serial.print('.');
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\n[ERRO] Falha ao conectar ao Wi-Fi");
    return false;
  }

  Serial.println("\n[WIFI] Conectado!");
  Serial.print("[WIFI] IP: ");
  Serial.println(WiFi.localIP());
  return true;
}

}  // namespace

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println("ESP32-CAM Stream");

  if (!initCamera()) {
    Serial.println("Reiniciando em 5s...");
    delay(5000);
    ESP.restart();
  }

  if (!connectWiFi()) {
    Serial.println("Reiniciando em 5s...");
    delay(5000);
    ESP.restart();
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/stream", HTTP_GET, handleStream);
  server.begin();

  Serial.println("[HTTP] Acesse http://<IP>/ para visualizar o vídeo");
}

void loop() {
  server.handleClient();
  delay(5);
}
