"""麦克风语音练习: PC 麦克风 → faster-whisper → MQTT → 前端

不依赖 F4 / 串口, 单独跑就能把说的话转成文字显示在前端界面。
发布 glove/board1/asr {"text","ts"}, 后端 mqtt_bridge 转 SSE 'asr' 事件,
前端 app.js 收到后显示为 learner 语音。

跑法:
    D:\\Anaconda_envs\\envs\\pytorch\\python.exe asr_practice.py
Ctrl+C 退出。

env 可覆盖:
    DEVICE_ID / MQTT_BROKER / MQTT_PORT
    ASR_ENGINE      auto(默认, 云优先+本地回退) | cloud | local
    BAIDU_API_KEY / BAIDU_SECRET_KEY / BAIDU_DEV_PID   云端(百度)凭证
    ASR_MODEL_SIZE / ASR_DEVICE                          本地 whisper 参数
"""
import json
import os
import time

from asr_whisper import SpeechRecognizer
from mqtt_client import MQTTClient

DEVICE_ID = os.environ.get('DEVICE_ID', 'board1')
MQTT_BROKER = os.environ.get('MQTT_BROKER', '127.0.0.1')
MQTT_PORT = int(os.environ.get('MQTT_PORT', '1883'))
ASR_MODEL_SIZE = os.environ.get('ASR_MODEL_SIZE', 'small')
ASR_DEVICE = os.environ.get('ASR_DEVICE', 'cuda')
ASR_ENGINE = os.environ.get('ASR_ENGINE', 'auto')   # auto(云优先) | cloud | local


def main():
    print('=== 语音练习 (麦克风 → 前端) ===')

    mqtt = None
    try:
        mqtt = MQTTClient(device_id=DEVICE_ID)
        mqtt.connect(MQTT_BROKER, MQTT_PORT)
        print(f'[MQTT] 连接 {MQTT_BROKER}:{MQTT_PORT} (设备:{DEVICE_ID})')
        # 等连接建立, 否则前几句发不出去
        for _ in range(30):
            if mqtt.connected:
                break
            time.sleep(0.1)
        if mqtt.connected:
            mqtt.publish_status({'version': 'pc_asr_practice'})
        else:
            print('[MQTT] 未连上 broker, 转写结果无法送前端 (先启动 Mosquitto)')
    except Exception as e:
        print(f'[MQTT] 连接失败: {e}')
        mqtt = None

    def on_text(text):
        print(f'\n[ASR:{asr.last_engine or ASR_ENGINE}] {text}')
        if mqtt and mqtt.connected:
            mqtt._client.publish(
                f'glove/{DEVICE_ID}/asr',
                json.dumps({'text': text, 'ts': time.time()}, ensure_ascii=False),
                qos=1)

    print(f'初始化语音引擎 (engine={ASR_ENGINE})...')
    asr = SpeechRecognizer(model_size=ASR_MODEL_SIZE, device=ASR_DEVICE,
                           engine=ASR_ENGINE)
    if not asr.available:
        print('[ASR] 模型不可用, 退出')
        if mqtt:
            mqtt.stop()
        return

    asr.start(callback=on_text)
    print('\n对着麦克风说话, 识别结果会打到前端界面; Ctrl+C 退出\n')
    try:
        while True:
            time.sleep(0.5)
    except KeyboardInterrupt:
        pass
    finally:
        asr.stop()
        if mqtt:
            mqtt.stop()
        print('\n已退出')


if __name__ == '__main__':
    main()
