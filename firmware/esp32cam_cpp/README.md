# ESP32-CAM Visão + Voz (C++)

Projeto em C++ (Arduino / PlatformIO) que permite à ESP32-CAM (câmera OV2640) capturar imagens, enviar para um serviço de visão computacional e, em seguida, sintetizar voz informando o objeto identificado.

## Visão geral
- A ESP32-CAM captura um frame JPEG (`FRAMESIZE_QVGA`) periodicamente.
- O firmware envia o JPEG bruto via HTTP (`POST /infer`) para um servidor em rede local.
- O servidor executa uma rede YOLO (modelo `yolov8n.pt` por padrão) e devolve um JSON com os objetos detectados.
- O firmware fala em voz alta o rótulo com maior confiança utilizando o sintetizador SAM (biblioteca `ESP8266SAM`) via saídas I2S.

## Hardware
- ESP32-CAM (módulo AI-Thinker ou equivalente com PSRAM).
- Módulo amplificador I2S (ex.: MAX98357A) + alto-falante de 4–8 Ω.
- Alimentação estável (>= 5 V / 2 A recomendável).

### Ligações sugeridas (ESP32-CAM → Amplificador I2S)
| Sinal | ESP32-CAM | Amplificador |
|-------|-----------|--------------|
| BCK   | GPIO12    | BCLK         |
| WS    | GPIO13    | LRCLK        |
| DATA  | GPIO15    | DIN          |
| 5 V   | 5V        | VCC          |
| GND   | GND       | GND          |

> ⚠️ GPIO12/13/15 são pinos de boot strap; mantenha o amplificador desligado durante o flash para evitar conflitos. Após o upload, ligue o amplificador.

## Configuração do firmware
1. Duplique `include/app_config.h` como `include/app_config_local.h` e preencha:
   ```cpp
   #define WIFI_SSID     "MinhaRede"
   #define WIFI_PASSWORD "SenhaDaRede"
   #define INFERENCE_HOST "192.168.0.10"
   #define INFERENCE_PORT 8000
   #define INFERENCE_PATH "/infer"
   ```
2. Ajuste os pinos I2S caso sua montagem utilize outros GPIOs.

## Compilação e upload (PlatformIO)
```bash
cd firmware/esp32cam_cpp
pio run                   # compila
pio run -t upload         # grava via USB/serial
pio device monitor        # abre o monitor serial (115200 bps)
```

> Se utilizar a IDE Arduino, importe o diretório `src/` e ajuste as bibliotecas manualmente (`ArduinoJson`, `esp32-camera`, `ESP8266Audio`, `ESP8266SAM`).

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
  3. Analisa o JSON com `ArduinoJson` e seleciona o objeto com maior confiança.
  4. Mostra no monitor serial e gera frase (`"DETECTED <LABEL> <CONFIDENCE> PERCENT"`).
  5. Fala a frase usando `SAM::Say`.

## Ajustes úteis
- Reduza `kInferenceIntervalMs` para inferências mais frequentes.
- Altere `config.frame_size`/`jpeg_quality` conforme seu enlace Wi-Fi.
- Substitua o servidor por outro serviço (Edge Impulse, Google Vision, etc.) mantendo o formato de resposta.
- Adapte o texto falado em `buildSpeechPhrase` (atualmente em inglês por compatibilidade com o SAM).

## Limitações e próximos passos
- YOLOv8 roda no servidor externo; a ESP32-CAM apenas captura e envia imagens.
- O sintetizador SAM possui voz robótica e suporte limitado ao português. Integre outra lib de TTS (ex.: `ESP32-TTS`, `Flite`, streaming de áudio gerado no servidor) para melhor resultado.
- Para operação offline, treine e converta um modelo leve (por exemplo, Edge Impulse FOMO) para rodar na ESP32, mas isso exigirá otimizações e ajustes significativos.

## Licença
Consulte as licenças das dependências incluídas (Ultralytics YOLO, ESP8266Audio/SAM, etc.) antes de uso comercial.
