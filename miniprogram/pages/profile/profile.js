// pages/profile/profile.js - 个人中心
Page({
  data: {
    userInfo: {},
    userId: '',
    trainDays: 0,
    isLogin: false
  },

  onShow() {
    const userInfo = wx.getStorageSync('userInfo') || {}
    const isLogin = wx.getStorageSync('isLogin') || false
    this.setData({
      userInfo,
      isLogin,
      userId: (userInfo.code || 'MNL2024').toString().slice(-6).toUpperCase(),
      trainDays: this.calcDays()
    })
  },

  calcDays() {
    const start = wx.getStorageSync('startDate')
    if (start) {
      const days = Math.floor((Date.now() - start) / 86400000) + 1
      return Math.min(days, 99)
    }
    wx.setStorageSync('startDate', Date.now())
    return 1
  },

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
  },

  logout() {
    wx.showModal({
      title: '退出登录',
      content: '确定要退出当前账号吗？',
      success: (res) => {
        if (res.confirm) {
          wx.removeStorageSync('isLogin')
          wx.removeStorageSync('userInfo')
          wx.removeStorageSync('wxCode')
          wx.reLaunch({ url: '/pages/login/login' })
        }
      }
    })
  }
})
