"""MQTT云端客户端 - 板子端，自动重连 + LWT遗嘱"""
import json
import time
import paho.mqtt.client as mqtt

# 公共测试Broker，后续换为私有云Broker
DEFAULT_BROKER = "broker.emqx.io"
DEFAULT_PORT = 1883
DEFAULT_DEVICE = "board1"

TOPIC_SENTENCE = "glove/{device}/sentence"
TOPIC_PREDICTION = "glove/{device}/prediction"
TOPIC_STATUS = "glove/{device}/status"
TOPIC_PINYIN = "glove/{device}/pinyin"


class MQTTClient:
    def __init__(self, device_id=DEFAULT_DEVICE):
        self.device_id = device_id
        self.connected = False
        self._client = mqtt.Client(
            mqtt.CallbackAPIVersion.VERSION2,
            client_id=f"glove_{device_id}_{int(time.time())}",
            protocol=mqtt.MQTTv5,
        )
        self._client.will_set(
            TOPIC_STATUS.format(device=device_id),
            json.dumps({"online": False}),
            qos=1, retain=True,
        )
        self._client.on_connect = self._on_connect
        self._client.on_disconnect = self._on_disconnect
        self._client.on_message = self._on_message
        self._cmd_handlers = {}
        # paho 内置自动重连
        self._client.reconnect_delay_set(min_delay=3, max_delay=15)

    def _on_connect(self, client, userdata, flags, reason_code, properties=None):
        if reason_code == 0:
            self.connected = True
            client.publish(
                TOPIC_STATUS.format(device=self.device_id),
                json.dumps({"online": True, "version": "v9"}),
                qos=1, retain=True,
            )
            client.subscribe(f"glove/{self.device_id}/cmd", qos=1)
            print(f"[MQTT] 已连接")
        else:
            self.connected = False
            print(f"[MQTT] 连接失败 rc={reason_code}")

    def _on_disconnect(self, client, userdata, flags, reason_code, properties=None):
        self.connected = False
        if reason_code != 0:
            print(f"[MQTT] 断开 (rc={reason_code}), 自动重连中...")

    def _on_message(self, client, userdata, msg):
        try:
            payload = json.loads(msg.payload.decode())
            cmd = payload.get("cmd", "")
            if cmd in self._cmd_handlers:
                self._cmd_handlers[cmd](payload)
        except Exception as e:
            print(f"[MQTT] 消息解析失败: {e}")

    def on_cmd(self, cmd_name):
        """注册命令处理回调"""
        def wrapper(fn):
            self._cmd_handlers[cmd_name] = fn
            return fn
        return wrapper

    def connect(self, broker=DEFAULT_BROKER, port=DEFAULT_PORT):
        self._client.connect(broker, port, keepalive=60)
        self._client.loop_start()  # paho 内置后台线程

    def publish_sentence(self, text):
        """发布识别句子 (QoS 1)"""
        if not self.connected:
            return
        self._client.publish(
            TOPIC_SENTENCE.format(device=self.device_id),
            json.dumps({"text": text, "ts": time.time()}, ensure_ascii=False),
            qos=1,
        )

    def publish_prediction(self, label, confidence, latency_ms, mode='gesture'):
        """发布每次推理结果 (QoS 0, 高频)

        mode: 'gesture' 手势 / 'pinyin' 拼音, 供网页端区分显示
        """
        if not self.connected:
            return
        self._client.publish(
            TOPIC_PREDICTION.format(device=self.device_id),
            json.dumps({
                "label": label, "confidence": round(confidence, 3),
                "latency_ms": round(latency_ms, 1), "ts": time.time(),
                "mode": mode,
            }, ensure_ascii=False),
            qos=0,
        )

    def publish_status(self, info):
        """发布状态/心跳"""
        if not self.connected:
            return
        self._client.publish(
            TOPIC_STATUS.format(device=self.device_id),
            json.dumps({"online": True, **info}, ensure_ascii=False),
            qos=1, retain=True,
        )

    def publish_pinyin_state(self, state):
        """发布拼音累积状态 (QoS 0, 高频)"""
        if not self.connected:
            return
        self._client.publish(
            TOPIC_PINYIN.format(device=self.device_id),
            json.dumps(state, ensure_ascii=False),
            qos=0,
        )

    def stop(self):
        if self.connected:
            self.publish_status({"online": False})
            self._client.disconnect()
        self._client.loop_stop()
