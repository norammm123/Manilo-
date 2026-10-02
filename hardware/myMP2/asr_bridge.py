"""ASR语音识别 → MQTT桥接 (板子端运行)"""
import json
import time
import sys
import os
import signal

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from mqtt_client import MQTTClient

# ASR模块路径
ASR_PATH = '/opt/gesture_glove/src/speech_recognizer.py'
sys.path.insert(0, '/opt/gesture_glove/src')

# MQTT配置
BROKER = os.environ.get('MQTT_BROKER', '10.250.167.245')
PORT = int(os.environ.get('MQTT_PORT', '1883'))
DEVICE_ID = os.environ.get('DEVICE_ID', 'board1')

mqtt = None
asr = None


def on_text(text):
    """ASR识别回调 → MQTT发布"""
    print(f"\r[ASR] {text}")
    if mqtt and mqtt.connected:
        mqtt._client.publish(
            f"glove/{DEVICE_ID}/asr",
            json.dumps({"text": text, "ts": time.time()}, ensure_ascii=False),
            qos=1,
        )


def main():
    global mqtt, asr

    print("=== ASR → MQTT 桥接 ===")

    # 连接MQTT
    print(f"[MQTT] 连接 {BROKER}:{PORT} ...")
    mqtt = MQTTClient(device_id=DEVICE_ID)
    mqtt.connect(BROKER, PORT)
    print(f"[MQTT] 已连接 (设备:{DEVICE_ID})")

    # 启动语音识别
    from speech_recognizer import SpeechRecognizer
    model_path = '/opt/gesture_glove/models/vosk-model-small-cn-0.22'
    asr = SpeechRecognizer(model_path=model_path)
    if asr.available:
        asr.start(callback=on_text)
        print("[ASR] 语音识别已启动, 对着麦克风说话...")
    else:
        print("[ASR] 不可用")
        return

    def cleanup(sig, frame):
        print("\n停止...")
        if asr:
            asr.stop()
        if mqtt:
            mqtt.stop()
        sys.exit(0)

    signal.signal(signal.SIGINT, cleanup)
    signal.signal(signal.SIGTERM, cleanup)

    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        cleanup(None, None)


if __name__ == '__main__':
    main()
