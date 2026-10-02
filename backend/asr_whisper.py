"""faster-whisper 本地语音识别 (替代板端 Vosk speech_recognizer)

接口对齐板端 speech_recognizer.SpeechRecognizer:
    available       属性, 模型是否可用
    start(callback) 启动后台监听, 每识别出一句调用 callback(text)
    stop()          停止
    _running        运行标志

音频源: PC 麦克风 (sounddevice), 16kHz 单声道 float32。
分段: 能量阈值静音检测, 说话停顿 0.8s 视为一句结束, 送 whisper 转写。

依赖: pip install faster-whisper sounddevice
首次运行会自动下载模型 (需联网), 之后走本地缓存。
"""
import threading

import numpy as np

SAMPLE_RATE = 16000
BLOCK_FRAMES = 1600            # 0.1s @ 16kHz
SILENCE_RMS = 0.012            # 归一化幅度静音阈值 (按环境可调)
SPEECH_START_BLOCKS = 3        # 连续 0.3s 超阈值 → 说话开始
SILENCE_END_BLOCKS = 8         # 连续 0.8s 静音 → 一句结束
MIN_SPEECH_SEC = 0.4           # 过短段丢弃
MAX_SPEECH_SEC = 15.0          # 单段上限, 防止无限累积


class SpeechRecognizer:
    def __init__(self, model_size='small', device='cuda',
                 compute_type='float16', language='zh'):
        self.model_size = model_size
        self.device = device
        self.compute_type = compute_type
        self.language = language
        self.available = False
        self._running = False
        self._thread = None
        self._model = None
        self._callback = None
        self._load()

    def _load(self):
        try:
            from faster_whisper import WhisperModel
        except ImportError as e:
            print(f"[ASR] 未安装 faster-whisper: {e}")
            print("[ASR] 请执行: pip install faster-whisper")
            return
        try:
            self._model = WhisperModel(self.model_size, device=self.device,
                                       compute_type=self.compute_type)
            self.available = True
            print(f"[ASR] faster-whisper '{self.model_size}' 已加载 ({self.device}/{self.compute_type})")
        except Exception as e:
            print(f"[ASR] {self.device} 加载失败: {e}, 回退 CPU(int8)")
            try:
                self._model = WhisperModel(self.model_size, device='cpu',
                                           compute_type='int8')
                self.device = 'cpu'
                self.compute_type = 'int8'
                self.available = True
                print(f"[ASR] faster-whisper '{self.model_size}' 已加载 (cpu/int8)")
            except Exception as e2:
                print(f"[ASR] 模型加载失败: {e2}")

    def start(self, callback=None):
        if not self.available:
            print("[ASR] 模型不可用, 无法启动")
            return
        if self._running:
            return
        self._callback = callback
        self._running = True
        self._thread = threading.Thread(target=self._loop, daemon=True)
        self._thread.start()

    def stop(self):
        self._running = False
        t = self._thread
        if t is not None:
            t.join(timeout=3)
        self._thread = None

    def _transcribe(self, audio):
        segments, _ = self._model.transcribe(
            audio, language=self.language, beam_size=5,
            vad_filter=False, condition_on_previous_text=False)
        return "".join(seg.text for seg in segments).strip()

    def _loop(self):
        try:
            import sounddevice as sd
        except Exception as e:
            print(f"[ASR] sounddevice 不可用: {e}")
            self._running = False
            return
        print("[ASR] 麦克风监听中...")

        buf = []
        voiced = 0
        silent = 0
        in_speech = False

        def flush():
            nonlocal buf, in_speech, voiced, silent
            if buf:
                audio = np.concatenate(buf)
                dur = len(audio) / SAMPLE_RATE
                if dur >= MIN_SPEECH_SEC:
                    try:
                        text = self._transcribe(audio)
                    except Exception as e:
                        print(f"[ASR] 转写失败: {e}")
                        text = ""
                    if text and self._callback:
                        try:
                            self._callback(text)
                        except Exception as e:
                            print(f"[ASR] 回调失败: {e}")
            buf = []
            in_speech = False
            voiced = 0
            silent = 0

        try:
            with sd.InputStream(samplerate=SAMPLE_RATE, channels=1,
                                dtype='float32', blocksize=BLOCK_FRAMES) as stream:
                while self._running:
                    block, _ = stream.read(BLOCK_FRAMES)
                    mono = block[:, 0]
                    rms = float(np.sqrt(np.mean(mono * mono)))

                    if rms >= SILENCE_RMS:
                        voiced += 1
                        silent = 0
                    else:
                        silent += 1
                        voiced = 0

                    if not in_speech:
                        if voiced >= SPEECH_START_BLOCKS:
                            in_speech = True
                            buf = [mono.copy()]
                    else:
                        buf.append(mono.copy())
                        if silent >= SILENCE_END_BLOCKS:
                            flush()
                        elif len(buf) * BLOCK_FRAMES / SAMPLE_RATE >= MAX_SPEECH_SEC:
                            flush()
        except Exception as e:
            print(f"[ASR] 采集异常: {e}")
        finally:
            self._running = False
            print("[ASR] 已停止")
