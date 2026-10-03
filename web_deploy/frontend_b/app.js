(function () {
  if (!window.Vue) return;

  const { createApp, nextTick } = Vue;
  const demoConversations = [
    { role: 'learner', text: '这个手势总是识别不出来。', time: '10:05:08' },
    { role: 'assistant', text: '我们慢一点试试。先伸出食指，再把拇指放到掌心。', time: '10:05:10' },
    { role: 'learner', text: '苹果。', time: '10:05:12' },
    { role: 'assistant', text: '识别到“苹果”。手形很稳定，继续保持。', time: '10:05:13' },
  ];

  createApp({
    template: '#app-template',
    data() {
      const query = new URLSearchParams(location.search);
      const savedBase = localStorage.getItem('gloveBApiBase');
      const apiBase = (query.get('api') || savedBase || 'http://localhost:5000').replace(/\/$/, '');
      return {
        apiBase,
        apiDraft: apiBase,
        demoMode: query.get('demo') === '1',
        connected: false,
        streamMode: 'sse',
        deviceVersion: '',
        lastSeen: '',
        conversations: [],
        sessions: [],
        reports: [],
        selectedSessionId: '',
        currentReport: null,
        reportError: '',
        assistantConfigured: false,
        reportConfigured: false,
        assistantModel: '',
        assistantMessage: '',
        draftMessage: '',
        chatSending: false,
        reportGenerating: false,
        predictions: [],
        stats: { sessions_today: 0, predictions_today: 0, sentences_today: 0 },
        analytics: { daily: [], vocabulary: [], vocabulary_count: 0, follow_read_attempts: 0, follow_read_successes: 0, follow_read_success_rate: null, follow_read_match_samples: 0, follow_read_match_rate: null, follow_read_character_error_rate: null, follow_read_by_phrase: [], sessions_today: 0, average_session_seconds: null },
        thresholds: { confidence_thresh: 0.6, word_cooldown: 0.8, min_margin: 0.1, vote_window: 7, vote_majority: 4 },
        wordThresholds: {},
        letterThresholds: {},
        settingsLoaded: false,
        settingsMessage: '',
        gestureLabels: ['不','你','叫','哪儿','在','天','好','妈妈','很','我','打电话','早上','是','有','来','要','认识'],
        pinyinLabels: ['A','B','C','CH','D','E','F','G','H','I','J','K','L','M','N','NG','O','P','Q','R','S','SH','T','U','V','W','X','Y','Z','ZH'],
        settingFields: [
          { key: 'confidence_thresh', label: '置信度阈值', min: 0.1, max: 0.95, step: 0.05, command: 'set_threshold' },
          { key: 'word_cooldown', label: '词语冷却 (秒)', min: 0.3, max: 2.5, step: 0.1, command: 'set_cooldown' },
          { key: 'min_margin', label: '模糊过滤', min: 0, max: 0.4, step: 0.02, command: 'set_margin' },
          { key: 'vote_window', label: '投票窗口', min: 3, max: 15, step: 1, command: 'set_vote_window' },
          { key: 'vote_majority', label: '多数票', min: 2, max: 15, step: 1, command: 'set_vote_majority' },
        ],
        charts: {},
        source: null,
        reconnectTimer: null,
        refreshTimer: null,
        socket: null,
        observerLogs: [],
        observerMessage: '',
        observerScanning: false,
        observerDraftInterval: 30,
        observerConfig: { enabled: false, llm_enabled: false, llm_configured: false, interval_seconds: 30, llm_interval_seconds: 120, min_delta: 2, scans: 0, llm_calls: 0, llm_errors: 0, last_emit_ts: null, last_insight: null },
      };
    },
    computed: {
      todayLabel() {
        return new Intl.DateTimeFormat('zh-CN', { year: 'numeric', month: 'long', day: 'numeric', weekday: 'short' }).format(new Date());
      },
      metrics() {
        if (this.demoMode) return { sessions: 6, recognitions: 84, vocabulary: 12, followReadRate: 0.86, followReadSamples: 21, followReadMatchRate: 0.71, followReadMatchSamples: 17, followReadCer: 0.08, averageSessionSeconds: 486 };
        return {
          sessions: this.analytics.sessions_today ?? this.stats.sessions_today ?? 0,
          recognitions: this.stats.predictions_today ?? 0,
          vocabulary: this.analytics.vocabulary_count ?? 0,
          followReadRate: this.analytics.follow_read_success_rate ?? null,
          followReadSamples: this.analytics.follow_read_attempts ?? 0,
          followReadMatchRate: this.analytics.follow_read_match_rate ?? null,
          followReadMatchSamples: this.analytics.follow_read_match_samples ?? 0,
          followReadCer: this.analytics.follow_read_character_error_rate ?? null,
          averageSessionSeconds: this.analytics.average_session_seconds ?? null,
        };
      },
      visibleConversations() {
        if (this.demoMode) return demoConversations;
        return this.conversations.filter((item) => this.selectedSessionId
          ? item.session_id === this.selectedSessionId
          : !item.session_id);
      },
      vocabulary() {
        return this.demoMode ? [
          { label: '你好', exposures: 26, accuracy: 0.91 }, { label: '苹果', exposures: 21, accuracy: 0.82 }, { label: '谢谢', exposures: 18, accuracy: 0.88 },
          { label: '我', exposures: 15, accuracy: 0.95 }, { label: '喜欢', exposures: 12, accuracy: 0.77 }, { label: '再见', exposures: 9, accuracy: 0.86 },
        ] : this.analytics.vocabulary || [];
      },
      followReadByPhrase() {
        return this.demoMode ? [
          { label: '苹果', attempts: 8, successes: 7, success_rate: 0.875 },
          { label: '我喜欢苹果', attempts: 6, successes: 5, success_rate: 0.833 },
          { label: '你好', attempts: 7, successes: 6, success_rate: 0.857 },
        ] : this.analytics.follow_read_by_phrase || [];
      },
      finishedSessions() {
        return this.sessions.filter((session) => session.status === 'finished');
      },
      observerRunning() {
        return !!(this.observerConfig.enabled && this.connected);
      },
    },
    mounted() {
      this.connect();
      this.refreshAll();
      this.fetchObserverStatus().catch(() => {});
      this.$nextTick(() => {
        if (window.echarts) {
          this.charts.progress = echarts.init(document.getElementById('progressChart'));
          this.charts.vocabulary = echarts.init(document.getElementById('vocabularyChart'));
          this.charts.followRead = echarts.init(document.getElementById('followReadChart'));
          this.renderCharts();
          this.handleResize = () => this.resizeCharts();
          window.addEventListener('resize', this.handleResize);
        }
      });
    },
    beforeUnmount() {
      if (this.source) this.source.close();
      if (this.socket) this.socket.close();
      clearTimeout(this.reconnectTimer);
      clearTimeout(this.refreshTimer);
      if (this.handleResize) window.removeEventListener('resize', this.handleResize);
      Object.values(this.charts).forEach((chart) => chart.dispose());
    },
    watch: {
      demoMode() {
        this.renderCharts();
      },
      analytics: {
        deep: true,
        handler() { this.renderCharts(); },
      },
    },
    methods: {
      async refreshAll() {
        if (this.demoMode) return;
        await Promise.allSettled([this.fetchStats(), this.fetchAnalytics(), this.fetchSessions(), this.fetchReports(), this.fetchAssistantStatus(), this.fetchObserverStatus()]);
      },
      async fetchStats() {
        const response = await fetch(`${this.apiBase}/api/learning/stats`);
        if (!response.ok) throw new Error(`stats ${response.status}`);
        this.stats = await response.json();
      },
      async fetchAnalytics() {
        const response = await fetch(`${this.apiBase}/api/learning/analytics`);
        if (!response.ok) throw new Error(`analytics ${response.status}`);
        this.analytics = await response.json();
      },
      async fetchSessions() {
        const response = await fetch(`${this.apiBase}/api/learning/sessions?limit=30`);
        if (!response.ok) throw new Error(`sessions ${response.status}`);
        this.sessions = await response.json();
        if (!this.selectedSessionId) this.selectedSessionId = this.finishedSessions[0]?.id || '';
      },
      async fetchReports() {
        const response = await fetch(`${this.apiBase}/api/learning/reports`);
        if (!response.ok) throw new Error(`reports ${response.status}`);
        this.reports = await response.json();
        if (!this.currentReport && this.reports.length) this.currentReport = this.reports[0];
      },
      async fetchAssistantStatus() {
        const response = await fetch(`${this.apiBase}/api/assistant/status`);
        if (!response.ok) throw new Error(`assistant status ${response.status}`);
        const status = await response.json();
        this.assistantConfigured = !!(status.chat_configured ?? status.configured);
        this.reportConfigured = !!(status.report_configured ?? status.configured);
        this.assistantModel = status.model || '';
      },
      async fetchObserverStatus() {
        const response = await fetch(`${this.apiBase}/api/agent/observer`);
        if (!response.ok) throw new Error(`observer ${response.status}`);
        this.observerConfig = await response.json();
        this.observerDraftInterval = this.observerConfig.interval_seconds;
        if (!this.observerLogs.length && this.observerConfig.last_insight) {
          this.observerLogs = [this.observerConfig.last_insight];
        }
      },
      async updateObserver(patch) {
        try {
          const response = await fetch(`${this.apiBase}/api/agent/observer`, {
            method: 'POST', headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(patch),
          });
          if (!response.ok) throw new Error(`observer ${response.status}`);
          this.observerConfig = await response.json();
          this.observerMessage = '已应用';
        } catch (error) {
          this.observerMessage = error.message || '设置失败';
        }
      },
      toggleObserver() {
        this.updateObserver({ enabled: !this.observerConfig.enabled });
      },
      toggleObserverLlm() {
        this.updateObserver({ llm_enabled: !this.observerConfig.llm_enabled });
      },
      applyObserverInterval() {
        this.updateObserver({ interval_seconds: Number(this.observerDraftInterval) });
      },
      async triggerObserverScan() {
        if (this.observerScanning) return;
        this.observerScanning = true;
        this.observerMessage = '';
        try {
          const response = await fetch(`${this.apiBase}/api/agent/observer/tick`, { method: 'POST' });
          const result = await response.json();
          if (!response.ok) throw new Error(result.error || '扫描失败');
          if (result.record) this.pushObserverLog(result.record);
          await this.fetchObserverStatus();
          this.observerMessage = result.emitted ? '已产出新观察' : '本次扫描无新增练习数据';
        } catch (error) {
          this.observerMessage = error.message || '扫描失败';
        } finally {
          this.observerScanning = false;
        }
      },
      pushObserverLog(record) {
        if (!record || !record.ts) return;
        if (this.observerLogs.some((row) => row.ts === record.ts && row.decision === record.decision)) return;
        this.observerLogs.unshift(record);
        this.observerLogs = this.observerLogs.slice(0, 50);
      },
      formatSession(session) {
        const at = session.ended_at || session.started_at;
        const when = at ? new Date(Number(at) * 1000).toLocaleString('zh-CN', { month: '2-digit', day: '2-digit', hour: '2-digit', minute: '2-digit' }) : '时间未知';
        return `${when} · ${session.mode === 'pinyin' ? '拼音' : '手语'}${session.expected_text ? ` · ${session.expected_text}` : ''}`;
      },
      formatReport(report) {
        const at = report.ended_at || report.created_at;
        const when = at ? new Date(Number(at) * 1000).toLocaleString('zh-CN', { month: '2-digit', day: '2-digit', hour: '2-digit', minute: '2-digit' }) : '学习报告';
        return `${when} · ${report.mode === 'pinyin' ? '拼音' : '手语'}`;
      },
      async generateReport() {
        if (!this.selectedSessionId || this.demoMode) return;
        this.reportGenerating = true;
        this.reportError = '';
        try {
          const response = await fetch(`${this.apiBase}/api/learning/report`, {
            method: 'POST', headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ session_id: this.selectedSessionId }),
          });
          const result = await response.json();
          if (!response.ok) throw new Error(result.error || '报告生成失败');
          this.currentReport = result;
          await this.fetchReports();
        } catch (error) {
          this.reportError = error.message || '报告服务暂不可用';
        } finally {
          this.reportGenerating = false;
        }
      },
      async sendAssistantMessage() {
        const text = this.draftMessage.trim();
        if (!text || !this.assistantConfigured || this.chatSending || this.demoMode) return;
        this.chatSending = true;
        this.assistantMessage = '';
        try {
          const response = await fetch(`${this.apiBase}/api/assistant/chat`, {
            method: 'POST', headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ text, session_id: this.selectedSessionId }),
          });
          const result = await response.json();
          if (!response.ok) throw new Error(result.error || '助手暂不可用');
          this.addConversation('learner', result.learner_message);
          this.addConversation('assistant', result.assistant_message);
          this.draftMessage = '';
        } catch (error) {
          this.assistantMessage = error.message || '助手暂不可用';
        } finally {
          this.chatSending = false;
        }
      },
      async onSettingsToggle(event) {
        if (!event.target.open || this.settingsLoaded) return;
        try {
          const response = await fetch(`${this.apiBase}/api/thresholds`);
          if (!response.ok) throw new Error('无法读取设备参数');
          const saved = await response.json();
          this.thresholds = { ...this.thresholds, ...saved };
          this.wordThresholds = Object.fromEntries(this.gestureLabels.map((label) => [label, saved.word_thresholds?.[label] ?? 0.6]));
          this.letterThresholds = Object.fromEntries(this.pinyinLabels.map((label) => [label, saved.letter_thresholds?.[label] ?? 0.6]));
          this.settingsLoaded = true;
          this.settingsMessage = '已读取后台保存的设备参数';
        } catch (_) {
          this.settingsMessage = '读取失败；请确认主站后台正在运行';
        }
      },
      async sendCommand(command, params = {}) {
        try {
          const response = await fetch(`${this.apiBase}/api/cmd`, {
            method: 'POST', headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ cmd: command, params }),
          });
          const result = await response.json();
          if (!response.ok) throw new Error(result.error || '设置失败');
          this.settingsMessage = result.status === 'sent' ? '设备已收到设置' : '设备离线，设置已保存';
          return true;
        } catch (error) {
          this.settingsMessage = error.message || '后台连接失败';
          return false;
        }
      },
      async applySetting(setting) {
        await this.sendCommand(setting.command, { value: Number(this.thresholds[setting.key]) });
      },
      async applyLabelThreshold(command, label, value) {
        await this.sendCommand(command, { label, value: Number(value) });
      },
      async resetThresholds(command) {
        const sent = await this.sendCommand(command, {});
        if (sent) {
          this.settingsLoaded = false;
          await this.onSettingsToggle({ target: { open: true } });
        }
      },
      connect() {
        const socketUrl = new URLSearchParams(location.search).get('ws');
        if (socketUrl) this.connectWebSocket(socketUrl);
        else this.connectSse();
      },
      connectWebSocket(url) {
        try {
          this.socket = new WebSocket(url);
          this.socket.onopen = () => { this.connected = true; this.streamMode = 'ws'; };
          this.socket.onmessage = (event) => {
            try { this.handleEnvelope(JSON.parse(event.data)); } catch (_) { /* ignore malformed frames */ }
          };
          this.socket.onerror = () => this.socket.close();
          this.socket.onclose = () => {
            this.connected = false;
            if (!this.source) this.connectSse();
          };
        } catch (_) {
          this.connectSse();
        }
      },
      connectSse() {
        if (this.source) return;
        this.streamMode = 'sse';
        this.source = new EventSource(`${this.apiBase}/api/stream`);
        this.source.onopen = () => { this.connected = true; };
        this.source.onmessage = (event) => {
          try { this.handleEnvelope(JSON.parse(event.data)); } catch (_) { /* ignore malformed frames */ }
        };
        this.source.onerror = () => {
          this.connected = false;
        };
      },
      handleEnvelope(envelope) {
        if (!envelope || !envelope.type) return;
        if (envelope.type === 'init') {
          const initial = envelope;
          if (initial.status) this.updateStatus(initial.status);
          this.observerLogs = (initial.agent_logs || []).slice(-50).reverse();
          this.conversations = [];
          const messages = (initial.conversation_messages?.length ? initial.conversation_messages : [
            ...(initial.asr_messages || []).map((item) => ({ ...item, role: 'learner' })),
            ...(initial.sentences || []).map((item) => ({ ...item, role: 'assistant' })),
          ]).slice(-100).sort((a, b) => Number(a.ts || 0) - Number(b.ts || 0));
          messages.forEach((item) => this.addConversation(item.role, item));
          this.$nextTick(this.scrollConversation);
          return;
        }
        if (envelope.type === 'status') this.updateStatus(envelope.data || {});
        if (envelope.type === 'prediction' && envelope.data) {
          this.predictions.unshift(envelope.data);
          this.predictions = this.predictions.slice(0, 100);
          clearTimeout(this.refreshTimer);
          this.refreshTimer = setTimeout(() => {
            this.fetchStats().catch(() => {});
            this.fetchAnalytics().catch(() => {});
          }, 1200);
        }
        if (envelope.type === 'sentence' && envelope.data) this.addConversation('assistant', envelope.data);
        if (envelope.type === 'asr' && envelope.data) this.addConversation('learner', envelope.data);
        if (envelope.type === 'conversation' && envelope.data) this.addConversation(envelope.data.role, envelope.data);
        if (envelope.type === 'follow_read' && envelope.data) {
          clearTimeout(this.refreshTimer);
          this.refreshTimer = setTimeout(() => this.fetchAnalytics().catch(() => {}), 300);
        }
        if (envelope.type === 'agent_log' && envelope.data) this.pushObserverLog(envelope.data);
        if (envelope.type === 'learning_finished') {
          this.fetchSessions().catch(() => {});
          this.fetchReports().catch(() => {});
        }
      },
      updateStatus(status) {
        this.connected = !!status.online;
        this.deviceVersion = status.version || '';
        this.lastSeen = status.last_seen || '';
      },
      addConversation(role, record) {
        if (role === 'patient') role = 'learner';
        if (!['learner', 'assistant'].includes(role)) role = 'learner';
        const text = record.text || record.current_sentence || '';
        if (!text) return;
        const identity = `${role}|${record.ts || record.time || ''}|${text}`;
        if (this.conversations.some((item) => item.id === identity)) return;
        this.conversations.push({ id: identity, role, text, time: record.time || this.formatClock(record.ts), session_id: record.session_id || '' });
        this.conversations = this.conversations.slice(-60);
        this.$nextTick(this.scrollConversation);
      },
      formatClock(timestamp) {
        if (!timestamp) return '--:--:--';
        return new Date(Number(timestamp) * 1000).toLocaleTimeString('zh-CN', { hour12: false });
      },
      phaseClass(phase) {
        const value = String(phase || '').toLowerCase();
        if (value.includes('感知') || value.includes('perception')) return 'perception';
        if (value.includes('分析') || value.includes('analysis')) return 'analysis';
        if (value.includes('决策') || value.includes('decision')) return 'decision';
        if (value.includes('反馈') || value.includes('feedback')) return 'feedback';
        return 'other';
      },
      scrollConversation() {
        const element = this.$refs.conversationList;
        if (element) element.scrollTop = element.scrollHeight;
      },
      toggleDemo() {
        this.demoMode = !this.demoMode;
        if (!this.demoMode) this.refreshAll();
        this.renderCharts();
      },
      applyApi() {
        const candidate = this.apiDraft.trim().replace(/\/$/, '');
        try {
          const url = new URL(candidate);
          if (!['http:', 'https:'].includes(url.protocol)) throw new Error('协议无效');
          this.apiBase = candidate;
          localStorage.setItem('gloveBApiBase', candidate);
          if (this.source) this.source.close();
          this.source = null;
          this.connected = false;
          this.connectSse();
          this.refreshAll();
        } catch (_) {
          window.alert('请输入有效的 HTTP 或 HTTPS 地址，例如 http://localhost:5000');
        }
      },
      async copyText(text) {
        try { await navigator.clipboard.writeText(text); } catch (_) { window.prompt('复制接口地址', text); }
      },
      renderCharts() {
        if (!window.echarts || !this.charts.progress || !this.charts.vocabulary) return;
        const realDays = this.analytics.daily || [];
        const days = this.demoMode ? [
          { date: '09-26', sessions: 2, sentences: 5 }, { date: '09-27', sessions: 3, sentences: 8 },
          { date: '09-28', sessions: 2, sentences: 6 }, { date: '09-29', sessions: 4, sentences: 11 },
          { date: '09-30', sessions: 3, sentences: 9 }, { date: '10-01', sessions: 5, sentences: 13 },
          { date: '10-02', sessions: 6, sentences: 17 },
        ] : realDays.map((item) => ({ ...item, date: item.date.slice(5) }));
        const axisStyle = { color: '#857357', fontSize: 9 };
        this.charts.progress.setOption({
          animationDuration: 550,
          grid: { left: 28, right: 13, top: 25, bottom: 25 },
          tooltip: { trigger: 'axis', backgroundColor: '#FFFCF4', borderColor: '#E4D6BB', textStyle: { color: '#463823', fontSize: 9 } },
          legend: { show: false },
          xAxis: { type: 'category', data: days.map((item) => item.date), axisLine: { lineStyle: { color: '#E4D6BB' } }, axisTick: { show: false }, axisLabel: axisStyle },
          yAxis: { type: 'value', minInterval: 1, splitLine: { lineStyle: { color: '#EDE1CB', type: 'dashed' } }, axisLabel: axisStyle },
          series: [
            { name: '学习次数', type: 'bar', data: days.map((item) => item.sessions), barMaxWidth: 14, itemStyle: { color: '#53442C', borderRadius: [4, 4, 0, 0] } },
            { name: '完成句子', type: 'line', smooth: true, symbolSize: 5, data: days.map((item) => item.sentences), lineStyle: { width: 2, color: '#F57E65' }, itemStyle: { color: '#F57E65' }, areaStyle: { color: 'rgba(245,126,101,.10)' } },
          ],
        }, true);
        const vocabulary = this.vocabulary.slice(0, 7);
        this.charts.vocabulary.setOption({
          animationDuration: 550,
          grid: { left: 45, right: 12, top: 16, bottom: 22 },
          tooltip: { trigger: 'axis', axisPointer: { type: 'shadow' }, backgroundColor: '#FFFCF4', borderColor: '#E4D6BB', textStyle: { color: '#463823', fontSize: 9 } },
          xAxis: { type: 'value', minInterval: 1, splitLine: { lineStyle: { color: '#EDE1CB', type: 'dashed' } }, axisLabel: axisStyle },
          yAxis: { type: 'category', inverse: true, data: vocabulary.map((item) => item.label), axisLine: { show: false }, axisTick: { show: false }, axisLabel: { ...axisStyle, color: '#53442C' } },
          series: [{ name: '识别次数', type: 'bar', data: vocabulary.map((item) => item.exposures), barMaxWidth: 12, itemStyle: { color: '#7C6B52', borderRadius: [0, 4, 4, 0] } }],
        }, true);
        const followReadRows = this.followReadByPhrase.slice(0, 7);
        this.charts.followRead.setOption({
          animationDuration: 450,
          grid: { left: 42, right: 18, top: 7, bottom: 18 },
          tooltip: { trigger: 'axis', axisPointer: { type: 'shadow' }, backgroundColor: '#FFFCF4', borderColor: '#E4D6BB', textStyle: { color: '#463823', fontSize: 9 }, formatter: (items) => `${items[0].name} · 转写成功率 ${items[0].value}%` },
          xAxis: { type: 'value', min: 0, max: 100, axisLabel: { ...axisStyle, formatter: '{value}%' }, splitLine: { lineStyle: { color: '#EDE1CB', type: 'dashed' } } },
          yAxis: { type: 'category', inverse: true, data: followReadRows.map((item) => item.label), axisLine: { show: false }, axisTick: { show: false }, axisLabel: { ...axisStyle, color: '#53442C' } },
          series: [{ name: '转写成功率', type: 'bar', data: followReadRows.map((item) => Math.round((item.success_rate ?? 0) * 100)), barMaxWidth: 9, itemStyle: { color: '#F57E65', borderRadius: [0, 4, 4, 0] } }],
        }, true);
      },
      resizeCharts() {
        Object.values(this.charts).forEach((chart) => chart.resize());
      },
    },
  }).mount('#app');
})();
