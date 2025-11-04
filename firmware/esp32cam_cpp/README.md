# ESP32-CAM Visão (C++)

Projeto em C++ (Arduino / PlatformIO) que permite à ESP32-CAM (câmera OV2640) capturar imagens, enviar para um serviço de visão computacional e descrever os objetos identificados via monitor serial.

## Visão geral
- A ESP32-CAM captura um frame JPEG (`FRAMESIZE_QVGA`) periodicamente.
- O firmware envia o JPEG bruto via HTTP (`POST /infer`) para um servidor em rede local.
- O servidor executa uma rede YOLO (modelo `yolov8n.pt` por padrão) e devolve um JSON com os objetos detectados.
- O firmware mostra no monitor serial os objetos detectados e destaca o mais provável.

## Hardware
- ESP32-CAM (módulo AI-Thinker ou equivalente com PSRAM).
- Cabo USB + base ESP32-CAM-MB ou adaptador serial.
- Alimentação estável (>= 5 V / 2 A recomendável).

## Configuração do firmware
1. Duplique `include/app_config.h` como `include/app_config_local.h` e preencha:
   ```cpp
   #define WIFI_SSID     "MinhaRede"
   #define WIFI_PASSWORD "SenhaDaRede"
   #define INFERENCE_HOST "192.168.0.10"
   #define INFERENCE_PORT 8000
   #define INFERENCE_PATH "/infer"
   ```
2. Ajuste `INFERENCE_HOST`/`PORT/PATH` para combinar com o servidor que você executará.

## Compilação e upload (PlatformIO)
```bash
cd firmware/esp32cam_cpp
pio run                   # compila
pio run -t upload         # grava via USB/serial
pio device monitor        # abre o monitor serial (115200 bps)
```

> Se utilizar a IDE Arduino, importe o diretório `src/` e instale as bibliotecas `ArduinoJson` (>= 6.21) e certifique-se de que o núcleo ESP32 esteja atualizado (inclui `esp32-camera`).

## Servidor de inferência
O diretório `scripts/` contém um servidor FastAPI (`inference_server.py`) que usa YOLOv8 para inferência.

### Dependências
```bash
python -m venv .venv
source .venv/bin/activate
pip install -r firmware/esp32cam_cpp/scripts/requirements.txt
ultralytics download yolov8n.pt   # opcional; o script baixa automaticamente se necessário
```

### Execução
```bash
cd firmware/esp32cam_cpp/scripts
uvicorn inference_server:app --host 0.0.0.0 --port 8000
```

> Ajuste `CONFIDENCE_THRESHOLD` ou `YOLO_MODEL_PATH` via variáveis de ambiente para trocar o modelo ou filtrar resultados.

### Resposta esperada
```json
{
  "objects": [
    {"label": "person", "confidence": 0.82},
    {"label": "dog", "confidence": 0.61}
  ]
}
```

## Funcionamento do firmware (`src/main.cpp`)
- Conecta ao Wi-Fi (`ensureWiFi`).
- Inicializa câmera (`esp_camera`) e sintetizador SAM (`AudioOutputI2S`).
- A cada ~8 s:
  1. Captura `camera_fb_t`.
  2. Envia via HTTP (JPEG binário) para `INFERENCE_HOST:INFERENCE_PORT/INFERENCE_PATH`.
  3. Analisa o JSON com `ArduinoJson`, gera um resumo textual dos objetos e o escreve no monitor serial.
  4. Destaca o objeto mais provável com sua confiança.

## Ajustes úteis
- Reduza `kInferenceIntervalMs` para inferências mais frequentes.
- Altere `config.frame_size`/`jpeg_quality` conforme seu enlace Wi-Fi.
- Substitua o servidor por outro serviço (Edge Impulse, Google Vision, etc.) mantendo o formato de resposta.
- Adapte o texto falado em `buildSpeechPhrase` (atualmente em inglês por compatibilidade com o SAM).

## Limitações e próximos passos
- YOLOv8 roda no servidor externo; a ESP32-CAM apenas captura e envia imagens.
- Para saída de áudio, considere integrar futuramente um sintetizador de voz ou reproduzir áudio pré-gravado, adicionando o hardware de som correspondente.
- Para operação offline, treine e converta um modelo leve (por exemplo, Edge Impulse FOMO) para rodar na ESP32, mas isso exigirá otimizações e ajustes significativos.

## Licença
Consulte as licenças das dependências incluídas (Ultralytics YOLO, ESP8266Audio/SAM, etc.) antes de uso comercial.
