// pages/history/history.js - 历史记录
const api = require('../../utils/api.js')

Page({
  data: {
    stats: null,
    records: [],
    loading: true,
    error: ''
  },

  onLoad() {
    this.loadData()
  },

  async loadData() {
    this.setData({ loading: true, error: '' })
    try {
      const [stats, history] = await Promise.all([
        api.getStats(),
        api.getHistory(50)
      ])
      const records = (history || []).map(r => Object.assign({}, r, {
        date_text: this.formatDate(r.ts)
      })).reverse() // 最新的在上面
      // 置信度预格式化(wxml 不支持方法调用)
      if (stats) {
        stats.confStr = stats.avg_confidence != null
          ? (stats.avg_confidence * 100).toFixed(1)
          : '-'
      }
      this.setData({ stats, records, loading: false })
    } catch (e) {
      console.error('历史加载失败:', e)
      this.setData({
        loading: false,
        error: '无法连接服务器，请在「设置」页检查服务器地址'
      })
    }
  },

  formatDate(ts) {
    if (!ts) return ''
    const d = new Date(ts * 1000)
    const pad = n => (n < 10 ? '0' + n : n)
    return `${pad(d.getMonth() + 1)}-${pad(d.getDate())}`
  },

  onPullDownRefresh() {
    this.loadData().finally(() => wx.stopPullDownRefresh())
  }
})
