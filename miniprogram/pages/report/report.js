// pages/report/report.js - 训练报告
const api = require('../../utils/api.js')

Page({
  data: {
    stats: null,
    reports: [],
    loading: true,
    error: ''
  },

  onLoad() {
    this.loadData()
  },

  async loadData() {
    this.setData({ loading: true, error: '' })
    try {
      const [stats, reports] = await Promise.all([
        api.getLearningStats(),
        api.getReports()
      ])
      console.log('训练报告数据:', stats, reports)
      const list = (reports || []).map(r => Object.assign({}, r, {
        created_at_text: this.formatTime(r.created_at),
        metrics_text: this.metricsText(r.metrics || {})
      }))
      this.setData({ stats, reports: list, loading: false })
    } catch (e) {
      console.error('训练报告加载失败:', e)
      this.setData({ loading: false, error: '加载失败：' + (e.statusCode ? ('HTTP ' + e.statusCode) : '无法连接服务器，请在设置页检查服务器地址') })
    }
  },

  // 汇总 metrics 为一行文字
  metricsText(m) {
    const parts = []
    if (m.distinct_labels) parts.push('识别 ' + m.distinct_labels + ' 种手势')
    if (m.duration_seconds) parts.push('时长 ' + Math.round(m.duration_seconds) + ' 秒')
    if (m.follow_read_attempts) parts.push('跟读 ' + m.follow_read_attempts + ' 次')
    if (m.avg_confidence != null) parts.push('置信度 ' + Math.round(m.avg_confidence * 100) + '%')
    return parts.join(' · ') || '本次练习'
  },

  formatTime(ts) {
    if (!ts) return ''
    const d = new Date(ts * 1000)
    const pad = n => (n < 10 ? '0' + n : n)
    return `${d.getFullYear()}-${pad(d.getMonth() + 1)}-${pad(d.getDate())} ${pad(d.getHours())}:${pad(d.getMinutes())}`
  },

  viewReport(e) {
    const id = e.currentTarget.dataset.id
    wx.showToast({ title: '报告详情开发中', icon: 'none' })
  },

  onPullDownRefresh() {
    this.loadData().finally(() => wx.stopPullDownRefresh())
  }
})
