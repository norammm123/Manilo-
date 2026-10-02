# 学习报告 Agent 接入

学习结束时，主站通过 `/api/learning/finish` 把会话交给 Flask。Flask 根据 session ID 和时间范围读取本地设备日志、语音转写日志、Agent trace 与对话记录，生成统计指标并调用报告模型。B 端也可以选择已结束会话，通过 `/api/learning/report` 重新生成；最近报告保存在 `mqtt_logs/learning_reports.jsonl`。

优先配置 `LLM_API_URL`、`LLM_API_KEY`、`LLM_MODEL`，同一模型服务同时用于报告和 `/api/assistant/chat`。详细配置见 [AI_AGENT_SETUP.md](AI_AGENT_SETUP.md)。现有自建报告服务仍可通过 `LEARNING_REPORT_AGENT_URL` 单独接入；该变量只影响报告生成，对话助手使用通用 `LLM_*` 配置。

报告模型收到 `task`、`session`、`metrics`、`events`、`agent_logs`、`conversation_messages`。任务指令要求模型区分设备置信度、转写成功和目标文本吻合；没有目标语句或评测数据时，不能推断完整掌握度。

模型未配置时，结束学习和 B 端报告功能仍返回本地日志摘要，并标记 `agent_used: false`。模型调用失败时，学习记录保留并显示本地摘要；对话助手会返回可见错误，不生成伪造的模型回答。
