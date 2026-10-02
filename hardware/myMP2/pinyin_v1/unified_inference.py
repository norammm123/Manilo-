"""统一推理脚本 — 手势/拼音双模式 + ASR串口回传"""
import os
# 限制 ONNX Runtime 线程数, 防止双模型加载时线程池死锁
os.environ.setdefault('OMP_NUM_THREADS', '1')
os.environ.setdefault('ORT_INTRA_OP_NUM_THREADS', '1')

import numpy as np
import json
import time
import subprocess
import sys
import threading
from collections import deque, Counter

_SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, _SCRIPT_DIR)
sys.path.insert(0, os.path.dirname(_SCRIPT_DIR))
sys.path.insert(0, '/opt/gesture_glove/src')  # ASR SpeechRecognizer

from pinyin_tracker import PinyinTracker
from pinyin_ime import PinyinIME
from mqtt_client import MQTTClient

# === MQTT ===
MQTT_BROKER = os.environ.get('MQTT_BROKER', '10.233.108.145')
MQTT_PORT = int(os.environ.get('MQTT_PORT', '1883'))
MQTT_ENABLE = os.environ.get('MQTT_ENABLE', '1') == '1'
DEVICE_ID = os.environ.get('DEVICE_ID', 'board1')

# === 串口 ===
SERIAL_PORT = '/dev/ttySTM9'
SERIAL_BAUD = 115200

# === 推理 ===
NUM_FEATURES = 11
WINDOW_FRAMES = 200
INFER_INTERVAL = 0.15  # 推理间隔: 字母确认 = VOTE_WINDOW×此值, 0.15 时 7票≈1.05s

# === 手势追踪 ===
VOTE_WINDOW = 7
VOTE_MAJORITY = 4
CONFIDENCE_THRESH = 0.6
MIN_MARGIN = 0.10
WORD_COOLDOWN = 1.5
SENSOR_ACTIVE_THRESH = 200  # 弯曲传感器偏离放松态的阈值 (scaled ADC)
MIN_ACTIVE_CHANNELS = 3     # 至少3个手指有明显动作才算"在打手势"

# === 拼音追踪 ===
LETTER_COOLDOWN = 0.8
PAUSE_COMMIT_S = 1.0  # 停顿提交: 松手后等待这么久, 一次性转换整串并播报
PAUSE_COMMIT_FINAL_S = 3.0  # 末尾音节可续写(可能还在输入)时, 最多再等这么久才强制提交, 防半字
                            # 从 last_pinyin_feed 计: 慢速拼字(字母间松手2s内)不会被误判为停手

# === 模型路径 ===
MODEL_DIR_GESTURE = os.environ.get('GESTURE_MODEL_DIR', '/root/sign_language/models/gesture0718')
MODEL_DIR_PINYIN = '/root/sign_language/models/pinyin_board'
ASR_MODEL_PATH = '/opt/gesture_glove/models/vosk-model-small-cn-0.22'

THRESHOLDS_FILE = '/root/sign_language/unified_thresholds.json'

# === 帧定义 ===
FRAME_MODE_GESTURE = 0xFF  # 手势模式
FRAME_MODE_PINYIN = 0xEE   # 拼音模式
FRAME_MODE_VOICE = 0xFE    # 语音模式
FRAME_START = 0xAA
FRAME_END = 0xBB
MODE_BYTES = (FRAME_MODE_GESTURE, FRAME_MODE_PINYIN, FRAME_MODE_VOICE)
PROTO_B_START1 = 0xFE  # 协议B: 手势/拼音结果
PROTO_B_START2 = 0xFF
PROTO_B_END1 = 0xFF
PROTO_B_END2 = 0xEF
PROTO_A_START1 = 0xFE  # 协议A: 语音识别结果
PROTO_A_START2 = 0xEF
PROTO_A_END1 = 0xEF
PROTO_A_END2 = 0xFE

# ============================================================
# 工具函数
# ============================================================

def _find_file(name, search_dirs):
    for d in search_dirs:
        p = os.path.join(d, name)
        if os.path.exists(p):
            return p
    return os.path.join(search_dirs[0], name)


def load_thresholds():
    if os.path.exists(THRESHOLDS_FILE):
        try:
            with open(THRESHOLDS_FILE, 'r', encoding='utf-8') as f:
                return json.load(f)
        except Exception:
            pass
    return {}


def save_thresholds_local(data):
    try:
        os.makedirs(os.path.dirname(THRESHOLDS_FILE), exist_ok=True)
        with open(THRESHOLDS_FILE, 'w', encoding='utf-8') as f:
            json.dump(data, f, ensure_ascii=False, indent=2)
    except Exception as e:
        print(f'[阈值] 保存失败: {e}')


def speak(text):
    """espeak TTS 中文播报"""
    try:
        subprocess.run(
            ['espeak', '-v', 'zh', '-s', '160', '-a', '100',
             '-w', '/tmp/tts.wav', text],
            timeout=5, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        subprocess.run(
            ['aplay', '-q', '-D', 'plughw:0,0', '/tmp/tts.wav'],
            timeout=5, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except Exception as e:
        print(f'[TTS] 失败: {e}')


# ============================================================
# 模型加载
# ============================================================

def load_gesture_model():
    from stai_mpu import stai_mpu_network
    search = [MODEL_DIR_GESTURE, _SCRIPT_DIR]
    onnx_path = _find_file('sign_language.onnx', search)
    if not os.path.exists(onnx_path):
        raise FileNotFoundError(f'手势模型未找到: sign_language.onnx 搜索={search}')
    net = stai_mpu_network(onnx_path, use_hw_acceleration=False)
    print(f'[手势模型] {onnx_path}')
    print(f'[手势模型] Backend: {net.get_backend_engine()}')
    return net


def load_pinyin_model():
    from stai_mpu import stai_mpu_network
    search = [MODEL_DIR_PINYIN, _SCRIPT_DIR,
              os.path.join(_SCRIPT_DIR, 'checkpoints')]
    onnx_path = _find_file('pinyin_model.onnx', search)
    if not os.path.exists(onnx_path):
        raise FileNotFoundError(f'拼音模型未找到: pinyin_model.onnx 搜索={search}')
    net = stai_mpu_network(onnx_path, use_hw_acceleration=False)
    print(f'[拼音模型] {onnx_path}')
    print(f'[拼音模型] Backend: {net.get_backend_engine()}')
    return net


def load_scaler(model_dir, search_extra=None):
    search = [model_dir, _SCRIPT_DIR]
    if search_extra:
        search = list(search_extra) + search
    data = np.load(_find_file('scaler.npz', search))
    mean, std = data['mean'], data['std']
    # 兼容20特征scaler → 取前NUM_FEATURES个
    if mean.ndim == 2 and mean.shape[1] > NUM_FEATURES:
        mean = mean[:, :NUM_FEATURES]
        std = std[:, :NUM_FEATURES]
    if mean.ndim == 2 and mean.shape[0] == 1:
        mean = mean.squeeze(0)
        std = std.squeeze(0)
    # 加载clip阈值 (新版scaler包含, 旧版兼容)
    clip_lo = data.get('clip_lo', None)
    clip_hi = data.get('clip_hi', None)
    # 加载ADC放松态基线 (scaler.npz内嵌; 训练时5路柔性传感器已中心化,
    # 推理必须同样减去, 否则实时数据分布整体偏移导致识别失败)
    adc_baseline = data.get('adc_baseline', None)
    if adc_baseline is not None:
        adc_baseline = np.asarray(adc_baseline, dtype=np.float32).ravel()
        # 只作用于前5路ADC通道, IMU通道(5:11)补0
        full = np.zeros(NUM_FEATURES, dtype=np.float32)
        n = min(5, len(adc_baseline))
        full[:n] = adc_baseline[:n]
        adc_baseline = full
    return mean, std, clip_lo, clip_hi, adc_baseline


def load_labels(model_dir, search_extra=None):
    search = [model_dir, _SCRIPT_DIR]
    if search_extra:
        search = list(search_extra) + search
    path = _find_file('label_names.npy', search)
    if os.path.exists(path):
        return np.load(path, allow_pickle=True).tolist()
    return []


# ============================================================
# 串口读写
# ============================================================

class SerialIO:
    """串口帧读写: F4←→MP2"""

    def __init__(self, port=SERIAL_PORT, baud=SERIAL_BAUD):
        self.port = port
        self.baud = baud
        self.fd = None
        self.buf = bytearray()
        self.frame_ok = 0
        self.frame_fail = 0
        self._backend = 'termios'
        self._mode = 'gesture'  # 默认手势模式, 模式字节可覆盖

    def open(self):
        try:
            import serial
            self.ser = serial.Serial(
                port=self.port, baudrate=self.baud, timeout=0.01,
                bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE)
            self._backend = 'pyserial'
            print(f'[串口] {self.port} @ {self.baud} (pyserial)')
            return True
        except ImportError:
            pass
        except Exception as e:
            print(f'[串口] pyserial失败: {e}')

        try:
            import termios
            fd = os.open(self.port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
            tty = termios.tcgetattr(fd)
            baud_map = {9600: termios.B9600, 115200: termios.B115200,
                        921600: termios.B921600, 460800: termios.B460800,
                        57600: termios.B57600, 38400: termios.B38400,
                        19200: termios.B19200}
            tty[4] = baud_map.get(self.baud, termios.B9600)
            tty[5] = tty[4]
            tty[2] = termios.CLOCAL | termios.CREAD | termios.CS8
            tty[0] = 0; tty[1] = 0; tty[3] = 0
            tty[6][termios.VMIN] = 0
            tty[6][termios.VTIME] = 0
            termios.tcsetattr(fd, termios.TCSANOW, tty)
            self.fd = fd
            self._backend = 'termios'
            print(f'[串口] {self.port} @ {self.baud} (termios)')
            return True
        except Exception as e:
            print(f'[串口] 打开失败: {e}')
            return False

    def _read_bytes(self):
        try:
            if self._backend == 'pyserial' and hasattr(self, 'ser'):
                n = self.ser.in_waiting
                if n > 0:
                    data = self.ser.read(n)
                    if data:
                        self.buf.extend(data)
            elif self.fd is not None:
                data = os.read(self.fd, 512)
                if data:
                    self.buf.extend(data)
        except (BlockingIOError, OSError):
            pass

    def read_frame(self):
        """返回 (mode: str, frame: list) 或 (None, None)

        F4发送格式:
          模式切换: 单字节 0xFF(手势) 或 0xEE(拼音)
          数据帧:   0xAA [ASCII CSV 11值] 0xBB
        CSV示例: 1845,-320,1200,890,-450,123,456,789,10,-20,30
        (11个逗号分隔的十进制整数, int32_t范围)
        """
        self._read_bytes()

        # 清除前导零
        zero_end = 0
        for b in self.buf:
            if b == 0x00:
                zero_end += 1
            else:
                break
        if zero_end > 0:
            del self.buf[:zero_end]

        mode_signal = None

        # 检测模式字节 (可能独立出现, 也可能在 0xAA 之前)
        while len(self.buf) > 0 and self.buf[0] in MODE_BYTES:
            b = self.buf[0]
            if b == FRAME_MODE_GESTURE:
                mode_signal = 'gesture'
            elif b == FRAME_MODE_PINYIN:
                mode_signal = 'pinyin'
            else:
                mode_signal = 'voice'
            del self.buf[0]

        if mode_signal is not None:
            self._mode = mode_signal

        # 找帧头 0xAA
        aa_idx = -1
        for i in range(len(self.buf)):
            if self.buf[i] == FRAME_START:
                aa_idx = i
                break

        if aa_idx < 0:
            return (mode_signal, None)

        # 丢弃 0xAA 之前的无效字节
        if aa_idx > 0:
            del self.buf[:aa_idx]

        # 找帧尾 0xBB
        bb_idx = -1
        for i in range(1, len(self.buf)):
            if self.buf[i] == FRAME_END:
                bb_idx = i
                break

        if bb_idx < 0:
            return (mode_signal, None)

        # 提取 ASCII CSV payload
        payload = bytes(self.buf[1:bb_idx])
        del self.buf[:bb_idx + 1]

        try:
            text = payload.decode('ascii', errors='ignore').strip()
            if not text:
                self.frame_fail += 1
                return (mode_signal, None)
            parts = text.split(',')
            if len(parts) < NUM_FEATURES:
                self.frame_fail += 1
                return (mode_signal, None)
            frame = [float(p) for p in parts[:NUM_FEATURES]]
            self.frame_ok += 1
            return (mode_signal or self._mode, frame)
        except (ValueError, UnicodeDecodeError):
            self.frame_fail += 1
            return (mode_signal, None)

    def write_protocol_b(self, text, speak=False):
        """协议B v2: 手势/拼音结果 → F4
        0xFE 0xFF + [ulen] + UTF-8中文 + [glen] + GBK + 0xFF 0xEF
        预览帧 glen=0 只上屏; 确认帧 speak=True 带 GBK 供 F4 播报 TTS
        """
        u = text.encode('utf-8')
        g = text.encode('gbk', errors='ignore') if speak else b''
        packet = bytearray([PROTO_B_START1, PROTO_B_START2, len(u)])
        packet.extend(u)
        packet.extend([len(g)])
        packet.extend(g)
        packet.extend([PROTO_B_END1, PROTO_B_END2])
        self._write_raw(packet)

    def write_protocol_a(self, text):
        """协议A: 语音识别结果 → F4
        0xFE 0xEF + UTF-8中文 + 0xFE 0xEF
        """
        data = text.encode('utf-8')
        packet = bytearray([PROTO_A_START1, PROTO_A_START2])
        packet.extend(data)
        packet.extend([PROTO_A_END1, PROTO_A_END2])
        self._write_raw(packet)

    def send_gesture_result(self, index, confidence):
        """发送手势识别结果 → F4
        0xCC + 下标(0-17) + 置信率(0-100) + 0xDD
        """
        packet = bytearray([0xCC, index & 0xFF, confidence & 0xFF, 0xDD])
        self._write_raw(packet)

    def _write_raw(self, data):
        import errno, select
        buf = bytes(data)
        offset = 0
        deadline = time.time() + 1.0  # 1秒超时

        while offset < len(buf):
            try:
                if self._backend == 'pyserial' and hasattr(self, 'ser'):
                    n = self.ser.write(buf[offset:])
                    offset += n if n else len(buf) - offset
                elif self.fd is not None:
                    # 等待 fd 可写 (最多100ms)
                    _, w, _ = select.select([], [self.fd], [], 0.1)
                    if not w:
                        if time.time() > deadline:
                            print(f'[串口] 写入超时')
                            return
                        continue
                    n = os.write(self.fd, buf[offset:])
                    if n > 0:
                        offset += n
                    elif n == 0:
                        break
            except (BlockingIOError, OSError) as e:
                if hasattr(e, 'errno') and e.errno in (errno.EAGAIN, errno.EWOULDBLOCK):
                    if time.time() > deadline:
                        print(f'[串口] 写入超时')
                        return
                    time.sleep(0.01)
                    continue
                print(f'[串口] 写入失败: {e}')
                return

    def close(self):
        if self._backend == 'pyserial' and hasattr(self, 'ser'):
            self.ser.close()
        elif self.fd is not None:
            try:
                os.close(self.fd)
            except OSError:
                pass


# ============================================================
# 手势追踪 (简化版, 去静默超时)
# ============================================================

class WordTracker:
    def __init__(self):
        self.last_word = None
        self.last_word_time = time.time()
        self.cooldown_until = 0.0
        self.vote_history = deque(maxlen=VOTE_WINDOW)
        self.word_thresholds = {}

    def get_threshold(self, label):
        return self.word_thresholds.get(label, CONFIDENCE_THRESH)

    def feed(self, label, confidence, margin=0.0):
        now = time.time()
        thresh = self.get_threshold(label)

        if margin < MIN_MARGIN:
            return None
        if now < self.cooldown_until and label == self.last_word:
            return None
        if confidence < thresh:
            self.vote_history.append(('', 0))
            return None

        self.vote_history.append((label, confidence))
        if len(self.vote_history) < VOTE_WINDOW:
            return None

        valid = [(l, c) for l, c in self.vote_history if l != '']
        if not valid:
            return None

        vote_counts = Counter(l for l, _ in valid)
        vote_confs = {}
        for l, c in valid:
            vote_confs[l] = vote_confs.get(l, []) + [c]

        best_label, best_count = vote_counts.most_common(1)[0]
        best_avg_conf = sum(vote_confs[best_label]) / len(vote_confs[best_label])
        best_thresh = self.get_threshold(best_label)

        if best_count < VOTE_MAJORITY or best_avg_conf < best_thresh:
            return None

        self.vote_history.clear()
        self.last_word_time = now
        self.cooldown_until = now + WORD_COOLDOWN
        self.last_word = best_label
        return best_label

    def reset(self):
        self.last_word = None
        self.vote_history.clear()
        self.cooldown_until = 0.0


# ============================================================
# ASR 语音识别 (Vosk, 本地麦克风)
# ============================================================

_asr = None
_asr_lock = threading.Lock()


def _init_asr():
    global _asr
    try:
        from speech_recognizer import SpeechRecognizer
        _asr = SpeechRecognizer(model_path=ASR_MODEL_PATH)
        if _asr.available:
            print(f'[ASR] 模型已加载')
            return True
        else:
            print('[ASR] 模型不可用')
            _asr = None
            return False
    except Exception as e:
        print(f'[ASR] 初始化失败: {e}')
        _asr = None
        return False


def _start_asr(on_text_cb):
    global _asr
    with _asr_lock:
        if _asr is None:
            if not _init_asr():
                return
        if not _asr._running:
            _asr.start(callback=on_text_cb)
            print('[ASR] 语音识别已启动')


def _stop_asr():
    global _asr
    with _asr_lock:
        if _asr is not None and _asr._running:
            _asr.stop()


# ============================================================
# 推理循环
# ============================================================

def resample_to_grid(timed_frames, grid_hz=100, num_frames=200):
    """将不规则时间戳的帧线性插值到均匀时间网格

    timed_frames: list of (timestamp_float, ndarray[features])
    返回: (num_frames, features) ndarray, 或 None (帧不足)
    """
    if len(timed_frames) < 2:
        return None
    timestamps = np.array([t for t, _ in timed_frames], dtype=np.float64)
    data = np.array([f for _, f in timed_frames], dtype=np.float64)
    # 网格终点 = 最新帧时间, 起点 = 终点 - (num_frames-1)/grid_hz
    t_end = timestamps[-1]
    t_start = t_end - (num_frames - 1) / float(grid_hz)
    grid = np.linspace(t_start, t_end, num_frames, dtype=np.float64)
    n_features = data.shape[1]
    result = np.zeros((num_frames, n_features), dtype=np.float32)
    for c in range(n_features):
        result[:, c] = np.interp(grid, timestamps, data[:, c]).astype(np.float32)
    return result


def do_inference(network, x, labels):
    """推理一次, 返回 (label, confidence, margin)"""
    network.set_input(0, x)
    network.run()
    out = np.array(network.get_output(index=0))
    # 数值稳定 softmax: 减去 max 防止 exp 溢出
    logits = out[0] - np.max(out[0])
    prob = np.exp(logits) / np.sum(np.exp(logits))
    top2_idx = np.argsort(out[0])[-2:]
    top1_conf = float(prob[top2_idx[1]])
    top2_conf = float(prob[top2_idx[0]])
    label = labels[top2_idx[1]]
    return label, top1_conf, top1_conf - top2_conf


def sensor_is_active(raw_frames):
    """传感器激活检测: 至少 MIN_ACTIVE_CHANNELS 个手指有明显弯曲

    F4 adc_value[i] = (raw_adc - 300) * 5, 放松时 ≈ 0.
    取200帧窗口每个通道的均值绝对值, > SENSOR_ACTIVE_THRESH 算激活.
    返回 (is_active: bool, info: str)
    """
    if len(raw_frames) < 50:
        return False, 'buf_fill'
    arr = np.array(raw_frames)
    bending = arr[:, :5]
    mean_abs = np.mean(np.abs(bending), axis=0)
    active_mask = mean_abs > SENSOR_ACTIVE_THRESH
    active_count = int(np.sum(active_mask))
    is_active = active_count >= MIN_ACTIVE_CHANNELS
    if not is_active:
        info = f'idle({active_count}/{MIN_ACTIVE_CHANNELS}) mean_abs={[f"{v:.0f}" for v in mean_abs]}'
    else:
        info = f'active({active_count})'
    return is_active, info


def main():
    print('=== 统一推理 (手势+拼音双模式) ===')
    print(f'特征: {NUM_FEATURES}维, {WINDOW_FRAMES}帧窗口')
    print()

    # 加载手势模型 (可选)
    g_net = None
    g_labels = None
    g_mean = g_std = g_clip_lo = g_clip_hi = g_base = None
    try:
        g_net = load_gesture_model()
        g_labels = load_labels(MODEL_DIR_GESTURE)
        g_mean, g_std, g_clip_lo, g_clip_hi, g_base = load_scaler(MODEL_DIR_GESTURE)
        print(f'手势类别: {len(g_labels)} 类')
    except FileNotFoundError as e:
        print(f'[警告] 手势模型未找到, 仅运行拼音模式 ({e})')

    # 加载拼音模型
    print('加载拼音模型...')
    p_net = load_pinyin_model()
    p_labels = load_labels(MODEL_DIR_PINYIN, [os.path.join(_SCRIPT_DIR, 'checkpoints')])
    p_mean, p_std, p_clip_lo, p_clip_hi, p_base = load_scaler(MODEL_DIR_PINYIN, [os.path.join(_SCRIPT_DIR, 'checkpoints')])
    print(f'拼音类别: {len(p_labels)} 类')
    if p_clip_lo is not None:
        clip_info = ', '.join(f'{p_clip_lo[i]:.0f}~{p_clip_hi[i]:.0f}' for i in range(5))
        print(f'[拼音] clip: [{clip_info}]')
    if p_base is not None:
        base_info = ', '.join(f'{p_base[i]:.0f}' for i in range(5))
        print(f'[拼音] ADC基线: [{base_info}] (已启用中心化)')

    print('模型加载完成\n')

    # 状态变量
    current_mode = None  # 'gesture' | 'pinyin'
    ring_buf = deque(maxlen=WINDOW_FRAMES * 3)  # 多存一些供重采样
    ring_buf_raw = deque(maxlen=WINDOW_FRAMES * 3)  # 原始值, 用于激活检测
    ring_ts = deque(maxlen=WINDOW_FRAMES * 3)       # 时间戳, 与上面两个同步
    word_tracker = WordTracker()
    pinyin_tracker = PinyinTracker(
        letter_cooldown=LETTER_COOLDOWN,
        vote_window=VOTE_WINDOW,
        vote_majority=VOTE_MAJORITY,
        min_margin=MIN_MARGIN,
        confidence_thresh=CONFIDENCE_THRESH)

    last_infer = 0.0
    last_pinyin_feed = 0.0  # 拼音模式: 最后一次喂入推理的时间 (手按住时刷新)
    pred_count = 0
    total_latency = 0.0
    start_time = time.time()
    last_act_print = 0.0  # 传感器idle打印节流

    # 串口
    ser = SerialIO()
    if not ser.open():
        print('[错误] 串口打开失败')
        return

    # MQTT
    mqtt = None
    if MQTT_ENABLE:
        try:
            mqtt = MQTTClient(device_id=DEVICE_ID)
            mqtt.connect(MQTT_BROKER, MQTT_PORT)
            print(f'[MQTT] 连接 {MQTT_BROKER}:{MQTT_PORT} (设备:{DEVICE_ID})')
            mqtt.publish_status({'version': 'unified_v1'})

            @mqtt.on_cmd('ping')
            def _ping(payload):
                mqtt._client.publish(
                    f'glove/{DEVICE_ID}/pong',
                    json.dumps({'rtt_ms': 0, 'uptime': int(time.time() - start_time)}),
                    qos=1)

            @mqtt.on_cmd('status')
            def _status(payload):
                fps = pred_count / max(time.time() - start_time, 1)
                avg_lat = total_latency / max(pred_count, 1)
                mqtt._client.publish(
                    f'glove/{DEVICE_ID}/pong',
                    json.dumps({
                        'fps': round(fps, 1),
                        'avg_latency_ms': round(avg_lat, 1),
                        'predictions': pred_count,
                        'uptime': int(time.time() - start_time),
                        'mode': current_mode,
                        'frame_ok': ser.frame_ok,
                        'frame_fail': ser.frame_fail,
                    }, ensure_ascii=False),
                    qos=1)

            @mqtt.on_cmd('set_threshold')
            def _set_threshold(payload):
                global CONFIDENCE_THRESH
                CONFIDENCE_THRESH = float(payload.get('value', CONFIDENCE_THRESH))
                _persist()

            @mqtt.on_cmd('set_word_threshold')
            def _set_word_threshold(payload):
                label = payload.get('label', '')
                v = payload.get('value')
                if label and v is not None:
                    word_tracker.word_thresholds[label] = float(v)
                    _persist()

            @mqtt.on_cmd('set_letter_threshold')
            def _set_letter_threshold(payload):
                label = payload.get('label', '')
                v = payload.get('value')
                if label and v is not None:
                    pinyin_tracker.letter_thresholds[label] = float(v)
                    _persist()

            @mqtt.on_cmd('reset_thresholds')
            def _reset_thresholds(payload):
                word_tracker.word_thresholds.clear()
                pinyin_tracker.letter_thresholds.clear()
                _persist()

            @mqtt.on_cmd('reset_pinyin')
            def _reset_pinyin(payload):
                pinyin_tracker.reset()

            @mqtt.on_cmd('force_ime')
            def _force_ime(payload):
                chinese, _, sentence = pinyin_tracker.force_flush()
                if chinese:
                    print(f'\n[IME] 手动触发: {chinese}')
                    ser.write_protocol_b(chinese, speak=True)
                    if mqtt and mqtt.connected:
                        mqtt.publish_sentence(sentence)
                    speak(chinese)
                    pinyin_tracker.reset_sentence()   # 播报后清空已发送句子, 防止叠加

            # 订阅 ASR, 收到后通过串口转发给 F4
            def _on_asr(client, userdata, msg):
                try:
                    payload = json.loads(msg.payload.decode())
                    text = payload.get('text', '')
                    if text:
                        print(f'\n[ASR→F4] {text}')
                        ser.write_protocol_a(text)
                except Exception:
                    pass

            mqtt._client.subscribe(f'glove/{DEVICE_ID}/asr', qos=1)
            mqtt._client.message_callback_add(
                f'glove/{DEVICE_ID}/asr', _on_asr)

            def _persist():
                data = {
                    'confidence_thresh': CONFIDENCE_THRESH,
                    'min_margin': MIN_MARGIN,
                    'letter_cooldown': LETTER_COOLDOWN,
                    'word_thresholds': word_tracker.word_thresholds,
                    'letter_thresholds': pinyin_tracker.letter_thresholds,
                }
                save_thresholds_local(data)

            # 恢复持久化阈值
            saved = load_thresholds()
            if 'word_thresholds' in saved:
                word_tracker.word_thresholds = dict(saved['word_thresholds'])
                print(f'[阈值] 恢复 word_thresholds: {len(word_tracker.word_thresholds)}条')
            if 'letter_thresholds' in saved:
                pinyin_tracker.letter_thresholds = dict(saved['letter_thresholds'])
                print(f'[阈值] 恢复 letter_thresholds: {len(pinyin_tracker.letter_thresholds)}条')

        except Exception as e:
            print(f'[MQTT] 连接失败: {e}')

    # ====== 主循环 ======
    print('\n等待手套数据...\n')

    def _pinyin_commit():
        """停顿/长按提交: 累积字母一次转换整串, 上屏(完整句) + GBK播报(新增段)"""
        chinese, syllables, sentence = pinyin_tracker.flush_to_ime()
        if not chinese:
            return
        print(f'\n[提交] {" ".join(syllables)} → {chinese}')
        ser.write_protocol_b(sentence if sentence else chinese, speak=True)
        if mqtt and mqtt.connected:
            mqtt.publish_sentence(sentence)
        speak(chinese)
        pinyin_tracker.reset_sentence()   # 播报后清空已发送句子, 防止叠加

    try:
        while True:
            mode, frame = ser.read_frame()

            if mode is None:
                time.sleep(0.001)
                continue

            # 模式切换: 清空状态
            if mode != current_mode:
                # 离开语音模式 → 停止ASR
                if current_mode == 'voice':
                    _stop_asr()
                current_mode = mode
                ring_buf.clear()
                ring_buf_raw.clear()
                ring_ts.clear()
                word_tracker.reset()
                pinyin_tracker.reset()
                last_infer = 0.0
                last_pinyin_feed = 0.0
                print(f'\n[模式] → {mode}')

                # 进入语音模式 → 立即启动ASR (不等数据帧)
                if mode == 'voice':
                    def _on_asr_text(text):
                        ser.write_protocol_a(text)
                        if mqtt and mqtt.connected:
                            mqtt._client.publish(
                                f'glove/{DEVICE_ID}/asr',
                                json.dumps({'text': text, 'ts': time.time()}, ensure_ascii=False),
                                qos=1)
                        print(f'\n[ASR] {text}')
                    _start_asr(_on_asr_text)

            # 仅模式字节, 无数据帧
            if frame is None:
                continue

            # 语音模式: 无需推理, 纯ASR监听
            if mode == 'voice':
                time.sleep(0.01)
                continue

            # 记录接收时间戳
            now_ts = time.time()

            # 转为数组；保留一份未标准化数据用于传感器激活检测
            arr = np.asarray(frame, dtype=np.float32)
            ring_buf_raw.append(arr.copy())

            # 标准化 (与训练一致: 先减ADC放松基线, 再clip噪声尖峰, 最后z-score)
            if mode == 'gesture':
                if g_mean is None:
                    continue  # 手势模型未加载, 跳过
                if g_base is not None:
                    arr = arr - g_base
                if g_clip_lo is not None:
                    arr = np.clip(arr, g_clip_lo, g_clip_hi)
                arr = (arr - g_mean) / (g_std + 1e-8)
            else:
                if p_base is not None:
                    arr = arr - p_base
                if p_clip_lo is not None:
                    arr = np.clip(arr, p_clip_lo, p_clip_hi)
                arr = (arr - p_mean) / (p_std + 1e-8)

            ring_buf.append((now_ts, arr))
            ring_ts.append(now_ts)

            if len(ring_buf) < WINDOW_FRAMES:
                continue

            now = time.time()
            if now - last_infer < INFER_INTERVAL:
                continue
            last_infer = now

            t0 = time.perf_counter()
            # 重采样到均匀100Hz网格, 补偿F4时序抖动
            x = resample_to_grid(list(ring_buf), grid_hz=100, num_frames=WINDOW_FRAMES)
            if x is None:
                continue
            x = x.reshape(1, WINDOW_FRAMES, NUM_FEATURES)

            if mode == 'gesture':
                if g_net is not None:
                    label, conf, margin = do_inference(g_net, x, g_labels)
                else:
                    continue  # 手势模型未加载, 跳过
                word = word_tracker.feed(label, conf, margin)

                elapsed = (time.perf_counter() - t0) * 1000
                pred_count += 1
                total_latency += elapsed

                status = f'[{elapsed:5.1f}ms] {label:6s} ({conf:.1%})'
                if word:
                    status += f'  确认: {word}'
                    label_idx = g_labels.index(word) if word in g_labels else 0
                    ser.send_gesture_result(label_idx, int(conf * 100))
                    ser.write_protocol_b(word)
                    if mqtt and mqtt.connected:
                        mqtt.publish_sentence(word)
                    speak(word)
                print(f'\r{status:<80}', end='', flush=True)

                if mqtt and mqtt.connected:
                    mqtt.publish_prediction(label, conf, elapsed, mode='gesture')

            else:  # pinyin
                # 停顿提交: 手停下(不再喂入推理) > PAUSE_COMMIT_S 才认为拼完一组
                # 计时基准 = 最后一次"手按住喂入推理"的时间 last_pinyin_feed:
                # 手按住期间推理持续进行, 计时不断刷新, 慢速拼字不会被误判为停顿;
                # 只有真正松手停下来, 计时才开始走
                # 防半字: 若末尾音节可续写(如 "niha" 可能补 o 成 "hao"), 延后到 FINAL 超时才提交
                if pinyin_tracker.letter_buffer and \
                        (now - last_pinyin_feed) > PAUSE_COMMIT_S:
                    letters_now = ''.join(pinyin_tracker.letter_buffer).lower()
                    if (now - last_pinyin_feed) > PAUSE_COMMIT_FINAL_S or \
                            not pinyin_tracker.ime.can_extend_last(letters_now):
                        _pinyin_commit()
                # 传感器激活检测: 手放松时直接跳过, 不推理
                active, act_info = sensor_is_active(list(ring_buf_raw))
                if not active:
                    if now - last_act_print > 2.0 and act_info != 'buf_fill':
                        print(f'\r[传感器] {act_info:<60}', end='', flush=True)
                        last_act_print = now
                    continue

                label, conf, margin = do_inference(p_net, x, p_labels)
                last_pinyin_feed = now
                triggered, letter = pinyin_tracker.feed(label, conf, margin)

                elapsed = (time.perf_counter() - t0) * 1000
                pred_count += 1
                total_latency += elapsed

                if triggered:
                    print('\n[长按触发] 立即提交')
                    _pinyin_commit()

                status = f'[{elapsed:5.1f}ms] {label:4s} ({conf:.1%})'
                if letter:
                    status += f' 字母:{letter}'
                    print(f'\r{status:<80}', end='', flush=True)
                    # 屏幕实时预览当前拼写的拼音字母 (不转换不发声; 转换交给停顿/长按提交)
                    preview = ''.join(pinyin_tracker.letter_buffer)
                    if preview:
                        ser.write_protocol_b(preview)

                if mqtt and mqtt.connected:
                    mqtt.publish_prediction(label, conf, elapsed, mode='pinyin')
                    pinyin_state = pinyin_tracker.get_pinyin_state()
                    mqtt.publish_pinyin_state(pinyin_state)

    except KeyboardInterrupt:
        pass
    finally:
        _stop_asr()
        ser.close()
        if mqtt:
            mqtt.stop()
        avg_lat = total_latency / max(pred_count, 1)
        print(f'\n[统计] 推理{pred_count}次, 平均{avg_lat:.1f}ms')
        print(f'[串口] 成功帧:{ser.frame_ok} 失败帧:{ser.frame_fail}')
        print('已退出')


if __name__ == '__main__':
    main()
