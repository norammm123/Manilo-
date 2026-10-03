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

  // 开始轮询后端状态
  startPolling() {
    this.pollStats()
    this.timer = setInterval(() => {
      this.pollStats()
    }, 3000)
  },

  stopPolling() {
    if (this.timer) {
      clearInterval(this.timer)
      this.timer = null
    }
  },

  // 轮询获取后端状态
  async pollStats() {
    try {
      const stats = await api.getStats()
      this.setData({ 
        backendConnected: stats.online,
        stats: stats
      })
      if (stats.online && stats.sentences_today > 0) {
        this.loadRecentFromBackend()
      }
    } catch (e) {
      this.setData({ backendConnected: false })
    }
  },

  // 手动检查后端
  async checkBackend() {
    wx.showLoading({ title: '检查后端...' })
    try {
      await this.pollStats()
      wx.showToast({ 
        title: this.data.backendConnected ? '✅ 后端在线' : '❌ 设备离线', 
        icon: 'none' 
      })
    } catch (err) {
      wx.showToast({ title: '❌ 无法连接后端', icon: 'none' })
    }
    wx.hideLoading()
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
