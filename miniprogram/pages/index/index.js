// pages/index/index.js - Manilo掌音首页 (HTTP 轮询版)
const app = getApp()
const api = require('../../utils/api.js')

Page({
  data: {
    backendConnected: false,
    stats: null,
    today: '',
    recentList: []
  },

  onLoad() {
    this.setToday()
    this.loadRecent()
    this.startPolling()
  },

  onUnload() {
    this.stopPolling()
  },

  // 设置今日日期
  setToday() {
    const date = new Date()
    const month = date.getMonth() + 1
    const day = date.getDate()
    this.setData({
      today: `${month}月${day}日`
    })
  },

  // 开始轮询后端状态 (失败指数退避, 避免刷屏)
  startPolling() {
    this._pollDelay = 3000
    this._schedulePoll()
  },

  stopPolling() {
    if (this._pollTimer) {
      clearTimeout(this._pollTimer)
      this._pollTimer = null
    }
  },

  _schedulePoll() {
    if (!this._pollDelay) return
    this._pollTimer = setTimeout(() => this.pollStats(), this._pollDelay)
  },

  // 轮询获取后端状态
  async pollStats() {
    // 上次还在请求中就跳过
    if (this._polling) return
    this._polling = true
    try {
      const stats = await api.getStats()
      this._failCount = 0
      this._pollDelay = 3000
      this.setData({ 
        backendConnected: stats.online,
        stats: stats,
        netError: ''
      })
      if (stats.online && stats.sentences_today > 0) {
        this.loadRecentFromBackend()
      }
    } catch (e) {
      // 失败: 延迟翻倍 (3s -> 6s -> 12s -> 24s, 上限 30s)
      this._failCount = (this._failCount || 0) + 1
      this._pollDelay = Math.min(30000, this._pollDelay * 2)
      this.setData({
        backendConnected: false,
        netError: this._failCount >= 2 ? (e && e.errMsg ? e.errMsg : '无法连接服务器') : ''
      })
    } finally {
      this._polling = false
      this._schedulePoll()
    }
  },

  // 手动检查后端 (任何失败都弹窗提示)
  async checkBackend() {
    const api = require('../../utils/api.js')
    let res
    try {
      res = await api.getStats()
    } catch (err) {
      // 请求失败: 必弹窗
      const detail = (err && err.errMsg) ? err.errMsg : (err && err.message) || String(err)
      this.setData({ backendConnected: false })
      wx.showModal({
        title: '❌ 无法连接后端',
        content: this.domainHint(detail) + '\n\n地址: ' + app.globalData.baseUrl,
        showCancel: false,
        confirmText: '知道了'
      })
      return
    }

    this.setData({
      backendConnected: res.online,
      stats: res
    })
    if (res.online) {
      wx.showToast({ title: '✅ 后端在线', icon: 'none' })
    } else {
      wx.showModal({
        title: '⚠️ 后端可达但设备离线',
        content: '服务器能连上，但手套没有上报数据。\n请检查手套 PC 的 pc_inference 与蓝牙状态。',
        showCancel: false,
        confirmText: '知道了'
      })
    }
  },

  // 根据错误信息给出针对性的提示
  domainHint(detail) {
    const d = detail.toLowerCase()
    if (d.indexOf('domain') >= 0 || d.indexOf('url') >= 0) {
      return '请求被微信域名校验拦截：体验版只能用 HTTPS 合法域名。\n开发调试请用「预览 + 打开调试」，或把地址换成已配置的 HTTPS 域名。'
    }
    if (d.indexOf('timeout') >= 0) {
      return '连接超时：请确认手机与手套 PC 在同一网络，且后端已启动。'
    }
    if (d.indexOf('fail') >= 0) {
      return '请求失败：请检查服务器地址是否正确，以及防火墙是否放行 5000 端口。'
    }
    return '无法连接服务器：' + (detail || '未知错误')
  },

  // 加载最近识别
  async loadRecentFromBackend() {
    try {
      const history = await api.getHistory(10)
      const list = history.map(item => ({
        text: item.text,
        time: item.time,
        confidence: 0.9
      }))
      this.setData({ recentList: list })
    } catch (e) {
      // 使用模拟数据
      this.loadRecentMock()
    }
  },

  loadRecentMock() {
    this.setData({
      recentList: [
        { text: '你好，很高兴认识你', time: '10:28', confidence: 0.92 },
        { text: '今天天气不错', time: '10:25', confidence: 0.85 },
        { text: '我要喝水', time: '10:20', confidence: 0.94 },
        { text: '谢谢', time: '10:15', confidence: 0.88 }
      ]
    })
  },

  loadRecent() {
    this.loadRecentMock()
  },

  // 页面跳转
  goToTraining() {
    wx.switchTab({ url: '/pages/training/training' })
  },

  goToDevice() {
    wx.switchTab({ url: '/pages/device/device' })
  },

  goToHistory() {
    wx.navigateTo({ url: '/pages/history/history' })
  },

  showHistory() {
    wx.navigateTo({ url: '/pages/history/history' })
  },

  goToLexicon() {
    wx.navigateTo({ url: '/pages/lexicon/lexicon' })
  },

  showLexicon() {
    wx.navigateTo({ url: '/pages/lexicon/lexicon' })
  }
})
