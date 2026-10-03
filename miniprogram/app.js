// app.js - Manilo掌音 全局应用
App({
  globalData: {
    // 默认地址: 手机热点下手套PC的IP; 运行时可在「设置」页修改并持久化
    baseUrl: 'http://10.175.178.245:5000',
    userInfo: null,
    systemInfo: {},
    theme: 'light'
  },

  onLaunch() {
    this.globalData.systemInfo = wx.getSystemInfoSync()

    // 优先使用设置页保存的服务器地址(运行时配置, 改网络不用改代码)
    const saved = wx.getStorageSync('serverUrl')
    if (saved) {
      this.globalData.baseUrl = saved
    }

    wx.onThemeChange((res) => {
      this.globalData.theme = res.theme
    })
  },

  // 更新服务器地址(设置页调用)
  setServerUrl(url) {
    const baseUrl = url.replace(/\/+$/, '') // 去掉末尾斜杠
    this.globalData.baseUrl = baseUrl
    wx.setStorageSync('serverUrl', baseUrl)
  }
})
