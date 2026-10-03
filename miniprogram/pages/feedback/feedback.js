// pages/feedback/feedback.js - 反馈建议
const api = require('../../utils/api.js')

Page({
  data: {
    content: '',
    sending: false
  },

  onInput(e) {
    this.setData({ content: e.detail.value })
  },

  submit() {
    const text = this.data.content.trim()
    if (!text) {
      wx.showToast({ title: '请先输入内容', icon: 'none' })
      return
    }
    if (text.length < 5) {
      wx.showToast({ title: '再写详细一点吧', icon: 'none' })
      return
    }

    this.setData({ sending: true })
    wx.showLoading({ title: '提交中...' })

    api.addConversation('learner', '【反馈】' + text)
      .then(() => {
        wx.hideLoading()
        this.setData({ sending: false, content: '' })
        wx.showToast({ title: '感谢反馈！', icon: 'success' })
      })
      .catch((err) => {
        wx.hideLoading()
        this.setData({ sending: false })
        wx.showToast({ title: '提交失败，后端未连接', icon: 'none' })
        console.error('反馈失败:', err)
      })
  }
})
