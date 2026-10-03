// pages/settings/settings.js - 通用设置
const app = getApp()

Page({
  data: {
    serverUrl: '',
    saving: false,
    serverStatus: '',
    darkMode: false,
    vibrateOn: true,
    speakOn: true
  },

  onLoad() {
    const saved = wx.getStorageSync('serverUrl')
    this.setData({
      serverUrl: saved || app.globalData.baseUrl,
      darkMode: wx.getStorageSync('darkMode') || false,
      vibrateOn: wx.getStorageSync('vibrateOn') !== false,
      speakOn: wx.getStorageSync('speakOn') !== false
    })
  },

  // 输入服务器地址
  onServerInput(e) {
    this.setData({ serverUrl: e.detail.value, serverStatus: '' })
  },

  // 保存服务器地址并测试连通
  saveServer() {
    let url = (this.data.serverUrl || '').trim()
    if (!url) {
      wx.showToast({ title: '请输入服务器地址', icon: 'none' })
      return
    }
    // 补全 http:// 前缀
    if (!/^https?:\/\//i.test(url)) {
      url = 'http://' + url
    }
    url = url.replace(/\/+$/, '')

    this.setData({ saving: true, serverStatus: '测试连接中...' })

    // 先测通再保存
    wx.request({
      url: url + '/api/stats',
      timeout: 8000,
      success: (res) => {
        app.setServerUrl(url)
        this.setData({
          saving: false,
          serverStatus: '✅ 已保存并连接成功',
          serverUrl: url
        })
        wx.showToast({ title: '已保存', icon: 'success' })
      },
      fail: () => {
        // 连接失败也允许保存(可能手套PC没开机, 但下次开机就能用)
        app.setServerUrl(url)
        this.setData({
          saving: false,
          serverStatus: '⚠️ 保存成功，但当前连接不通（检查手套PC/网络）'
        })
        wx.showToast({ title: '已保存（未连通）', icon: 'none' })
      }
    })
  },

  toggleDark(e) {
    const v = e.detail.value
    wx.setStorageSync('darkMode', v)
    this.setData({ darkMode: v })
  },

  toggleVibrate(e) {
    const v = e.detail.value
    wx.setStorageSync('vibrateOn', v)
    this.setData({ vibrateOn: v })
  },

  toggleSpeak(e) {
    const v = e.detail.value
    wx.setStorageSync('speakOn', v)
    this.setData({ speakOn: v })
  },

  clearCache() {
    wx.showModal({
      title: '清除缓存',
      content: '将清除本地缓存数据（不影响后端训练记录与已保存的服务器地址）',
      success: (res) => {
        if (res.confirm) {
          // 保留服务器地址，防止清缓存后连不上
          const serverUrl = wx.getStorageSync('serverUrl')
          wx.clearStorageSync()
          if (serverUrl) wx.setStorageSync('serverUrl', serverUrl)
          wx.showToast({ title: '已清除', icon: 'success' })
          this.onLoad()
        }
      }
    })
  },

  about() {
    wx.showModal({
      title: 'Manilo掌音',
      content: '用科技连接声音，让沟通更简单。\n版本 v2.0.0',
      showCancel: false
    })
  }
})
