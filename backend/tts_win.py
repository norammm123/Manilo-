"""Windows SAPI 中文语音播报 (替代板端 espeak/aplay)

用法:
    import tts_win
    tts_win.speak("你好")   # 异步, 立即返回

优先选用系统里的中文语音 (Huihui/Xiaoxiao 等), 找不到则用默认语音。
"""
import threading

_voice = None
_init_done = False
_available = False
_lock = threading.Lock()

# 常见中文语音描述关键字 (小写匹配)
_ZH_HINTS = ("huihui", "yaoyao", "xiaoxiao", "kangkang", "chinese", "zh-cn", "zh_cn", "中文", "汉语")


def _init():
    global _voice, _init_done, _available
    if _init_done:
        return _available
    _init_done = True
    try:
        import win32com.client
        voice = win32com.client.Dispatch("SAPI.SpVoice")
        tokens = voice.GetVoices()
        chosen = None
        for i in range(tokens.Count):
            tok = tokens.Item(i)
            try:
                desc = tok.GetDescription()
            except Exception:
                desc = ""
            if any(h in desc.lower() for h in _ZH_HINTS):
                chosen = tok
                break
        if chosen is not None:
            voice.Voice = chosen
            try:
                print(f"[TTS] 使用语音: {chosen.GetDescription()}")
            except Exception:
                print("[TTS] 已选用中文语音")
        else:
            print("[TTS] 未找到中文语音, 使用系统默认语音")
        _voice = voice
        _available = True
    except Exception as e:
        print(f"[TTS] SAPI 初始化失败: {e}")
        _available = False
    return _available


def speak(text):
    """异步播报文本, 不阻塞调用线程; SAPI 不可用时仅打印"""
    if not text:
        return
    with _lock:
        if not _init():
            print(f"[TTS] {text}")
            return
        try:
            _voice.Speak(str(text), 1)  # 1 = SVSFlagsAsync
        except Exception as e:
            print(f"[TTS] 播报失败: {e}")


if __name__ == "__main__":
    speak("语音播报测试")
    import time
    time.sleep(3)
