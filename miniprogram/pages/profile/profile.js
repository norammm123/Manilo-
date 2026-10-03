// pages/profile/profile.js - 个人中心
Page({
  data: {},

  goReport() {
    wx.navigateTo({ url: '/pages/report/report' })
  },

  goBadge() {
    wx.navigateTo({ url: '/pages/badge/badge' })
  },

  goSettings() {
    wx.navigateTo({ url: '/pages/settings/settings' })
  },

  goHelp() {
    wx.navigateTo({ url: '/pages/help/help' })
  },

  goFeedback() {
    wx.navigateTo({ url: '/pages/feedback/feedback' })
  }
})
