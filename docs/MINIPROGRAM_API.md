# 小程序接口文档（手语/拼音学习后端）

本文档面向做微信小程序的队友，说明如何调用 Manilo 后端的 HTTP 接口。

后端是**跑在 PC 上的 Flask 服务**（`backend/mqtt_bridge.py`），默认监听 `0.0.0.0:5000`，
它从 MQTT 接收手套（F4）的数据、落盘日志，并把数据通过 HTTP 提供出去。

> 本文档只讲小程序用得到的部分。B 端看板（网页）的接口约定见 `frontend/API_CONTRACT.md`，两者是同一套后端。

---

## 1. 先说三个必须知道的坑

小程序和后端不在同一台机器上，有三条硬限制，**先看完再写代码**：

### 1.1 `wx.request` 用不了 SSE

后端有个 `GET /api/stream` 是 **SSE（Server-Sent Events）** 实时推送，网页端用它。
但 **`wx.request` 不支持 SSE**。小程序里只能用：

- **轮询**（推荐，现在就能用）：定时 `GET` 请求拿最新数据。
- **WebSocket**（`wx.connectSocket`）：后端目前**没有** WebSocket 接口，需要后端加（见 §6）。

### 1.2 生产环境必须 HTTPS + 已备案域名

微信要求正式发布的请求域名：

- 必须是 **HTTPS**；
- 必须在微信公众平台 **「开发管理 → 服务器域名 → request 合法域名」** 里配置；
- 域名必须完成 **ICP 备案**。

也就是说，**打包上线**时不能直接连 `http://192.168.x.x:5000`，需要一个公网 HTTPS 域名反代到 PC。

### 1.3 开发阶段怎么连（重点）

开发调试时用**开发者工具**，可以绕过上面的限制：

1. 微信开发者工具 → 右上角 **详情 → 本地设置**；
2. 勾选 **「不校验合法域名、web-view（业务域名）、TLS 版本以及 HTTPS 证书」**；
3. 这时可以直接请求 `http://<PC的局域网IP>:5000`。

**注意 `localhost` 指的是手机/模拟器自己，不是你的 PC。** 必须用 PC 的局域网 IP。

**当前手套 PC 的局域网 IP：`10.175.178.245`**（Wi-Fi，**已设为静态地址，不会再变**；掩码 `255.255.255.0`，网关/DNS `10.175.178.175`）。
联不上时先在这台 PC 上 `ipconfig`，找 **WLAN** 适配器的 IPv4 确认一次——别用 `192.168.0.100`（那是以前接板子的有线网，网线已拔）、也别用 VMware 的 `192.168.x.1`（虚拟网卡）。

> ⚠️ 静态地址是绑在 Wi-Fi 网卡上的：**以后连别的 Wi-Fi（家里/学校）会连不上**，需改回 DHCP：
> `netsh interface ip set address name="WLAN" dhcp` + `netsh interface ip set dns name="WLAN" dhcp`（管理员）。换新热点后要重新 `ipconfig` 取值并重设静态。

手机/模拟器和这台 PC 要在**同一个局域网络**下。
> ⚠️ **不要连公共 WLAN。** 很多公共 WLAN 开了「AP 客户端隔离」，同网两台设备互相不可见，小程序请求会卡到超时（curl error 28）。现在是用**手机热点**组网（双方都连同一个热点），手机热点默认不隔离。换热点后 IP 会变，按上面方法重新 `ipconfig` 取 WLAN 那个 IPv4。

小程序里配一个全局 baseUrl：

```js
// app.js
App({
  globalData: {
    // 开发：手套 PC 的局域网 IP（当前 10.175.178.245）；上线：换成 HTTPS 域名
    baseUrl: 'http://10.175.178.245:5000',
  },
})
```

### 1.4 后端跑在哪（对接分工）

- **后端跑在手套那台 PC 上**（不是小程序开发机）。同一台 PC 上还有：HC-08 蓝牙串口 + `pc_inference.py`（跑模型），链路是 `F4 →(蓝牙)→ pc_inference → MQTT → 后端 → HTTP`。
- 小程序开发机只需**联同一个局域网**，请求 `http://<手套PC的IP>:5000`。
- 手套 PC 要**放行 5000 端口**：首次启动后端时 Windows 会弹窗，点「允许访问」；没弹或点错了，手动加一条入站规则放行 TCP 5000。
- 查 IP：在手套 PC 上 `ipconfig`，取无线/以太网适配器的 IPv4。

> **⚠️ 别用另一份部署包起服务。** 如果收到过一套 `deploy_backend.bat` + 根目录 `mqtt_bridge.py`，那是**另一个后端**（接口不一样，且同样占 5000 端口），拿它起服务会跟本文档描述的接口对不上。本文档对应的后端只有一个：仓库里的 `backend/mqtt_bridge.py`，由 `start_all.bat` 启动。

---

## 2. 通用约定

- **所有接口都返回 JSON**，`Content-Type: application/json`。
- **POST 请传 JSON 体**（`wx.request` 默认就是 `application/json`，直接传 object 即可）。
- **时间字段**：
  - `ts`：Unix 秒（浮点数，如 `1759450000.123`）；
  - `time`：服务器本地时间字符串 `"HH:MM:SS"`，只用于展示。
- 返回 `null` 表示"暂无数据"（比如还没有识别记录时 `avg_confidence` 为 `null`），不要当成 0。
- HTTP 状态码：`200` 成功、`201` 已记录、`400` 参数错、`404` 找不到、`409` 状态冲突（如重复结束会话）、`502/503` AI 服务不可用。

### 请求封装（建议直接用）

```js
// utils/request.js
const app = getApp()

function request(method, path, data) {
  return new Promise((resolve, reject) => {
    wx.request({
      url: app.globalData.baseUrl + path,
      method,
      data,
      header: { 'content-type': 'application/json' },
      success: (res) => {
        if (res.statusCode >= 200 && res.statusCode < 300) resolve(res.data)
        else reject({ statusCode: res.statusCode, body: res.data })
      },
      fail: reject,
    })
  })
}
module.exports = {
  get: (p) => request('GET', p),
  post: (p, d) => request('POST', p, d || {}),
}
```

---

## 3. 接口总览

| 分类 | 方法 | 路径 | 用途 |
| --- | --- | --- | --- |
| 实时状态 | GET | `/api/stats` | 设备在线状态 + 今日汇总（**轮询用这个**） |
| 实时状态 | GET | `/api/history?n=50` | 最近 N 条成句记录 |
| 学习会话 | POST | `/api/learning/start` | 开始一次练习 |
| 学习会话 | GET | `/api/learning/session?session_id=` | 查询某次练习实时进度（**轮询用这个**） |
| 学习会话 | GET | `/api/learning/sessions?limit=30` | 历史会话列表 |
| 学习会话 | POST | `/api/learning/finish` | 结束练习并生成报告 |
| 学习会话 | POST | `/api/learning/report` | 对已结束会话重新生成报告 |
| 学习会话 | GET | `/api/learning/reports` | 报告列表 |
| 学习数据 | GET | `/api/learning/stats` | 今日练习统计 |
| 学习数据 | GET | `/api/learning/analytics` | 7 天趋势 + 词汇掌握 + 跟读分析（**报表页用这个**） |
| 上报 | POST | `/api/learning/follow-read` | 上报一次跟读转写结果 |
| 上报 | POST | `/api/conversation` | 记一条对话消息 |
| 上报 | POST | `/api/agent/log` | 记一条 Agent 日志 |
| AI 助手 | GET | `/api/assistant/status` | AI 助手是否可用 |
| AI 助手 | POST | `/api/assistant/chat` | 向 AI 助手提问 |
| 日志查询 | GET | `/api/agent/logs` | 最近 100 条 Agent 日志 |
| 设备管理 | POST | `/api/cmd` | 下发设备命令（阈值等） |
| 设备管理 | GET | `/api/thresholds` | 读取已保存的阈值 |
| 设备管理 | GET | `/api/agent/observer` | 观察 Agent 运行状态 |
| 实时推送 | GET | `/api/stream` | SSE（**小程序用不了，见 §1.1**） |

---

## 4. 接口详情

### 4.1 `GET /api/stats` — 设备状态 + 今日汇总

轮询首选。建议每 2~3 秒一次。

**响应**

```json
{
  "online": true,
  "version": "v12",
  "last_seen": "14:32:07",
  "sentences_today": 18,
  "predictions_today": 240,
  "avg_confidence": 0.923
}
```

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `online` | bool | 手套设备是否在线 |
| `version` | string | 设备上报的模型版本，未知时为 `"?"` |
| `last_seen` | string \| null | 最后在线时间 `HH:MM:SS`，离线为 `null` |
| `sentences_today` | int | 今日成句数 |
| `predictions_today` | int | 今日识别次数 |
| `avg_confidence` | float | 今日平均置信度（0~1）；无数据时为 `null` |

### 4.2 `GET /api/history?n=50` — 最近成句

`n` 默认 `50`（取最近 n 条）。返回**数组**，按时间正序（最早在前）。

```json
[
  { "text": "你好", "ts": 1759450000.12, "time": "14:32:01", "session_id": "a1b2c3..." }
]
```

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `text` | string | 识别出的中文句子 |
| `ts` | float | Unix 秒 |
| `time` | string | `HH:MM:SS` |
| `session_id` | string | 可选，所属学习会话 id |

### 4.3 `POST /api/learning/start` — 开始练习

**请求**

```json
{ "mode": "gesture", "expected_text": "你好" }
```

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `mode` | 是 | `"gesture"`（手势直译）或 `"pinyin"`（拼音拼写），其它值返回 400 |
| `expected_text` | 否 | 本次练习的目标文本（跟读/比对用），最长 500 字 |

**响应 `200`**

```json
{
  "id": "9f2c...",
  "started_at": 1759450100.5,
  "mode": "gesture",
  "expected_text": "你好",
  "predictions": 0,
  "sentences": 0,
  "sessions_today": 3
}
```

> 用返回的 `id` 作为后续 `session_id`。**开始练习后，手套产生的识别记录会自动关联到这个会话**，不用你手动上报。

**错误**：400 `{"error": "invalid learning mode"}`

### 4.4 `GET /api/learning/session?session_id=<id>` — 查询练习实时进度

**响应（进行中）`200`**

```json
{
  "active": true,
  "id": "9f2c...",
  "started_at": 1759450100.5,
  "mode": "gesture",
  "expected_text": "你好",
  "predictions": 12,
  "sentences": 2
}
```

**已结束**：`200` `{"active": false}`
**不存在**：`404` `{"active": false}`

> 判断条件：`active === true` 才算进行中。`predictions` / `sentences` 是**实时**数字，练习页轮询这个接口即可显示进度。

### 4.5 `GET /api/learning/sessions?limit=30` — 会话列表

`limit` 默认 `30`，自动限制在 `1~100`。按开始时间**倒序**（最新在前）。

```json
[
  {
    "id": "9f2c...",
    "started_at": 1759450100.5,
    "ended_at": 1759450400.2,
    "mode": "gesture",
    "expected_text": "你好",
    "status": "finished"
  }
]
```

| 字段 | 说明 |
| --- | --- |
| `ended_at` | 未结束时可能缺失 |
| `status` | `"active"` 或 `"finished"` |

### 4.6 `POST /api/learning/finish` — 结束练习并生成报告

**请求**

```json
{ "session_id": "9f2c..." }
```

**响应 `200`**

```json
{
  "session_id": "9f2c...",
  "started_at": 1759450100.5,
  "ended_at": 1759450400.2,
  "mode": "gesture",
  "report": { },
  "agent_used": true,
  "agent_report": "本次练习……",
  "agent_error": null,
  "summary": "本次练习……",
  "sessions_today": 4
}
```

| 字段 | 说明 |
| --- | --- |
| `report` | 客观统计（识别次数、成句数、跟读成功率等，字段随会话内容变化） |
| `agent_used` | 是否用了大模型生成点评 |
| `agent_report` | 模型点评，未用模型时为空字符串 |
| `agent_error` | 模型调用失败原因，成功为 `null` |
| `summary` | 最终展示用摘要（模型点评，无模型时是兜底文案），**直接展示这个** |

**错误**：`404` 会话不存在；`409` 会话已结束。
**注意**：这个接口可能调用大模型，**耗时可能十几秒**，`wx.request` 记得调大 `timeout`：

```js
wx.request({ url: ..., timeout: 60000 })
```

### 4.7 `POST /api/learning/report` — 重新生成报告

请求 `{ "session_id": "..." }`。只能对**已结束**的会话调用（否则 `409`）。
返回结构同 §4.6 的报告记录。同样可能很慢。

### 4.8 `GET /api/learning/reports` — 报告列表

返回最近 50 条报告，**倒序**。每项结构：

```json
{
  "session_id": "9f2c...",
  "started_at": 1759450100.5,
  "ended_at": 1759450400.2,
  "mode": "gesture",
  "metrics": { },
  "summary": "本次练习……",
  "agent_used": true,
  "agent_error": "",
  "created_at": 1759450401.1
}
```

### 4.9 `GET /api/learning/stats` — 今日统计

```json
{ "sessions_today": 3, "predictions_today": 240, "sentences_today": 18 }
```

### 4.10 `GET /api/learning/analytics` — 报表页数据（重点）

一次拿到 7 天趋势、词汇掌握、跟读分析。字段较多，按需取用。

```json
{
  "daily": [
    { "date": "2026-09-27", "sessions": 2, "recognitions": 120, "sentences": 9 }
  ],
  "vocabulary": [
    { "label": "你好", "exposures": 20, "checked": 15, "correct": 13, "accuracy": 0.867 }
  ],
  "vocabulary_count": 12,
  "follow_read_attempts": 30,
  "follow_read_successes": 25,
  "follow_read_success_rate": 0.833,
  "follow_read_match_samples": 20,
  "follow_read_match_rate": 0.7,
  "follow_read_character_error_rate": 0.11,
  "follow_read_by_phrase": [
    { "label": "你好", "attempts": 10, "successes": 9, "success_rate": 0.9 }
  ],
  "error_rate": 0.13,
  "error_samples": 15,
  "mastery_available": true,
  "sessions_today": 3,
  "average_session_seconds": 420.5
}
```

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `daily` | array(7) | 最近 7 天，按日期正序，`date` 为 `YYYY-MM-DD` |
| `vocabulary` | array(≤12) | 按"被检测次数"排序的前 12 个词 |
| `vocabulary[].accuracy` | float \| null | `correct / checked`；`checked` 为 0 时是 `null` |
| `vocabulary_count` | int | 词汇总量（不只是前 12） |
| `follow_read_attempts` | int | 跟读总次数 |
| `follow_read_success_rate` | float \| null | 转写成功（转出了内容）的比例 |
| `follow_read_match_rate` | float \| null | **与目标文本吻合**的比例（比上面更严格） |
| `follow_read_character_error_rate` | float \| null | 字符错误率 CER，越低越好 |
| `follow_read_by_phrase` | array(≤12) | 按语句分组的跟读成功率 |
| `error_rate` / `error_samples` | float \| null / int | 识别错误率及其样本量 |
| `mastery_available` | bool | 是否有足够数据判断掌握度（为 false 时别展示掌握度） |
| `average_session_seconds` | float \| null | 今日平均单次练习时长（秒） |

> **重点**：`transcription_ok`（转写成功）和 `matches_expected`（与目标吻合）是两个独立信号，UI 上不要混为一谈——转写成功 ≠ 读对了。

### 4.11 `POST /api/learning/follow-read` — 上报跟读结果

如果小程序端自己做语音识别，把结果上报给后端，后端会纳入报表统计。

**请求**

```json
{
  "session_id": "9f2c...",
  "expected_text": "你好",
  "transcribed_text": "你好",
  "transcription_ok": true,
  "matches_expected": true,
  "character_error_rate": 0.0
}
```

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `transcription_ok` | **是** | 必须是**布尔值**，否则返回 400 |
| `expected_text` | 否 | 目标文本 |
| `transcribed_text` | 否 | 实际转写文本 |
| `matches_expected` | 否 | 布尔值，是否吻合 |
| `character_error_rate` | 否 | 数值，字符错误率 |

**响应 `201`** `{ "status": "recorded", "data": { ... } }`

### 4.12 `POST /api/conversation` — 记一条对话

**请求**

```json
{ "role": "learner", "text": "我想练习问候语", "session_id": "9f2c..." }
```

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `role` | 否 | `"learner"` 或 `"assistant"`；传 `"patient"` 会自动转成 `"learner"`；默认 `"learner"` |
| `text` | **是** | 内容，最长 2000 字；也接受字段名 `content` |
| `session_id` | 否 | 关联会话 |

**响应 `201`** `{ "status": "recorded", "data": { "ts": ..., "time": "14:32:01", "role": "learner", "text": "..." } }`
**错误**：400 text 为空。

### 4.13 `POST /api/agent/log` — 记一条 Agent 日志

**请求**（字段都可选，兼容别名）

```json
{ "phase": "感知", "input": "识别到你好", "decision": "播报", "detail": "详情...", "session_id": "" }
```

| 字段 | 别名 | 说明 |
| --- | --- | --- |
| `phase` | `type` | 阶段名，最长 40 字 |
| `input` | `perception` | 输入，最长 500 字 |
| `decision` | `action` | 决策，最长 500 字 |
| `detail` | `message` | 详情，最长 1000 字 |
| `session_id` | — | 最长 80 字 |

**响应 `201`** `{ "status": "recorded", "data": { ... } }`；400 表示请求体不是 JSON 对象。

### 4.14 `GET /api/agent/logs` — 最近 100 条 Agent 日志

```json
[
  { "ts": 1759450300.1, "time": "14:31:40", "phase": "观察", "input": "...", "decision": "...", "detail": "...", "session_id": "" }
]
```

### 4.15 `GET /api/assistant/status` — AI 助手状态

```json
{
  "configured": true,
  "chat_configured": true,
  "report_configured": true,
  "report_agent_configured": false,
  "model": "deepseek-flash"
}
```

先查这个：`chat_configured` 为 `false` 时，**不要显示聊天入口**（调用会返回 503）。

### 4.16 `POST /api/assistant/chat` — 向 AI 助手提问

**请求**

```json
{ "text": "我今天的表现怎么样？", "session_id": "9f2c..." }
```

| 字段 | 必填 | 说明 |
| --- | --- | --- |
| `text` | 是 | 问题，最长 2000 字 |
| `session_id` | 否 | 传了会带上该次练习的数据作为上下文 |

**响应 `200`**

```json
{
  "reply": "……",
  "learner_message": { "role": "learner", "text": "...", "ts": 1759450500.1 },
  "assistant_message": { "role": "assistant", "text": "...", "ts": 1759450512.3 }
}
```

**错误**：400 空问题；503 未配置模型；502 模型调用失败（响应里也带 `learner_message`）。
**注意**：大模型生成，**很慢**，`timeout` 给大一点（建议 90000）。

### 4.17 `POST /api/cmd` — 下发设备命令

管理端用。`cmd` 会和 `params` 展平后发给手套。

**请求**

```json
{ "cmd": "set_threshold", "params": { "value": 0.75 } }
```

支持的 `cmd`：

| cmd | params | 作用 |
| --- | --- | --- |
| `set_threshold` | `{value: float}` | 全局置信度阈值 |
| `set_cooldown` | `{value: float}` | 词语冷却（秒） |
| `set_word_threshold` | `{label: str, value: float}` | 单词阈值 |
| `set_letter_threshold` | `{label: str, value: float}` | 单字母阈值 |
| `reset_letter_thresholds` | — | 重置字母阈值 |
| `reset_thresholds` | — | 重置单词阈值 |
| `set_margin` | `{value: float}` | 最小 margin |
| `set_vote_window` | `{value: int}` | 投票窗口 |
| `set_vote_majority` | `{value: int}` | 投票多数 |
| `set_silence_timeout` | `{value: float}` | 静音超时 |

**响应 `200`**

```json
{ "status": "sent", "cmd": "set_threshold" }
```

MQTT 断了会返回 `{ "status": "saved", "cmd": "...", "warning": "MQTT not connected, saved locally" }`（已保存，设备恢复后生效）。
**错误**：400 缺 `cmd`。**请求体必须是合法 JSON**（后端用 `force=True` 解析）。

### 4.18 `GET /api/thresholds` — 读取已保存阈值

返回一个对象，内容随保存过的设置变化，例如：

```json
{ "confidence_thresh": 0.75, "word_cooldown": 2.0, "vote_window": 5 }
```

### 4.19 `GET /api/agent/observer` — 观察 Agent 状态

```json
{
  "enabled": true,
  "phase": "观察",
  "interval_seconds": 30,
  "llm_interval_seconds": 120,
  "min_delta": 2,
  "llm_enabled": true,
  "llm_configured": true,
  "scans": 42,
  "llm_calls": 3,
  "llm_errors": 0,
  "last_emit_ts": 1759450490.5,
  "last_insight": { "phase": "观察", "input": "...", "decision": "...", "detail": "..." }
}
```

`last_insight` 为 `null` 时还没有产出过建议。

---

## 5. 推荐用法：用轮询替代 SSE

小程序用不了 SSE，建议这样组织：

### 5.1 首页 / 设备状态卡片

```js
Page({
  data: { stats: null },
  onShow() { this.startPolling() },
  onHide() { this.stopPolling() },

  startPolling() {
    this.timer = setInterval(async () => {
      const stats = await require('../../utils/request').get('/api/stats')
      this.setData({ stats })
    }, 3000) // 3 秒一次
  },
  stopPolling() { clearInterval(this.timer) },
})
```

### 5.2 练习页（进行中会话）

轮询 `/api/learning/session` 拿实时进度：

```js
const { get } = require('../../utils/request')

async startPractice() {
  const s = await get('/api/learning/start', {})     // 实际用 post
  this.sessionId = s.id
  this.timer = setInterval(async () => {
    const live = await get('/api/learning/session?session_id=' + this.sessionId)
    if (!live.active) { clearInterval(this.timer); return }
    this.setData({ predictions: live.predictions, sentences: live.sentences })
  }, 2000)
}

async finishPractice() {
  clearInterval(this.timer)
  wx.showLoading({ title: '生成报告…' })
  try {
    const report = await post('/api/learning/finish', { session_id: this.sessionId })
    this.setData({ summary: report.summary })
  } finally { wx.hideLoading() }
}
```

> 注意 `startPractice` 里的 `get` 应为 `post`，示例只为展示轮询结构。

### 5.3 落地建议

- 状态 / 进度：**2~3 秒**轮询一次。
- 报表 / 列表页：进入页面拉一次即可，不需要轮询。
- 页面 `onHide` / `onUnload` 一定要 `clearInterval`，否则后台还在请求。
- 所有接口都是**只读+追加**，重复请求安全，不用担心轮询产生副作用。

---

## 6. 如果一定要"服务器主动推"（WebSocket）

轮询够用就别折腾。若确实需要推送（比如识别到新句子立刻上屏，且不接受 2~3 秒延迟），
只能走 WebSocket，但**后端目前没有 WebSocket 接口**——现在的 `/api/stream` 是 SSE，`wx.connectSocket` 连不上。

需要后端加一个（例如用 `flask-sock` 或 `flask-socketio`），把现有 `_broadcast_sse` 的事件同时推给 WS 客户端。
**这属于后端新需求，需要和写后端的人确认后再做。**

---

## 7. 事件类型参考（若后端将来加了 WebSocket）

后端内部广播的事件 `type` 有这些，字段与 HTTP 接口里的同名对象一致：

| type | 对应 HTTP 接口的数据 |
| --- | --- |
| `init` | 初始快照（含 status/sentences/predictions/asr/pinyin/agent_logs/conversation/follow_read/thresholds） |
| `status` | 同 `/api/stats` 的在线部分 |
| `prediction` | 一次识别：`{label, confidence, latency_ms, ts, time, session_id?}` |
| `sentence` | 同 `/api/history` 的一条 |
| `asr` | 一次语音识别结果：`{text, ts, time, session_id?}` |
| `pinyin_state` | 拼音状态：`{letters, current_sentence, ts, session_id?, ...}` |
| `agent_log` | 同 `/api/agent/logs` 一条 |
| `conversation` | 同 `/api/conversation` 一条 |
| `follow_read` | 同 `/api/learning/follow-read` 一条 |
| `pong` | 心跳响应 |
| `learning_finished` | 会话结束报告（同 `/api/learning/finish`） |
| `learning_report` | 重新生成的报告 |

> `prediction` 里的 `confidence` 单位是 **0~1**（网页端历史上出现过 0~100，后端统计时会自动归一化；小程序直接按 0~1 展示，超过 1 就除以 100）。

---

## 8. 联调检查清单

1. PC 上启动后端（`start_all.bat` 或 `python backend/mqtt_bridge.py`），确认打印 `HTTP: http://0.0.0.0:5000`。
2. 手机浏览器打开 `http://<PC-IP>:5000/api/stats`，能看到 JSON → 网络通。
3. 开发者工具勾选「不校验合法域名」，`wx.request` 请求 `http://<PC-IP>:5000/api/stats`。
4. 不能只写 `localhost` —— 那是手机自己。
5. 正式发布前：把域名换成已备案的 HTTPS 域名，并加进「request 合法域名」白名单。
