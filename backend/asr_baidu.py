"""百度短语音识别 (REST 直连, 不依赖 baidu-aip SDK)

百度 AI 开放平台的短语音识别:
    token : POST https://aip.baidubce.com/oauth/2.0/token
    识别  : POST https://vop.baidu.com/server_api  (JSON, speech 为 base64 PCM)

要求音频: PCM/WAV, 16kHz, 单声道, 时长 <= 60s。
本模块入参是 float32 [-1,1] 的 16k 单声道 numpy 数组, 内部转成 int16 小端 PCM。

凭证 (env):
    BAIDU_API_KEY     百度应用的 API Key      (必填)
    BAIDU_SECRET_KEY  百度应用的 Secret Key   (必填)
    BAIDU_DEV_PID     语言模型, 默认 1537 普通话(纯中文)
                      可选 1536 普通话(中英混合) / 1737 英语 / 1637 粤语

接口: 与 asr_whisper.SpeechRecognizer 无关, 只暴露 available 属性和
      transcribe(audio_f32) -> str, 由 asr_whisper 组合调用。
"""
import base64
import json
import os
import threading
import time

import numpy as np
import requests

TOKEN_URL = 'https://aip.baidubce.com/oauth/2.0/token'
ASR_URL = 'https://vop.baidu.com/server_api'
RETRY_ERRNOS = (3301, 3302, 3303, 3304)  # token 相关错误, 刷新后重试

_SECRETS_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'secrets.json')
_secrets_cache = None


def _load_secrets():
    """凭证优先取环境变量, 否则读同目录 secrets.json (已在 .gitignore, 不会入库)"""
    global _secrets_cache
    if _secrets_cache is not None:
        return _secrets_cache
    _secrets_cache = {}
    if os.path.isfile(_SECRETS_FILE):
        try:
            with open(_SECRETS_FILE, encoding='utf-8') as f:
                _secrets_cache = json.load(f)
        except Exception as e:
            print(f'[百度ASR] secrets.json 解析失败: {e}')
    return _secrets_cache


def _cred(name):
    return os.environ.get(name) or _load_secrets().get(name) or ''


class BaiduASR:
    def __init__(self, api_key=None, secret_key=None, dev_pid=None, cuid='glove_pc'):
        self.api_key = api_key or _cred('BAIDU_API_KEY')
        self.secret_key = secret_key or _cred('BAIDU_SECRET_KEY')
        self.dev_pid = int(dev_pid or _cred('BAIDU_DEV_PID') or 1537)
        self.cuid = cuid
        self.available = False
        self._token = None
        self._token_exp = 0.0
        self._lock = threading.Lock()
        self._session = requests.Session()
        self._init()

    def _init(self):
        if not self.api_key or not self.secret_key:
            print('[百度ASR] 未配置 BAIDU_API_KEY / BAIDU_SECRET_KEY, 跳过')
            return
        if self._fetch_token():
            self.available = True
            print(f'[百度ASR] 已就绪 (dev_pid={self.dev_pid})')

    def _fetch_token(self):
        try:
            r = self._session.post(TOKEN_URL, params={
                'grant_type': 'client_credentials',
                'client_id': self.api_key,
                'client_secret': self.secret_key,
            }, timeout=10)
            data = r.json()
            if 'access_token' in data:
                self._token = data['access_token']
                self._token_exp = time.time() + float(data.get('expires_in', 2592000)) - 600
                return True
            print('[百度ASR] 取 token 失败:', data.get('error_description') or data)
        except Exception as e:
            print('[百度ASR] 取 token 异常: %s' % e)
        return False

    def _ensure_token(self):
        if not self._token or time.time() >= self._token_exp:
            return self._fetch_token()
        return True

    def transcribe(self, audio_f32):
        """float32 [-1,1] 16k 单声道 -> 文字 (失败抛异常, 由调用方决定回退)"""
        if not self.available:
            raise RuntimeError('百度ASR 不可用')
        pcm = (np.clip(audio_f32, -1.0, 1.0) * 32767.0).astype('<i2').tobytes()

        last_err = None
        for attempt in (0, 1):
            with self._lock:
                if not self._ensure_token():
                    raise RuntimeError('百度ASR 获取 token 失败')
                payload = {
                    'format': 'pcm', 'rate': 16000, 'channel': 1, 'cuid': self.cuid,
                    'token': self._token, 'speech': base64.b64encode(pcm).decode(),
                    'len': len(pcm), 'dev_pid': self.dev_pid,
                }
            try:
                r = self._session.post(ASR_URL, json=payload,
                                       headers={'Content-Type': 'application/json'},
                                       timeout=20)
                data = r.json()
            except Exception as e:
                raise RuntimeError(f'百度ASR 请求异常: {e}')

            err_no = data.get('err_no')
            if err_no == 0:
                return ''.join(data.get('result') or []).strip()
            last_err = data
            if err_no in RETRY_ERRNOS and attempt == 0:
                with self._lock:
                    self._token = None
                continue
            break

        # 非 token 类错误 (如 3301 音频质量差) 也抛出, 让调用方决定是否回退本地
        raise RuntimeError(
            f"百度ASR 识别失败: err_no={last_err.get('err_no')} {last_err.get('err_msg')}")
