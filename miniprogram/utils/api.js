// utils/api.js - 按 MINIPROGRAM_API2.md 接口封装
const app = getApp()

function request(method, path, data, timeout) {
  return new Promise((resolve, reject) => {
    wx.request({
      url: app.globalData.baseUrl + path,
      method,
      data,
      header: { 'content-type': 'application/json' },
      timeout: timeout || 10000,
      success: (res) => {
        if (res.statusCode >= 200 && res.statusCode < 300) resolve(res.data)
        else reject({ statusCode: res.statusCode, body: res.data })
      },
      fail: reject
    })
  })
}

module.exports = {
  // ===== 实时状态 =====
  getStats: () => request('GET', '/api/stats'),
  getHistory: (n) => request('GET', '/api/history?n=' + (n || 50)),

  // ===== 学习会话 =====
  startLearning: (mode, expectedText) =>
    request('POST', '/api/learning/start', { mode: mode || 'gesture', expected_text: expectedText || '' }),
  getSession: (sessionId) =>
    request('GET', '/api/learning/session?session_id=' + sessionId),
  getSessions: (limit) =>
    request('GET', '/api/learning/sessions?limit=' + (limit || 30)),
  finishLearning: (sessionId) =>
    request('POST', '/api/learning/finish', { session_id: sessionId }, 60000),
  regenReport: (sessionId) =>
    request('POST', '/api/learning/report', { session_id: sessionId }, 60000),
  getReports: () => request('GET', '/api/learning/reports'),

  // ===== 学习数据 =====
  getLearningStats: () => request('GET', '/api/learning/stats'),
  getAnalytics: () => request('GET', '/api/learning/analytics'),

  // ===== 上报 =====
  followRead: (data) => request('POST', '/api/learning/follow-read', data),
  addConversation: (role, text, sessionId) =>
    request('POST', '/api/conversation', { role: role || 'learner', text: text, session_id: sessionId || '' }),
  addAgentLog: (data) => request('POST', '/api/agent/log', data),

  // ===== AI 助手 =====
  getAssistantStatus: () => request('GET', '/api/assistant/status'),
  chat: (text, sessionId) =>
    request('POST', '/api/assistant/chat', { text: text, session_id: sessionId || '' }, 90000),

  // ===== 日志/设备 =====
  getAgentLogs: () => request('GET', '/api/agent/logs'),
  getObserver: () => request('GET', '/api/agent/observer'),
  getThresholds: () => request('GET', '/api/thresholds'),
  sendCmd: (cmd, params) =>
    request('POST', '/api/cmd', { cmd: cmd, params: params || {} })
}
