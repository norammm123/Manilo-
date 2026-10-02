"""MQTT Bridge - PC端 MQTT订阅 + Flask Web监控面板 + 远程控制"""
import json
import time
import os
import uuid
import queue
import threading
import urllib.request
import unicodedata
import paho.mqtt.client as mqtt
from flask import Flask, Response, request, jsonify, render_template

# === 配置 ===
BROKER = os.environ.get("MQTT_BROKER", "127.0.0.1")
PORT = int(os.environ.get("MQTT_PORT", "1883"))
DEVICE = os.environ.get("DEVICE_ID", "board1")
HTTP_HOST = "0.0.0.0"
HTTP_PORT = 5000
DATA_DIR = os.path.join(os.path.dirname(__file__), "mqtt_logs")

os.makedirs(DATA_DIR, exist_ok=True)

THRESHOLDS_FILE = os.path.join(DATA_DIR, "thresholds.json")
PREDICTIONS_FILE = os.path.join(DATA_DIR, "predictions.jsonl")
PINYIN_EVENTS_FILE = os.path.join(DATA_DIR, "pinyin_events.jsonl")
PINYIN_SENTENCES_FILE = os.path.join(DATA_DIR, "pinyin_sentences.jsonl")
ASR_EVENTS_FILE = os.path.join(DATA_DIR, "asr_events.jsonl")
LEARNING_EVENTS_FILE = os.path.join(DATA_DIR, "learning_sessions.jsonl")
AGENT_LOGS_FILE = os.path.join(DATA_DIR, "agent_logs.jsonl")
CONVERSATION_FILE = os.path.join(DATA_DIR, "conversation.jsonl")
FOLLOW_READ_FILE = os.path.join(DATA_DIR, "follow_read.jsonl")
REPORT_AGENT_URL = os.environ.get("LEARNING_REPORT_AGENT_URL", "").strip()
LLM_API_URL = os.environ.get("LLM_API_URL", "").strip()
LLM_API_KEY = os.environ.get("LLM_API_KEY", "").strip()
LLM_MODEL = os.environ.get("LLM_MODEL", "").strip()
REPORTS_FILE = os.path.join(DATA_DIR, "learning_reports.jsonl")
FRONTEND_B_ORIGINS = {
    "http://localhost:5174",
    "http://127.0.0.1:5174",
    os.environ.get("FRONTEND_B_ORIGIN", "").strip(),
} - {""}

# === 练习观察 Agent (后台线程持续分析练习日志) ===
# 只读取已落盘的 jsonl 日志, 不介入 MQTT 收包解析; 产出 agent_log(phase=观察)。
OBSERVER_ENABLED = os.environ.get("OBSERVER_AGENT", "1").strip().lower() not in ("0", "false", "off", "no")
OBSERVER_LLM = os.environ.get("OBSERVER_LLM", "1").strip().lower() not in ("0", "false", "off", "no")
OBSERVER_INTERVAL = float(os.environ.get("OBSERVER_INTERVAL", "30"))       # 扫描间隔(秒)
OBSERVER_LLM_INTERVAL = float(os.environ.get("OBSERVER_LLM_INTERVAL", "120"))  # 两次模型调用最短间隔(秒)
OBSERVER_MIN_DELTA = int(os.environ.get("OBSERVER_MIN_DELTA", "2"))        # 新增事件数达到多少才产出
OBSERVER_MAX_TOKENS = int(os.environ.get("OBSERVER_MAX_TOKENS", "6000"))   # 推理模型需留足 reasoning 预算
OBSERVER_PHASE = "观察"

# 运行时可调参数: 初值取自上面环境变量, B 端控制面板可在线修改(只影响本次进程, 不落盘)
_observer_config = {
    "enabled": OBSERVER_ENABLED,
    "llm_enabled": OBSERVER_LLM,
    "interval": OBSERVER_INTERVAL,
    "llm_interval": OBSERVER_LLM_INTERVAL,
    "min_delta": OBSERVER_MIN_DELTA,
}
_observer_lock = threading.Lock()


def load_thresholds():
    """加载持久化的阈值设置"""
    if os.path.exists(THRESHOLDS_FILE):
        try:
            with open(THRESHOLDS_FILE, "r", encoding="utf-8") as f:
                return json.load(f)
        except Exception:
            pass
    return {}


def save_thresholds(data):
    """持久化阈值设置"""
    with open(THRESHOLDS_FILE, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=2)


# === 全局状态 ===
saved_thresholds = load_thresholds()  # 启动时加载
app = Flask(__name__)
mqtt_client = None
sse_clients = []          # 所有 SSE 连接的队列
sentences = []            # 最近 200 条句子
predictions = []          # 最近 100 条预测
asr_messages = []         # 最近 100 条语音识别结果
pinyin_states = []        # 最近 100 条拼音状态
agent_logs = []           # 最近 200 条 Agent trace
conversation_messages = [] # 最近 500 条学习者/Agent 对话
follow_read_events = []   # 最近 500 条跟读转写结果
board_status = {"online": False, "version": "?", "last_seen": None}
stats = {"sentences_today": 0, "predictions_today": 0, "total_confidence": 0.0}
_lock = threading.Lock()


# ==== MQTT 回调 ====

def on_connect(client, userdata, flags, reason_code, properties=None):
    if reason_code == 0:
        print(f"[MQTT] 已连接 {BROKER}:{PORT}")
        client.subscribe(f"glove/{DEVICE}/#", qos=1)
    else:
        print(f"[MQTT] 连接失败 rc={reason_code}")


def on_message(client, userdata, msg):
    topic = msg.topic
    try:
        payload = json.loads(msg.payload.decode())
    except Exception:
        return

    with _lock:
        if topic.endswith("/sentence"):
            text = payload.get("text", "")
            record = {"text": text, "ts": payload.get("ts", time.time()),
                      "time": time.strftime("%H:%M:%S")}
            if payload.get("session_id"):
                record["session_id"] = str(payload["session_id"])
            else:
                _attach_active_session(record)
            sentences.append(record)
            if len(sentences) > 200:
                sentences.pop(0)
            stats["sentences_today"] += 1
            _save_sentence(record)
            _broadcast_sse({"type": "sentence", "data": record})

        elif topic.endswith("/prediction"):
            pred = {
                "label": payload.get("label", ""),
                "confidence": payload.get("confidence", 0),
                "latency_ms": payload.get("latency_ms", 0),
                "ts": payload.get("ts", time.time()),
                "time": time.strftime("%H:%M:%S"),
            }
            for key in ("correct", "session_id"):
                if key in payload:
                    pred[key] = payload[key]
            if not pred.get("session_id"):
                _attach_active_session(pred)
            predictions.append(pred)
            if len(predictions) > 100:
                predictions.pop(0)
            stats["predictions_today"] += 1
            stats["total_confidence"] += pred["confidence"]
            _append_jsonl(PREDICTIONS_FILE, pred)
            _broadcast_sse({"type": "prediction", "data": pred})

        elif topic.endswith("/status"):
            online = payload.get("online", False)
            board_status["online"] = online
            board_status["version"] = payload.get("version", "?")
            board_status["last_seen"] = time.strftime("%H:%M:%S") if online else None
            _broadcast_sse({"type": "status", "data": {
                "online": online, "version": board_status["version"],
                "last_seen": board_status["last_seen"],
            }})
            status_str = "在线" if online else "离线"
            print(f"[设备] {status_str} | 模型: {board_status['version']}")

        elif topic.endswith("/asr"):
            text = payload.get("text", "")
            record = {"text": text, "ts": payload.get("ts", time.time()),
                      "time": time.strftime("%H:%M:%S")}
            if payload.get("session_id"):
                record["session_id"] = str(payload["session_id"])
                session = next((item for item in _learning_sessions()
                                if item.get("session_id") == record["session_id"]), None)
            else:
                session = _attach_active_session(record)
            asr_messages.append(record)
            if len(asr_messages) > 100:
                asr_messages.pop(0)
            _append_jsonl(ASR_EVENTS_FILE, record)
            if session:
                expected_text = str(session.get("expected_text", "")).strip()
                transcription_ok = bool(str(text).strip())
                follow_read = {
                    **record,
                    "expected_text": expected_text,
                    "transcribed_text": str(text).strip(),
                    "transcription_ok": transcription_ok,
                }
                if expected_text:
                    follow_read["matches_expected"] = (
                        _normalize_learning_text(expected_text)
                        == _normalize_learning_text(str(text)))
                    follow_read["character_error_rate"] = _character_error_rate(expected_text, str(text))
                _record_follow_read(follow_read)
            _broadcast_sse({"type": "asr", "data": record})
            print(f"[ASR] {text}")

        elif topic.endswith("/pong"):
            _broadcast_sse({"type": "pong", "data": payload})

        elif topic.endswith("/pinyin"):
            pinyin_record = dict(payload)
            pinyin_record["ts"] = payload.get("ts", time.time())
            if not pinyin_record.get("session_id"):
                _attach_active_session(pinyin_record)
            pinyin_states.append(pinyin_record)
            if len(pinyin_states) > 100:
                pinyin_states.pop(0)
            _append_jsonl(PINYIN_EVENTS_FILE, pinyin_record)
            current_sentence = str(payload.get("current_sentence", "")).strip()
            previous_sentence = str(pinyin_states[-2].get("current_sentence", "")).strip() if len(pinyin_states) > 1 else ""
            if current_sentence and current_sentence != previous_sentence:
                _append_jsonl(PINYIN_SENTENCES_FILE, {
                    "text": current_sentence, "ts": pinyin_record["ts"],
                    "time": time.strftime("%H:%M:%S"),
                    "session_id": pinyin_record.get("session_id", ""),
                })
            _broadcast_sse({"type": "pinyin_state", "data": pinyin_record})
            letters = payload.get("letters", "")
            sentence = payload.get("current_sentence", "")
            if letters or sentence:
                print(f"[拼音] {letters} → {sentence}")

        elif topic.endswith("/agent/log"):
            _record_agent_log(payload)

        elif topic.endswith("/conversation"):
            _record_conversation(payload)
        elif topic.endswith("/learning/follow_read"):
            _record_follow_read(payload)


def on_disconnect(client, userdata, flags, reason_code, properties=None):
    if reason_code != 0:
        print(f"[MQTT] 断开 (rc={reason_code}), 自动重连中...")


def _save_sentence(record):
    _append_jsonl(os.path.join(DATA_DIR, "sentences.jsonl"), record)


def _append_jsonl(path, record):
    with open(path, "a", encoding="utf-8") as f:
        f.write(json.dumps(record, ensure_ascii=False) + "\n")


def _read_jsonl(path):
    records = []
    try:
        with open(path, "r", encoding="utf-8") as f:
            for line in f:
                try:
                    records.append(json.loads(line))
                except json.JSONDecodeError:
                    continue
    except FileNotFoundError:
        pass
    return records


def _learning_sessions():
    sessions = {}
    for event in _read_jsonl(LEARNING_EVENTS_FILE):
        if event.get("event") == "start":
            sessions[event["session_id"]] = event
        elif event.get("event") == "finish" and event.get("session_id") in sessions:
            sessions[event["session_id"]].update(event)
    return list(sessions.values())


def _attach_active_session(record):
    """将硬件日志关联到其发生时刻正在进行的学习会话。"""
    try:
        timestamp = float(record.get("ts", time.time()))
    except (TypeError, ValueError):
        timestamp = time.time()
    candidates = [session for session in _learning_sessions()
                  if float(session.get("started_at", 0)) <= timestamp
                  and (not session.get("ended_at") or timestamp <= float(session["ended_at"]))]
    if not candidates:
        return None
    session = max(candidates, key=lambda item: float(item.get("started_at", 0)))
    record["session_id"] = session["session_id"]
    return session


def _normalize_learning_text(value):
    """归一化文本，忽略空白和标点后用于目标语句比对。"""
    normalized = []
    for char in str(value or "").casefold():
        if char.isspace() or unicodedata.category(char).startswith("P"):
            continue
        normalized.append(char)
    return "".join(normalized)


def _character_error_rate(expected, actual):
    target = _normalize_learning_text(expected)
    result = _normalize_learning_text(actual)
    if not target:
        return None
    previous = list(range(len(result) + 1))
    for row, target_char in enumerate(target, start=1):
        current = [row]
        for col, result_char in enumerate(result, start=1):
            current.append(min(
                current[-1] + 1,
                previous[col] + 1,
                previous[col - 1] + (target_char != result_char),
            ))
        previous = current
    return round(previous[-1] / len(target), 4)


class LLMNotConfigured(Exception):
    pass


def _call_llm(messages, max_tokens=8000):
    """通过服务端环境变量调用 OpenAI-compatible Chat Completions 接口。

    注意: deepseek-flash 等推理模型会先用 max_tokens 生成 reasoning_content,
    预算给小了会出现 content 为空(finish_reason=length), 因此这里给足额度。
    实测本网关 reasoning 波动较大(3100~5200 token), 6000 在长回答时会截断,
    故默认给到 8000; 若换模型请重新实测 content_len 与 finish_reason。
    """
    if not (LLM_API_URL and LLM_API_KEY and LLM_MODEL):
        raise LLMNotConfigured("未配置 LLM_API_URL、LLM_API_KEY、LLM_MODEL")
    payload = {
        "model": LLM_MODEL,
        "messages": messages,
        "temperature": 0.4,
        "max_tokens": max_tokens,
    }
    req = urllib.request.Request(
        LLM_API_URL,
        data=json.dumps(payload, ensure_ascii=False).encode("utf-8"),
        headers={"Content-Type": "application/json", "Authorization": f"Bearer {LLM_API_KEY}"},
        method="POST",
    )
    with urllib.request.urlopen(req, timeout=90) as response:
        data = json.loads(response.read().decode("utf-8"))
    content = data["choices"][0]["message"]["content"]
    if isinstance(content, list):
        content = "\n".join(part.get("text", "") for part in content if isinstance(part, dict))
    if not isinstance(content, str) or not content.strip():
        raise ValueError("LLM 返回内容为空")
    return content.strip()


def _record_agent_log(payload):
    record = {
        "ts": payload.get("ts", time.time()),
        "time": payload.get("time", time.strftime("%H:%M:%S")),
        "phase": str(payload.get("phase", payload.get("type", "Agent")))[:40],
        "input": str(payload.get("input", payload.get("perception", "")))[:500],
        "decision": str(payload.get("decision", payload.get("action", "")))[:500],
        "detail": str(payload.get("detail", payload.get("message", "")))[:1000],
        "session_id": str(payload.get("session_id", ""))[:80],
    }
    _append_jsonl(AGENT_LOGS_FILE, record)
    agent_logs.append(record)
    if len(agent_logs) > 200:
        agent_logs.pop(0)
    _broadcast_sse({"type": "agent_log", "data": record})
    return record


agent_logs.extend(_read_jsonl(AGENT_LOGS_FILE)[-200:])
conversation_messages.extend(_read_jsonl(CONVERSATION_FILE)[-500:])
follow_read_events.extend(_read_jsonl(FOLLOW_READ_FILE)[-500:])
asr_messages.extend(_read_jsonl(ASR_EVENTS_FILE)[-100:])


def _record_conversation(payload):
    role = payload.get("role", "learner")
    if role == "patient":
        role = "learner"
    if role not in ("learner", "assistant"):
        role = "learner"
    record = {
        "ts": payload.get("ts", time.time()),
        "time": payload.get("time", time.strftime("%H:%M:%S")),
        "role": role,
        "text": str(payload.get("text", payload.get("content", "")))[:2000],
        "session_id": str(payload.get("session_id", ""))[:80],
    }
    if not record["text"].strip():
        return None
    _append_jsonl(CONVERSATION_FILE, record)
    conversation_messages.append(record)
    if len(conversation_messages) > 500:
        conversation_messages.pop(0)
    _broadcast_sse({"type": "conversation", "data": record})
    return record


def _record_follow_read(payload):
    """保存一次跟读转写结果；转写成功与目标文本吻合是两个独立信号。"""
    expected_text = str(payload.get("expected_text", ""))[:500].strip()
    transcribed_text = str(payload.get("transcribed_text", ""))[:1000].strip()
    transcription_ok = payload.get("transcription_ok")
    if not isinstance(transcription_ok, bool):
        return None
    record = {
        "ts": payload.get("ts", time.time()),
        "time": payload.get("time", time.strftime("%H:%M:%S")),
        "session_id": str(payload.get("session_id", ""))[:80],
        "expected_text": expected_text,
        "transcribed_text": transcribed_text,
        "transcription_ok": transcription_ok,
    }
    matches_expected = payload.get("matches_expected")
    if isinstance(matches_expected, bool):
        record["matches_expected"] = matches_expected
    try:
        error_rate = payload.get("character_error_rate")
        if error_rate is not None:
            record["character_error_rate"] = max(0.0, float(error_rate))
    except (TypeError, ValueError):
        pass
    _append_jsonl(FOLLOW_READ_FILE, record)
    follow_read_events.append(record)
    if len(follow_read_events) > 500:
        follow_read_events.pop(0)
    _broadcast_sse({"type": "follow_read", "data": record})
    return record


@app.after_request
def add_frontend_b_cors(response):
    origin = request.headers.get("Origin", "")
    if origin in FRONTEND_B_ORIGINS:
        response.headers["Access-Control-Allow-Origin"] = origin
        response.headers["Vary"] = "Origin"
        response.headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS"
        response.headers["Access-Control-Allow-Headers"] = "Content-Type"
    return response


def _day_start(timestamp=None):
    local = time.localtime(timestamp or time.time())
    return time.mktime((local.tm_year, local.tm_mon, local.tm_mday, 0, 0, 0,
                        local.tm_wday, local.tm_yday, local.tm_isdst))


def _event_in_session(event, session_id, start, ended_at):
    try:
        timestamp = float(event.get("ts", 0))
    except (TypeError, ValueError):
        return False
    if not start <= timestamp <= ended_at:
        return False
    return not event.get("session_id") or event.get("session_id") == session_id


def _session_metrics(session, ended_at):
    start = float(session["started_at"])
    session_id = session.get("session_id")
    in_session = lambda event: _event_in_session(event, session_id, start, ended_at)
    predictions_in_session = [event for event in _read_jsonl(PREDICTIONS_FILE) if in_session(event)]
    sentences_in_session = [event for event in _read_jsonl(os.path.join(DATA_DIR, "sentences.jsonl"))
                            if in_session(event)]
    pinyin_events = [event for event in _read_jsonl(PINYIN_EVENTS_FILE) if in_session(event)]
    pinyin_sentences = [event for event in _read_jsonl(PINYIN_SENTENCES_FILE) if in_session(event)]
    follow_read = [event for event in _read_jsonl(FOLLOW_READ_FILE) if in_session(event)]
    asr_events = [event for event in _read_jsonl(ASR_EVENTS_FILE) if in_session(event)]
    all_sentences = sentences_in_session + pinyin_sentences
    is_pinyin = session.get("mode") == "pinyin"
    activity_events = pinyin_events if is_pinyin else predictions_in_session
    if is_pinyin:
        labels = set()
        for event in activity_events:
            letter = str(event.get("last_letter", "")).strip()
            if not letter:
                sequence = str(event.get("pending_letters", event.get("letters", ""))).split()
                letter = sequence[-1] if sequence else ""
            if letter:
                labels.add(letter)
    else:
        labels = {str(p.get("label", "")).strip() for p in activity_events
                  if str(p.get("label", "")).strip()}
    confidences = []
    for event in activity_events:
        value = event.get("confidence", event.get("last_confidence"))
        try:
            if value is not None:
                confidence = float(value)
                confidences.append(confidence / 100 if confidence > 1 else confidence)
        except (TypeError, ValueError):
            continue
    avg_confidence = sum(confidences) / len(confidences) if confidences else None
    duration = max(0, int(ended_at - start))
    if not predictions_in_session and not pinyin_events and not follow_read:
        summary = "本次学习已记录时长。后台没有采集到识别事件；开始设备练习后，报告将补充练习次数和识别反馈。"
    else:
        confidence_text = f"{avg_confidence:.0%}" if avg_confidence is not None else "无置信度数据"
        follow_read_text = (f"跟读转写成功 {sum(1 for event in follow_read if event.get('transcription_ok') is True)}/"
                           f"{len(follow_read)} 次" if follow_read else "未采集跟读转写")
        summary = (f"本次练习 {duration // 60} 分 {duration % 60} 秒，完成 {len(all_sentences)} 句，"
                   f"记录 {len(predictions_in_session)} 次识别，覆盖 {len(labels)} 种手势，"
                   f"平均识别置信度 {confidence_text}；{follow_read_text}。这些是设备日志统计，不代表学习正确率。")
    if is_pinyin:
        follow_read_text = (f"跟读转写成功 {sum(1 for event in follow_read if event.get('transcription_ok') is True)}/"
                           f"{len(follow_read)} 次" if follow_read else "未采集跟读转写")
        summary = (f"本次拼音练习 {duration // 60} 分 {duration % 60} 秒，记录 {len(pinyin_events)} 条拼音状态，"
                   f"出现 {len(labels)} 种字母，完成 {len(pinyin_sentences)} 句。"
                   f"{follow_read_text}。平均模型置信度只反映设备识别信号，不代表学习正确率。")
    return {
        "duration_seconds": duration,
        "prediction_count": len(activity_events),
        "sentence_count": len(all_sentences),
        "distinct_labels": len(labels),
        "labels": sorted(labels),
        "avg_confidence": round(avg_confidence, 4) if avg_confidence is not None else None,
        "summary": summary,
        "pinyin_event_count": len(pinyin_events),
        "pinyin_sentence_count": len(pinyin_sentences),
        "follow_read_attempts": len(follow_read),
        "follow_read_successes": sum(1 for event in follow_read if event.get("transcription_ok") is True),
        "follow_read_success_rate": (sum(1 for event in follow_read if event.get("transcription_ok") is True) / len(follow_read)
                                     if follow_read else None),
        "follow_read_match_rate": (sum(1 for event in follow_read if event.get("matches_expected") is True)
                                   / sum(1 for event in follow_read if isinstance(event.get("matches_expected"), bool))
                                   if any(isinstance(event.get("matches_expected"), bool) for event in follow_read) else None),
        "follow_read_character_error_rate": (sum(float(event["character_error_rate"]) for event in follow_read
                                                   if isinstance(event.get("character_error_rate"), (int, float)))
                                              / sum(1 for event in follow_read
                                                    if isinstance(event.get("character_error_rate"), (int, float)))
                                              if any(isinstance(event.get("character_error_rate"), (int, float))
                                                     for event in follow_read) else None),
        "asr_attempts": len(asr_events),
        "asr_successes": sum(1 for event in asr_events if str(event.get("text", "")).strip()),
        "events": {"predictions": predictions_in_session, "sentences": all_sentences,
                   "pinyin_states": pinyin_events, "asr": asr_events,
                   "follow_read": follow_read},
    }


def _broadcast_sse(data):
    msg = f"data: {json.dumps(data, ensure_ascii=False)}\n\n"
    dead = []
    for q in sse_clients:
        try:
            q.put_nowait(msg)
        except queue.Full:
            dead.append(q)
    for q in dead:
        sse_clients.remove(q)


# ==== Flask 路由 ====

@app.route("/")
def index():
    return render_template("dashboard.html")


@app.route("/api/stream")
def api_stream():
    """SSE 实时推送"""
    q = queue.Queue(maxsize=200)
    with _lock:
        sse_clients.append(q)

    def generate():
        try:
            # 先发初始状态
            init_data = json.dumps({
                "type": "init",
                "status": board_status,
                "sentences": sentences[-20:],
                "predictions": predictions[-30:],
                "asr_messages": asr_messages[-20:],
                "pinyin_states": pinyin_states[-20:],
                "agent_logs": agent_logs[-50:],
                "conversation_messages": conversation_messages[-100:],
                "follow_read_events": follow_read_events[-100:],
                "thresholds": saved_thresholds,
            }, ensure_ascii=False)
            yield f"data: {init_data}\n\n"

            while True:
                try:
                    msg = q.get(timeout=15)
                    yield msg
                except queue.Empty:
                    yield ": keepalive\n\n"
        except GeneratorExit:
            with _lock:
                if q in sse_clients:
                    sse_clients.remove(q)

    return Response(generate(), mimetype="text/event-stream",
                    headers={"Cache-Control": "no-cache", "X-Accel-Buffering": "no"})


@app.route("/api/history")
def api_history():
    n = request.args.get("n", 50, type=int)
    with _lock:
        return jsonify(sentences[-n:])


@app.route("/api/stats")
def api_stats():
    with _lock:
        return jsonify({
            "online": board_status["online"],
            "version": board_status["version"],
            "last_seen": board_status["last_seen"],
            "sentences_today": stats["sentences_today"],
            "predictions_today": stats["predictions_today"],
            "avg_confidence": round(
                stats["total_confidence"] / max(stats["predictions_today"], 1), 3),
        })


@app.route("/api/learning/start", methods=["POST"])
def api_learning_start():
    body = request.get_json(silent=True) or {}
    mode = body.get("mode", "gesture")
    if mode not in ("gesture", "pinyin"):
        return jsonify({"error": "invalid learning mode"}), 400
    now = time.time()
    session = {
        "event": "start",
        "session_id": uuid.uuid4().hex,
        "started_at": now,
        "mode": mode,
        "expected_text": str(body.get("expected_text", ""))[:500].strip(),
    }
    _append_jsonl(LEARNING_EVENTS_FILE, session)
    return jsonify({
        "id": session["session_id"], "started_at": now, "mode": mode,
        "expected_text": session["expected_text"],
        "predictions": 0, "sentences": 0,
        "sessions_today": sum(1 for s in _learning_sessions()
                               if float(s.get("started_at", 0)) >= _day_start()),
    })


@app.route("/api/learning/session")
def api_learning_session():
    session_id = request.args.get("session_id", "")
    session = next((s for s in _learning_sessions()
                    if s.get("session_id") == session_id), None)
    if not session:
        return jsonify({"active": False}), 404
    if session.get("event") == "finish":
        return jsonify({"active": False})
    live_metrics = _session_metrics(session, time.time())
    return jsonify({
        "active": True, "id": session["session_id"],
        "started_at": session["started_at"], "mode": session.get("mode", "gesture"),
        "expected_text": session.get("expected_text", ""),
        "predictions": live_metrics["prediction_count"],
        "sentences": live_metrics["sentence_count"],
    })


@app.route("/api/learning/sessions")
def api_learning_sessions():
    limit = max(1, min(request.args.get("limit", 30, type=int), 100))
    sessions = sorted(_learning_sessions(), key=lambda item: float(item.get("started_at", 0)), reverse=True)
    return jsonify([{
        "id": session.get("session_id"),
        "started_at": session.get("started_at"),
        "ended_at": session.get("ended_at"),
        "mode": session.get("mode", "gesture"),
        "expected_text": session.get("expected_text", ""),
        "status": "finished" if session.get("event") == "finish" else "active",
    } for session in sessions[:limit]])


@app.route("/api/learning/stats")
def api_learning_stats():
    day_start = _day_start()
    sessions = [s for s in _learning_sessions()
                if float(s.get("started_at", 0)) >= day_start]
    predictions_today = [p for p in _read_jsonl(PREDICTIONS_FILE)
                         if float(p.get("ts", 0)) >= day_start]
    pinyin_events_today = [p for p in _read_jsonl(PINYIN_EVENTS_FILE)
                           if float(p.get("ts", 0)) >= day_start]
    sentences_today = [s for s in _read_jsonl(os.path.join(DATA_DIR, "sentences.jsonl"))
                       if float(s.get("ts", 0)) >= day_start]
    pinyin_sentences_today = [s for s in _read_jsonl(PINYIN_SENTENCES_FILE)
                              if float(s.get("ts", 0)) >= day_start]
    return jsonify({
        "sessions_today": len(sessions),
        "predictions_today": len(predictions_today) + len(pinyin_events_today),
        "sentences_today": len(sentences_today) + len(pinyin_sentences_today),
    })


@app.route("/api/learning/analytics")
def api_learning_analytics():
    now = time.time()
    today_start = _day_start(now)
    sessions = _learning_sessions()
    predictions_all = _read_jsonl(PREDICTIONS_FILE)
    pinyin_events_all = _read_jsonl(PINYIN_EVENTS_FILE)
    sentences_all = _read_jsonl(os.path.join(DATA_DIR, "sentences.jsonl"))
    pinyin_sentences_all = _read_jsonl(PINYIN_SENTENCES_FILE)
    daily = []
    for offset in reversed(range(7)):
        day = today_start - offset * 86400
        day_key = time.strftime("%Y-%m-%d", time.localtime(day))
        next_day = day + 86400
        day_sessions = [s for s in sessions
                        if day <= float(s.get("started_at", 0)) < next_day]
        day_predictions = [p for p in predictions_all
                           if day <= float(p.get("ts", 0)) < next_day]
        day_pinyin_events = [p for p in pinyin_events_all
                             if day <= float(p.get("ts", 0)) < next_day]
        daily.append({
            "date": day_key,
            "sessions": len(day_sessions),
            "recognitions": len(day_predictions) + len(day_pinyin_events),
            "sentences": sum(1 for s in sentences_all + pinyin_sentences_all
                             if day <= float(s.get("ts", 0)) < next_day),
        })

    vocabulary = {}
    checked = 0
    errors = 0
    for event in predictions_all:
        label = str(event.get("label", "")).strip()
        if not label:
            continue
        item = vocabulary.setdefault(label, {"label": label, "exposures": 0,
                                             "checked": 0, "correct": 0})
        item["exposures"] += 1
        if isinstance(event.get("correct"), bool):
            item["checked"] += 1
            checked += 1
            if event["correct"]:
                item["correct"] += 1
            else:
                errors += 1
    vocabulary_rows = sorted(vocabulary.values(),
                             key=lambda item: (-item["checked"], -item["exposures"], item["label"]))[:12]
    for item in vocabulary_rows:
        item["accuracy"] = (item["correct"] / item["checked"]
                            if item["checked"] else None)

    finished_today = [s for s in sessions
                      if s.get("ended_at") and today_start <= float(s["ended_at"]) <= now]
    durations = [max(0, float(s["ended_at"]) - float(s["started_at"]))
                 for s in finished_today]
    follow_read = [event for event in _read_jsonl(FOLLOW_READ_FILE)
                   if isinstance(event.get("transcription_ok"), bool)]
    follow_read_successes = sum(1 for event in follow_read if event["transcription_ok"])
    expected_checked = [event for event in follow_read
                        if isinstance(event.get("matches_expected"), bool)]
    expected_matches = sum(1 for event in expected_checked if event["matches_expected"])
    character_error_events = [event for event in follow_read
                              if isinstance(event.get("character_error_rate"), (int, float))]
    follow_read_by_phrase = {}
    for event in follow_read:
        phrase = str(event.get("expected_text", "")).strip() or "未指定目标语句"
        item = follow_read_by_phrase.setdefault(phrase, {"label": phrase, "attempts": 0, "successes": 0})
        item["attempts"] += 1
        item["successes"] += int(event["transcription_ok"])
    follow_read_rows = sorted(follow_read_by_phrase.values(),
                              key=lambda item: (-item["attempts"], item["label"]))[:12]
    for item in follow_read_rows:
        item["success_rate"] = item["successes"] / item["attempts"] if item["attempts"] else None
    return jsonify({
        "daily": daily,
        "vocabulary": vocabulary_rows,
        "vocabulary_count": len(vocabulary),
        "follow_read_attempts": len(follow_read),
        "follow_read_successes": follow_read_successes,
        "follow_read_success_rate": (follow_read_successes / len(follow_read) if follow_read else None),
        "follow_read_match_samples": len(expected_checked),
        "follow_read_match_rate": (expected_matches / len(expected_checked) if expected_checked else None),
        "follow_read_character_error_rate": (sum(float(event["character_error_rate"])
                                                  for event in character_error_events) / len(character_error_events)
                                              if character_error_events else None),
        "follow_read_by_phrase": follow_read_rows,
        "error_rate": errors / checked if checked else None,
        "error_samples": checked,
        "mastery_available": bool(checked),
        "sessions_today": sum(1 for s in sessions
                               if today_start <= float(s.get("started_at", 0)) <= now),
        "average_session_seconds": sum(durations) / len(durations) if durations else None,
    })


@app.route("/api/agent/log", methods=["POST"])
def api_agent_log():
    payload = request.get_json(silent=True)
    if not isinstance(payload, dict):
        return jsonify({"error": "expected a JSON object"}), 400
    with _lock:
        record = _record_agent_log(payload)
    return jsonify({"status": "recorded", "data": record}), 201


@app.route("/api/agent/logs")
def api_agent_logs():
    return jsonify(agent_logs[-100:])


@app.route("/api/conversation", methods=["POST"])
def api_conversation():
    payload = request.get_json(silent=True)
    if not isinstance(payload, dict):
        return jsonify({"error": "expected a JSON object"}), 400
    with _lock:
        record = _record_conversation(payload)
    if record is None:
        return jsonify({"error": "text is required"}), 400
    return jsonify({"status": "recorded", "data": record}), 201


@app.route("/api/learning/follow-read", methods=["POST"])
def api_learning_follow_read():
    payload = request.get_json(silent=True)
    if not isinstance(payload, dict):
        return jsonify({"error": "expected a JSON object"}), 400
    with _lock:
        record = _record_follow_read(payload)
    if record is None:
        return jsonify({"error": "transcription_ok must be a boolean"}), 400
    return jsonify({"status": "recorded", "data": record}), 201


def _session_report_payload(session, ended_at, metrics):
    session_id = session.get("session_id")
    start = float(session.get("started_at", 0))
    within = lambda record: _event_in_session(record, session_id, start, ended_at)
    return {
        "task": "根据学习日志生成简明、鼓励性且不过度推断的学习报告。区分设备识别置信度、语音转写成功与目标文本吻合；不要仅凭转写成功断言完整掌握度。日志没有的数据请明确说明。",
        "session": {"id": session_id, "mode": session.get("mode"),
                    "started_at": start, "ended_at": ended_at,
                    "expected_text": session.get("expected_text", "")},
        "metrics": {key: value for key, value in metrics.items() if key != "events"},
        "events": metrics["events"],
        "agent_logs": [record for record in _read_jsonl(AGENT_LOGS_FILE) if within(record)],
        "conversation_messages": [record for record in _read_jsonl(CONVERSATION_FILE) if within(record)],
    }


def _generate_report(session, ended_at, metrics):
    payload = _session_report_payload(session, ended_at, metrics)
    agent_report = ""
    agent_error = ""
    if REPORT_AGENT_URL:
        try:
            req = urllib.request.Request(
                REPORT_AGENT_URL,
                data=json.dumps(payload, ensure_ascii=False).encode("utf-8"),
                headers={"Content-Type": "application/json"},
                method="POST",
            )
            with urllib.request.urlopen(req, timeout=30) as response:
                agent_data = json.loads(response.read().decode("utf-8"))
            agent_report = agent_data.get("report", agent_data.get("text", agent_data.get("message", "")))
            if isinstance(agent_report, (dict, list)):
                agent_report = json.dumps(agent_report, ensure_ascii=False, indent=2)
            agent_report = str(agent_report).strip()
        except Exception as exc:
            agent_error = str(exc)
    elif LLM_API_URL and LLM_API_KEY and LLM_MODEL:
        try:
            agent_report = _call_llm([
                {"role": "system", "content": payload["task"]},
                {"role": "user", "content": json.dumps(payload, ensure_ascii=False)},
            ], max_tokens=9000)
        except Exception as exc:
            agent_error = str(exc)
    return {
        "session_id": session.get("session_id"),
        "started_at": float(session.get("started_at", 0)),
        "ended_at": ended_at,
        "mode": session.get("mode", "gesture"),
        "metrics": {key: value for key, value in metrics.items() if key != "events"},
        "summary": agent_report or metrics.get("summary", "学习日志已保存。"),
        "agent_used": bool(agent_report),
        "agent_error": agent_error,
        "created_at": time.time(),
    }


def _save_report(record):
    _append_jsonl(REPORTS_FILE, record)


@app.route("/api/learning/finish", methods=["POST"])
def api_learning_finish():
    body = request.get_json(silent=True) or {}
    session_id = str(body.get("session_id", ""))
    session = next((s for s in _learning_sessions()
                    if s.get("session_id") == session_id), None)
    if not session:
        return jsonify({"error": "learning session not found"}), 404
    if session.get("event") == "finish":
        return jsonify({"error": "learning session already finished"}), 409
    ended_at = time.time()
    metrics = _session_metrics(session, ended_at)
    finish_event = {
        "event": "finish", "session_id": session_id,
        "ended_at": ended_at, "report": {k: v for k, v in metrics.items() if k != "events"},
    }
    _append_jsonl(LEARNING_EVENTS_FILE, finish_event)
    report_record = _generate_report(session, ended_at, metrics)
    _save_report(report_record)
    _broadcast_sse({"type": "learning_finished", "data": report_record})
    result = {
        "session_id": session_id, "started_at": session["started_at"],
        "ended_at": ended_at, "mode": session.get("mode", "gesture"),
        "report": {k: v for k, v in metrics.items() if k not in ("summary", "events")},
        "agent_used": report_record["agent_used"],
        "agent_report": report_record["summary"] if report_record["agent_used"] else "",
        "agent_error": report_record["agent_error"] or None,
        "summary": report_record["summary"],
        "sessions_today": sum(1 for s in _learning_sessions()
                               if float(s.get("started_at", 0)) >= _day_start()),
    }
    return jsonify(result)


@app.route("/api/learning/reports")
def api_learning_reports():
    records = _read_jsonl(REPORTS_FILE)
    return jsonify(list(reversed(records[-50:])))


@app.route("/api/learning/report", methods=["POST"])
def api_learning_report():
    body = request.get_json(silent=True) or {}
    session_id = str(body.get("session_id", ""))
    session = next((item for item in _learning_sessions()
                    if item.get("session_id") == session_id), None)
    if not session:
        return jsonify({"error": "learning session not found"}), 404
    if session.get("event") != "finish":
        return jsonify({"error": "finish the learning session before generating a report"}), 409
    ended_at = float(session.get("ended_at", time.time()))
    metrics = _session_metrics(session, ended_at)
    report_record = _generate_report(session, ended_at, metrics)
    _save_report(report_record)
    _broadcast_sse({"type": "learning_report", "data": report_record})
    return jsonify(report_record)


@app.route("/api/assistant/status")
def api_assistant_status():
    chat_configured = bool(LLM_API_URL and LLM_API_KEY and LLM_MODEL)
    report_configured = chat_configured or bool(REPORT_AGENT_URL)
    return jsonify({"configured": chat_configured,
                    "chat_configured": chat_configured,
                    "report_configured": report_configured,
                    "report_agent_configured": bool(REPORT_AGENT_URL),
                    "model": LLM_MODEL if chat_configured else ""})


@app.route("/api/assistant/chat", methods=["POST"])
def api_assistant_chat():
    body = request.get_json(silent=True) or {}
    text = str(body.get("text", "")).strip()[:2000]
    session_id = str(body.get("session_id", ""))[:80]
    if not text:
        return jsonify({"error": "text is required"}), 400
    if not (LLM_API_URL and LLM_API_KEY and LLM_MODEL):
        return jsonify({"error": "AI 助手未配置模型服务"}), 503
    with _lock:
        learner_message = _record_conversation({
            "role": "learner", "text": text, "session_id": session_id,
        })
    session = next((item for item in _learning_sessions()
                    if item.get("session_id") == session_id), None) if session_id else None
    context = {}
    history = []
    if session:
        end = float(session.get("ended_at", time.time()))
        metrics = _session_metrics(session, end)
        context = {key: value for key, value in metrics.items() if key != "events"}
        history = [record for record in _read_jsonl(CONVERSATION_FILE)
                   if record.get("session_id") == session_id][-12:]
    system = (
        "你是手语/拼音学习陪练助手。根据提供的客观学习日志给出简短、耐心、可执行的建议。"
        "区分设备识别置信度、语音转写成功与目标文本吻合；没有数据就明确说明。"
        "不要臆造情绪、诊断、掌握度或设备状态，也不要把单次识别成功说成已经掌握。"
    )
    messages = [{"role": "system", "content": system + "\n本次学习统计：" + json.dumps(context, ensure_ascii=False)}]
    for record in history:
        if (record.get("role") == "learner"
                and record.get("ts") == learner_message.get("ts")):
            continue
        role = "assistant" if record.get("role") == "assistant" else "user"
        messages.append({"role": role, "content": record.get("text", "")})
    messages.append({"role": "user", "content": text})
    try:
        reply = _call_llm(messages, max_tokens=8000)
    except Exception as exc:
        return jsonify({"error": f"AI 服务调用失败：{exc}", "learner_message": learner_message}), 502
    with _lock:
        assistant_message = _record_conversation({
            "role": "assistant", "text": reply, "session_id": session_id,
        })
    return jsonify({"reply": reply, "learner_message": learner_message,
                    "assistant_message": assistant_message})


@app.route("/api/cmd", methods=["POST"])
def api_cmd():
    """发送远程控制命令到板子"""
    body = request.get_json(force=True)
    cmd = body.get("cmd", "")
    if not cmd:
        return jsonify({"error": "missing cmd"}), 400

    # 持久化阈值类命令
    _persist_cmd(body)

    # 展平: 把 params 提升到顶层, 板子端处理器从顶层读取
    mqtt_payload = {"cmd": cmd}
    params = body.get("params", {})
    if isinstance(params, dict):
        mqtt_payload.update(params)

    if mqtt_client and mqtt_client.is_connected():
        topic = f"glove/{DEVICE}/cmd"
        mqtt_client.publish(topic, json.dumps(mqtt_payload, ensure_ascii=False), qos=1)
        print(f"[CMD] → 板子: {cmd} {params}")
        return jsonify({"status": "sent", "cmd": cmd})
    else:
        # MQTT断连时也保存, 返回已保存状态
        return jsonify({"status": "saved", "cmd": cmd, "warning": "MQTT not connected, saved locally"})


def _persist_cmd(body):
    """持久化阈值设置命令"""
    cmd = body.get("cmd", "")
    params = body.get("params", body)
    global saved_thresholds

    if cmd == "set_threshold":
        v = params.get("value")
        if v is not None:
            saved_thresholds["confidence_thresh"] = float(v)
            save_thresholds(saved_thresholds)

    elif cmd == "set_cooldown":
        v = params.get("value")
        if v is not None:
            saved_thresholds["word_cooldown"] = float(v)
            save_thresholds(saved_thresholds)

    elif cmd == "set_word_threshold":
        label = params.get("label", "")
        v = params.get("value")
        if label and v is not None:
            saved_thresholds.setdefault("word_thresholds", {})[label] = float(v)
            save_thresholds(saved_thresholds)

    elif cmd == "set_letter_threshold":
        label = params.get("label", "")
        v = params.get("value")
        if label and v is not None:
            saved_thresholds.setdefault("letter_thresholds", {})[label] = float(v)
            save_thresholds(saved_thresholds)

    elif cmd == "reset_letter_thresholds":
        saved_thresholds.pop("letter_thresholds", None)
        save_thresholds(saved_thresholds)

    elif cmd == "reset_thresholds":
        saved_thresholds.pop("word_thresholds", None)
        save_thresholds(saved_thresholds)

    elif cmd == "set_margin":
        v = params.get("value")
        if v is not None:
            saved_thresholds["min_margin"] = float(v)
            save_thresholds(saved_thresholds)

    elif cmd == "set_vote_window":
        v = params.get("value")
        if v is not None:
            saved_thresholds["vote_window"] = int(v)
            save_thresholds(saved_thresholds)

    elif cmd == "set_vote_majority":
        v = params.get("value")
        if v is not None:
            saved_thresholds["vote_majority"] = int(v)
            save_thresholds(saved_thresholds)

    elif cmd == "set_silence_timeout":
        v = params.get("value")
        if v is not None:
            saved_thresholds["silence_timeout"] = float(v)
            save_thresholds(saved_thresholds)


@app.route("/api/thresholds")
def api_thresholds():
    """获取已保存的阈值设置"""
    return jsonify(saved_thresholds)


# ==== 练习观察 Agent (后台持续分析) ====

_observer_state = {
    "scans": 0,
    "llm_calls": 0,
    "llm_errors": 0,
    "last_count": 0,
    "last_llm_ts": 0.0,
    "last_emit_ts": 0.0,
    "last_insight": None,
    "window_key": None,
}


def _observer_window():
    """确定分析窗口: 优先"进行中的学习会话", 否则"今天最新的会话", 再否则"今日全部日志"。

    只取今天开始的数据, 避免把昨天的旧记录当成实时进度重复播报。
    """
    now = time.time()
    day = _day_start(now)
    sessions = [s for s in _learning_sessions() if float(s.get("started_at", 0)) >= day]
    sessions.sort(key=lambda item: float(item.get("started_at", 0)))
    active = [s for s in sessions if s.get("event") != "finish"]
    if active:
        session = active[-1]
        return ("练习中", float(session["started_at"]), now,
                session.get("session_id"), session.get("mode", "gesture"))
    if sessions:
        session = sessions[-1]
        return ("本次练习", float(session["started_at"]), float(session.get("ended_at", now)),
                session.get("session_id"), session.get("mode", "gesture"))
    return ("今日", day, now, None, "gesture")


def _observer_collect(start, end, session_id=None):
    """读取窗口内的练习日志, 汇总成客观统计。只读文件, 不触碰内存缓冲与 MQTT 回调。"""
    def within(record):
        try:
            timestamp = float(record.get("ts", 0))
        except (TypeError, ValueError):
            return False
        if not start <= timestamp <= end:
            return False
        return not session_id or not record.get("session_id") or record.get("session_id") == session_id

    predictions = [r for r in _read_jsonl(PREDICTIONS_FILE) if within(r)]
    sentences = [r for r in _read_jsonl(os.path.join(DATA_DIR, "sentences.jsonl")) if within(r)]
    pinyin_sentences = [r for r in _read_jsonl(PINYIN_SENTENCES_FILE) if within(r)]
    follow_read = [r for r in _read_jsonl(FOLLOW_READ_FILE) if within(r)]
    asr_events = [r for r in _read_jsonl(ASR_EVENTS_FILE) if within(r)]

    labels = [str(p.get("label", "")).strip() for p in predictions if str(p.get("label", "")).strip()]
    confidences = []
    for prediction in predictions:
        try:
            value = float(prediction.get("confidence"))
        except (TypeError, ValueError):
            continue
        confidences.append(value / 100 if value > 1 else value)

    ok_flags = [e.get("transcription_ok") for e in follow_read if isinstance(e.get("transcription_ok"), bool)]
    match_flags = [e.get("matches_expected") for e in follow_read if isinstance(e.get("matches_expected"), bool)]
    cer_values = [float(e["character_error_rate"]) for e in follow_read
                  if isinstance(e.get("character_error_rate"), (int, float))]

    all_sentences = sentences + pinyin_sentences
    return {
        "prediction_count": len(predictions),
        "sentence_count": len(all_sentences),
        "follow_read_attempts": len(follow_read),
        "asr_count": len(asr_events),
        "total": len(predictions) + len(all_sentences) + len(follow_read) + len(asr_events),
        "labels": labels,
        "distinct_labels": sorted(set(labels)),
        "avg_confidence": (sum(confidences) / len(confidences)) if confidences else None,
        "follow_read_success_rate": (sum(1 for f in ok_flags if f) / len(ok_flags)) if ok_flags else None,
        "follow_read_match_rate": (sum(1 for f in match_flags if f) / len(match_flags)) if match_flags else None,
        "follow_read_cer": (sum(cer_values) / len(cer_values)) if cer_values else None,
        "last_label": labels[-1] if labels else "",
        "last_sentence": all_sentences[-1].get("text", "") if all_sentences else "",
    }


def _observer_facts(snapshot):
    parts = []
    if snapshot["prediction_count"]:
        parts.append(f"识别 {snapshot['prediction_count']} 次")
    if snapshot["sentence_count"]:
        parts.append(f"成句 {snapshot['sentence_count']} 句")
    parts.append(f"覆盖 {len(snapshot['distinct_labels'])} 种标签")
    if snapshot["avg_confidence"] is not None:
        parts.append(f"平均置信度 {snapshot['avg_confidence']:.0%}")
    if snapshot["follow_read_attempts"]:
        rate = snapshot["follow_read_success_rate"]
        text = f"跟读转写 {snapshot['follow_read_attempts']} 次"
        parts.append(text + (f"(成功 {rate:.0%})" if rate is not None else ""))
    return "、".join(parts)


def _observer_local_advice(snapshot):
    """无模型或模型不可用时的本地规则建议, 保证面板始终有内容。"""
    tips = []
    if not snapshot["prediction_count"]:
        tips.append("尚未采集到识别事件，确认板子已连接并开始练习")
    if snapshot["avg_confidence"] is not None and snapshot["avg_confidence"] < 0.7:
        tips.append("平均识别置信度偏低，放慢出手速度并让手势多停留一下")
    if snapshot["follow_read_match_rate"] is not None and snapshot["follow_read_match_rate"] < 0.8:
        tips.append("跟读与目标文本吻合率不足，先逐词跟读再连成句子")
    if snapshot["follow_read_cer"] is not None and snapshot["follow_read_cer"] > 0.2:
        tips.append("转写字符错误率偏高，挑最常错的词单独纠音")
    if len(snapshot["distinct_labels"]) < 5 and snapshot["prediction_count"] >= 10:
        tips.append("识别标签集中在少数词，建议扩展新词汇")
    if not tips:
        tips.append("当前节奏稳定，继续保持并逐步把练习从单词过渡到短句")
    return "；".join(tips[:2])


def _observer_llm_advice(label, snapshot):
    facts = {
        "窗口": label,
        "识别次数": snapshot["prediction_count"],
        "成句数": snapshot["sentence_count"],
        "跟读次数": snapshot["follow_read_attempts"],
        "覆盖标签": snapshot["distinct_labels"][-20:],
        "平均置信度": snapshot["avg_confidence"],
        "跟读转写成功率": snapshot["follow_read_success_rate"],
        "目标文本吻合率": snapshot["follow_read_match_rate"],
        "字符错误率": snapshot["follow_read_cer"],
        "最近识别": snapshot["last_label"],
        "最近句子": snapshot["last_sentence"],
    }
    system = (
        "你是手语/拼音学习观察助手。根据实时练习统计，给学习者一句即时提醒，"
        "不超过 60 字，具体、可执行、鼓励性。只依据给出的数据，"
        "不要臆造掌握度、情绪或设备状态。"
    )
    return _call_llm([
        {"role": "system", "content": system},
        {"role": "user", "content": json.dumps(facts, ensure_ascii=False)},
    ], max_tokens=OBSERVER_MAX_TOKENS)


def _observer_tick(force=False):
    """扫描一次。force=True 时忽略"新增量"门槛(供 B 端手动扫描/演示)。

    用 _observer_lock 串行化, 避免后台线程与手动请求同时扫描(会重复调模型)。
    """
    with _observer_lock:
        label, start, end, session_id, _mode = _observer_window()
        snapshot = _observer_collect(start, end, session_id)
        state = _observer_state
        state["scans"] += 1
        if snapshot["total"] == 0:
            return None

        window_key = (session_id, label, round(start, 1))
        if state["window_key"] != window_key:
            state["window_key"] = window_key
            state["last_count"] = 0
        if not force and snapshot["total"] - state["last_count"] < _observer_config["min_delta"]:
            return None

        facts = _observer_facts(snapshot)
        decision = _observer_local_advice(snapshot)
        source = "本地"
        configured = bool(_observer_config["llm_enabled"] and LLM_API_URL and LLM_API_KEY and LLM_MODEL)
        # 注意: last_llm_ts 初值 0, 因此进程内首次产出一定会调一次模型, 之后才按间隔限流。
        # 想彻底不调用模型(零 token 成本)可在 B 端关掉"模型建议", 或设 OBSERVER_LLM=0。
        if configured and time.time() - state["last_llm_ts"] >= _observer_config["llm_interval"]:
            try:
                decision = _observer_llm_advice(label, snapshot)
                source = "模型"
                state["llm_calls"] += 1
                state["last_llm_ts"] = time.time()
            except Exception as exc:
                state["llm_errors"] += 1
                print(f"[观察Agent] 模型调用失败, 退回本地建议: {exc}")

        record = {
            "phase": OBSERVER_PHASE,
            "input": f"{label}：{facts}",
            "decision": decision,
            "detail": f"{label}窗口 · {facts}"
                      + (f" · 最近句子「{snapshot['last_sentence']}」" if snapshot["last_sentence"] else ""),
            "session_id": session_id or "",
            "ts": time.time(),
        }
        with _lock:
            stored = _record_agent_log(record)
        state["last_count"] = snapshot["total"]
        state["last_emit_ts"] = time.time()
        state["last_insight"] = stored
        print(f"[观察Agent] ({source}) {decision}")
        return stored


def _observer_loop():
    has_model = bool(_observer_config["llm_enabled"] and LLM_API_URL and LLM_API_KEY and LLM_MODEL)
    print(f"[观察Agent] 已启动: 每 {_observer_config['interval']:g}s 扫描一次, "
          f"模型建议至少间隔 {_observer_config['llm_interval']:g}s"
          + ("" if has_model else " (仅本地分析)"))
    while True:
        time.sleep(max(5.0, float(_observer_config["interval"])))
        if not _observer_config["enabled"]:
            continue  # 线程常驻, 等 B 端重新开启
        try:
            _observer_tick()
        except Exception as exc:
            print(f"[观察Agent] 扫描异常, 已跳过: {exc}")


def start_observer_agent():
    """线程始终启动, 是否产出由 _observer_config['enabled'] 决定(B 端可在线开关)。"""
    if not _observer_config["enabled"]:
        print("[观察Agent] 已启动但当前为关闭状态 (可在 B 端面板开启, 或设 OBSERVER_AGENT=1)")
    thread = threading.Thread(target=_observer_loop, name="practice-observer", daemon=True)
    thread.start()
    return thread


def _observer_status():
    return {
        "enabled": _observer_config["enabled"],
        "phase": OBSERVER_PHASE,
        "interval_seconds": _observer_config["interval"],
        "llm_interval_seconds": _observer_config["llm_interval"],
        "min_delta": _observer_config["min_delta"],
        "llm_enabled": _observer_config["llm_enabled"],
        "llm_configured": bool(LLM_API_URL and LLM_API_KEY and LLM_MODEL),
        "scans": _observer_state["scans"],
        "llm_calls": _observer_state["llm_calls"],
        "llm_errors": _observer_state["llm_errors"],
        "last_emit_ts": _observer_state["last_emit_ts"] or None,
        "last_insight": _observer_state["last_insight"],
    }


@app.route("/api/agent/observer", methods=["GET", "POST"])
def api_agent_observer():
    """观察 Agent 状态(GET) / 在线调整参数(POST), 供 B 端控制面板。"""
    if request.method == "POST":
        payload = request.get_json(silent=True) or {}
        if "enabled" in payload:
            _observer_config["enabled"] = bool(payload["enabled"])
        if "llm_enabled" in payload:
            _observer_config["llm_enabled"] = bool(payload["llm_enabled"])
        if "interval_seconds" in payload:
            try:
                _observer_config["interval"] = max(5.0, min(600.0, float(payload["interval_seconds"])))
            except (TypeError, ValueError):
                pass
        if "min_delta" in payload:
            try:
                _observer_config["min_delta"] = max(1, min(100, int(payload["min_delta"])))
            except (TypeError, ValueError):
                pass
    return jsonify(_observer_status())


@app.route("/api/agent/observer/tick", methods=["POST"])
def api_agent_observer_tick():
    """手动触发一次扫描; force=True 忽略新增量门槛, 便于演示时立即看到产出。"""
    try:
        record = _observer_tick(force=True)
    except Exception as exc:
        return jsonify({"emitted": False, "record": None, "error": f"扫描失败: {exc}", **_observer_status()}), 500
    return jsonify({"emitted": bool(record), "record": record, **_observer_status()})


# ==== 主入口 ====

def main():
    global mqtt_client

    print(f"=== 手语识别 MQTT Bridge ===")
    print(f"MQTT: {BROKER}:{PORT}")
    print(f"HTTP: http://{HTTP_HOST}:{HTTP_PORT}")
    print(f"设备: {DEVICE}")
    print(f"数据目录: {DATA_DIR}\n")

    # 启动 MQTT
    mqtt_client = mqtt.Client(
        mqtt.CallbackAPIVersion.VERSION2,
        client_id="pc_bridge_" + str(int(time.time())),
        protocol=mqtt.MQTTv5,
    )
    mqtt_client.on_connect = on_connect
    mqtt_client.on_message = on_message
    mqtt_client.on_disconnect = on_disconnect
    mqtt_client.reconnect_delay_set(min_delay=2, max_delay=10)
    mqtt_client.connect(BROKER, PORT, keepalive=60)
    mqtt_client.loop_start()

    # 启动练习观察 Agent (后台线程, 只读日志)
    start_observer_agent()

    # 启动 Flask
    try:
        app.run(host=HTTP_HOST, port=HTTP_PORT, debug=False, threaded=True)
    except KeyboardInterrupt:
        pass
    finally:
        mqtt_client.loop_stop()
        mqtt_client.disconnect()
        print("\n[Bridge] 已退出")


if __name__ == "__main__":
    main()
