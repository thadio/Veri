# ESP32-CAM MJPEG Stream

Firmware simples para visualizar a imagem da ESP32-CAM (módulo AI-Thinker com câmera OV2640) via navegador, usando streaming MJPEG.

## Como funciona
- O ESP32-CAM conecta-se à rede Wi-Fi definida em `app_config.h`.
- Cria um servidor HTTP na porta 80.
- A rota `/stream` envia quadros JPEG contínuos (`multipart/x-mixed-replace`), permitindo visualizar o vídeo em tempo real.
- A rota `/` mostra uma página HTML básica com a tag `<img>` apontando para `/stream`.

## Preparação (Arduino IDE)
1. Adicione a URL do pacote ESP32 nas Preferências:
   ```
   https://dl.espressif.com/dl/package_esp32_index.json
   ```
2. Instale o pacote “esp32” (Espressif Systems) pelo Gerenciador de Placas.
3. Crie um novo sketch chamado `esp32cam_stream` ou copie esta pasta para `Documents/Arduino/esp32cam_stream`.
4. Copie `include/app_config.h` para `include/app_config_local.h` e edite:
   ```cpp
   #define WIFI_SSID     "MinhaRede"
   #define WIFI_PASSWORD "MinhaSenha"
   ```

## Ajustes na IDE
- Placa: `AI Thinker ESP32-CAM`
- Porta: selecione a porta serial do ESP32-CAM-MB.
- Flash Mode: `QIO`
- Flash Frequency: `40MHz`
- Partition Scheme: `Huge APP (3MB No OTA/1MB SPIFFS)` ou equivalente.
- Upload Speed: 921600 (opcionalmente 115200 se tiver erros).

## Upload / Teste
1. Clique em “Verificar” para compilar. Depois clique em “Upload”.
   - Se estiver usando a base ESP32-CAM-MB, não é preciso alterar pinos de boot.
   - Se usar adaptador serial, mantenha IO0 ligado ao GND durante o upload.
2. Abra o Monitor Serial (115200 bps) para conferir o IP exibido após a conexão Wi-Fi:
   ```
   [WIFI] IP: 192.168.0.42
   [HTTP] Acesse http://192.168.0.42/ para visualizar o vídeo
   ```
3. No navegador (no mesmo Wi-Fi), acesse `http://<IP>/` e veja o streaming.

## Dicas
- Melhorar o FPS: reduza `config.frame_size = FRAMESIZE_VGA;` para `FRAMESIZE_QVGA`.
- Controle do flash LED: adicione `pinMode(4, OUTPUT); digitalWrite(4, HIGH);` para iluminar a cena.
- Segurança: o servidor não possui autenticação; use apenas em redes confiáveis.

## Alternativas
- Use o exemplo oficial `File > Examples > ESP32 > Camera > CameraWebServer` para recursos adicionais (captura com controle manual).
- Integre este stream a ferramentas como VLC (`Media > Open Network Stream` com `http://<IP>/stream`).
