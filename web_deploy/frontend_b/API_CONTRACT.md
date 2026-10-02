# B 端看板与后端联调约定

## 启动与边界

- 运行 `start_admin_dashboard.bat`，B 端独立地址为 `http://localhost:5174`。
- 学习者主站仍运行在 `http://localhost:5000`，主站不提供管理员跳转入口。
- B 端读取 Flask HTTP API 与 SSE。页面可改 API 地址；如后端提供 WebSocket，可用 `?ws=ws://...`，失败时回退 SSE。
- B 端用 Vue 3 与 ECharts CDN，无 Node/npm 安装步骤；“演示数据”仅用于界面预览和联调。

## 共用事件流

浏览器 A、B 不共享 JS 内存；Flask 后端是两端共享的数据源。SSE/WebSocket 消息使用 envelope：

```json
{"type":"agent_log","data":{"ts":1790918700.2,"phase":"感知","input":"手语：苹果","decision":"拆解音素 /p/","session_id":"session-id"}}
```

事件类型包括 `init`、`status`、`prediction`、`sentence`、`asr`、`pinyin_state`、`agent_log`、`conversation`、`follow_read`。Agent trace 被持久化并广播；它本身不调用大模型。

## Agent 日志

HTTP `POST /api/agent/log`，也可发布到 MQTT topic `glove/board1/agent/log`：

```json
{
  "phase": "感知",
  "input": "手语：苹果",
  "decision": "拆解音素 /p/ 并播放手形提示",
  "detail": "识别置信度 0.91；已启动跟练提示",
  "session_id": "session-id"
}
```

日志追加至 `mqtt_logs/agent_logs.jsonl`，并实时广播。情绪字段当前不使用。

## 跟读转写

HTTP `POST /api/learning/follow-read`，或 MQTT topic `glove/board1/learning/follow_read`：

```json
{
  "session_id": "session-id",
  "expected_text": "我喜欢苹果",
  "transcribed_text": "我喜欢苹果",
  "transcription_ok": true,
  "matches_expected": true
}
```

- `transcription_ok` 必须由语音识别后端明确提供布尔值；不能单凭设备置信度推断。
- `matches_expected` 可选，只有后端做过目标文本比较时才传布尔值。
- 统计口径分开：转写成功率 = `transcription_ok=true` 的次数 / 有效跟读事件数；目标文本吻合率只用带布尔 `matches_expected` 的事件计算。
- `transcription_ok` 可作为跟读过程参考，不等于完整的词汇掌握或学习效果。
- 记录保存在 `mqtt_logs/follow_read.jsonl`，SSE 事件类型为 `follow_read`，B 端看板按目标语句汇总。

## 对话与分析

- 对话：`POST /api/conversation`，字段 `{ "role": "learner" | "assistant", "text": "...", "session_id": "..." }`。日志保存在 `mqtt_logs/conversation.jsonl`。
- `/api/learning/analytics` 返回会话次数、设备识别次数、句子数、词汇覆盖、七日趋势、跟读转写率和目标文本吻合率。
- 词汇覆盖表示练习日志里出现过的识别标签，不代表掌握。
- 识别置信度表示设备模型信号，不代表转写成功或学习正确率。

## 学习报告与 AI 对话

- 学习者结束会话时，主站调用 `POST /api/learning/finish`；Flask 汇总当前会话日志，返回本地摘要，并在模型已配置时附带模型报告。
- B 端会话清单：`GET /api/learning/sessions`；报告历史：`GET /api/learning/reports`；重新生成：`POST /api/learning/report`，JSON `{ "session_id": "..." }`。
- AI 对话：`POST /api/assistant/chat`，JSON `{ "text": "...", "session_id": "..." }`。后端把本次学习统计和该会话历史交给模型，并把问答保存为 conversation 事件。
- 配置状态：`GET /api/assistant/status` 返回 `report_configured` 与 `chat_configured`。独立 `LEARNING_REPORT_AGENT_URL` 只启用报告；对话助手需要 `LLM_API_URL`、`LLM_API_KEY`、`LLM_MODEL`。Agent trace 仍保存并纳入报告上下文，但看板主面板已改为学习报告。
- 模型密钥只读后端环境变量，不从浏览器传入。
