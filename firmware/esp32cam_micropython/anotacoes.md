Garantindo MicroPython com suporte à câmera

Identifique sua placa: confirme que é uma ESP32-CAM (geralmente com câmera OV2640). Isso ajuda a baixar o arquivo correto.
Baixe o firmware certo: acesse https://micropython.org/download/ESP32/ e procure builds marcados como “ESP32-CAM” ou “with camera support”. Baixe o .bin mais recente, por exemplo esp32-cam-...bin.

Instale o firmware
Instale esptool.py (pip install esptool).
Conecte o ESP32-CAM-MB ao PC via USB e descubra a porta 

(ls /dev/tty.* no macOS/Linux ou verifique o Gerenciador de Dispositivos no Windows).

Apague e grave o firmware:
esptool.py --chip esp32 --port /dev/ttyUSB0 erase_flash
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 460800 write_flash -z 0x1000 esp32-cam-20240430-v1.22.1.bin


Confirme o módulo camera
Instale mpremote (pip install mpremote) ou use um terminal serial.
Conecte: mpremote connect /dev/ttyUSB0.
No REPL, digite:
import camera
camera.init()
Se não houver erro e camera.init() retornar 0, o módulo está presente.
Teste rápido da câmera
Capture um frame para garantir funcionamento:
camera.framesize(camera.FRAME_QVGA)
buf = camera.capture()
len(buf)