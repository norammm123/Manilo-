"""语音识别 (云优先 + 本地回退)

引擎:
    cloud  百度短语音识别 (asr_baidu, 需 BAIDU_API_KEY/SECRET_KEY)
    local  faster-whisper 本地 GPU/CPU (需先下载模型)
    auto   默认: 云优先, 云端不可用/报错时自动回退本地

接口 (与板端 speech_recognizer.SpeechRecognizer 对齐):
    available       属性, 是否有可用引擎
    start(callback) 启动后台监听, 每识别出一句调用 callback(text)
    stop()          停止
    _running        运行标志

音频源: PC 麦克风 (sounddevice), 16kHz 单声道 float32。
分段: 能量阈值静音检测, 说话停顿 0.8s 视为一句结束, 送识别。

依赖: sounddevice; 本地引擎额外需要 faster-whisper (首次需联网下模型)。
"""
import os
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
                 compute_type='float16', language='zh', engine=None):
        self.model_size = model_size
        self.device = device
        self.compute_type = compute_type
        self.language = language
        self.engine = (engine or os.environ.get('ASR_ENGINE', 'auto')).lower()
        self.available = False
        self._running = False
        self._thread = None
        self._callback = None
        self.last_engine = None
        # 云端 (百度)
        self._baidu = None
        if self.engine in ('auto', 'cloud'):
            try:
                from asr_baidu import BaiduASR
                self._baidu = BaiduASR()
            except Exception as e:
                print(f'[ASR] 百度引擎初始化失败: {e}')
        # 本地 (faster-whisper), 懒加载
        self._model = None
        self._local_tried = False

        cloud_ok = self._baidu is not None and self._baidu.available
        if self.engine == 'cloud':
            self.available = cloud_ok
        elif self.engine == 'local':
            self.available = self._load_local()
        else:  # auto: 云优先, 云端不可用则直接走本地
            if cloud_ok:
                self.available = True
            else:
                print('[ASR] 云端不可用, 尝试本地引擎')
                self.available = self._load_local()

    def _load_local(self):
        """懒加载 faster-whisper; 返回是否成功"""
        if self._model is not None:
            return True
        if self._local_tried:
            return self._model is not None
        self._local_tried = True
        try:
            from faster_whisper import WhisperModel
        except ImportError as e:
            print(f"[ASR] 未安装 faster-whisper: {e}")
            print("[ASR] 请执行: pip install faster-whisper")
            return False
        print(f"[ASR] 加载 faster-whisper '{self.model_size}' ({self.device})...")
        try:
            self._model = WhisperModel(self.model_size, device=self.device,
                                       compute_type=self.compute_type)
            print(f"[ASR] 本地已加载 ({self.device}/{self.compute_type})")
            return True
        except Exception as e:
            print(f"[ASR] {self.device} 加载失败: {e}, 回退 CPU(int8)")
            try:
                self._model = WhisperModel(self.model_size, device='cpu',
                                           compute_type='int8')
                self.device = 'cpu'
                self.compute_type = 'int8'
                print(f"[ASR] 本地已加载 (cpu/int8)")
                return True
            except Exception as e2:
                print(f"[ASR] 本地模型加载失败: {e2}")
                return False

    def start(self, callback=None):
        if not self.available:
            print("[ASR] 无可用识别引擎, 无法启动")
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
        # 1) 云端
        if self._baidu is not None and self._baidu.available:
            try:
                text = self._baidu.transcribe(audio)
                self.last_engine = 'cloud'
                return text
            except Exception as e:
                print(f'[ASR] 云端失败, 回退本地: {e}')
                if self.engine == 'cloud':
                    return ''
        # 2) 本地
        if not self._load_local():
            return ''
        segments, _ = self._model.transcribe(
            audio, language=self.language, beam_size=5,
            vad_filter=False, condition_on_previous_text=False)
        self.last_engine = 'local'
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
