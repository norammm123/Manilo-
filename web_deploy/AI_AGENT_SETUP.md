# AI 学习报告与对话助手配置

## 现有数据链路

硬件 MQTT 主题和报文格式保持不变。Flask 继续接收 `/prediction`、`/sentence`、`/asr`、`/pinyin` 等现有事件，保存到 `mqtt_logs/*.jsonl`，并通过现有 SSE 推送到学习主站和 B 端。后端按日志发生时间关联正在进行的学习会话。

设备识别置信度只说明模型对本次识别的把握，不等于学习者掌握度。一次语音转写非空只能说明识别服务产出了文本；如果没有输入目标句，无法比较学习者是否说对。主站的“本次目标句”是可选的会话元数据，不要求硬件改报文。设置目标句后，后端会忽略空格和标点后计算文本吻合率及字符错误率（CER）。

## 模型服务

报告生成和 AI 对话由 Flask 服务端调用 OpenAI-compatible Chat Completions 接口。配置以下进程环境变量：

- `LLM_API_URL`：完整的 Chat Completions URL，例如 OpenAI-compatible 服务的 `/v1/chat/completions` 地址。
- `LLM_API_KEY`：模型供应商密钥。不要放进前端文件或提交到 Git。
- `LLM_MODEL`：供应商支持的模型名称。

PowerShell 当前窗口启动示例（密钥仅存在进程环境，不会写入项目文件）：

```powershell
$env:LLM_API_URL = "https://api.openai.com/v1/chat/completions"
$env:LLM_MODEL = "填写你账号可用的模型名"
$env:LLM_API_KEY = Read-Host "输入模型 API Key"
python mqtt_bridge.py
```

如果通过 `start_all.bat` 启动 Flask，应先在同一个终端窗口设置这些变量再运行启动脚本。更改环境变量后需要重启 Flask。B 端的“模型已配置”状态来自 `/api/assistant/status`。

OpenAI 官方明确要求不要把标准 API Key 暴露在浏览器代码中，因此页面不提供密钥输入框，也不会将密钥发送到 B 端。[API Overview](https://developers.openai.com/api/reference/overview)

## 端点与行为

- 学习结束：`POST /api/learning/finish` 自动生成日志摘要；配置模型后再请求模型生成报告。
- B 端报告：`GET /api/learning/sessions`、`GET /api/learning/reports`、`POST /api/learning/report`。
- 对话助手：`POST /api/assistant/chat`。服务端会附上所选会话的指标和最多 12 条历史对话。
- 学习日志、对话和报告使用本项目的本地 JSONL 日志存储；AI 关闭或调用失败时，报告会退回本地统计摘要，对话会返回明确错误。

## 指标边界

- 学习次数、会话时长、识别次数、完成句子数、词汇覆盖：描述学习活动。
- 跟读转写成功率：当前以 ASR 结果是否非空作为最低限度的转写产出信号。
- 目标文本吻合率与 CER：仅对提供了目标句的会话有效。
- 完整掌握度仍需多次评测、教师标注或迁移练习等证据；报告 Agent 不应仅凭设备置信度或 ASR 有文本作此结论。
