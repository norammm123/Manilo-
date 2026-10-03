// pages/device/device.js - 设备页面
const app = getApp()

Page({
  data: {
    bleConnected: false,
    deviceInfo: null,
    threshold: 0.6,
    cooldown: 0.8,
    vibrationEnabled: true,
    battery: 85
  },

  onLoad() {
    this.setData({
      bleConnected: app.globalData.bleConnected
    })
  },

  // 蓝牙连接
  async toggleConnect() {
    const ble = app.getBLE()
    
    if (this.data.bleConnected) {
      ble.disconnect()
      this.setData({ bleConnected: false })
    } else {
      wx.showLoading({ title: '连接中...' })
      try {
        await ble.scanAndConnect()
        this.setData({ bleConnected: true })
        wx.showToast({ title: '连接成功', icon: 'success' })
      } catch (err) {
        wx.showToast({ title: '连接失败', icon: 'none' })
      }
      wx.hideLoading()
    }
  },

  // 阈值调整
  onThresholdChange(e) {
    const val = e.detail.value / 100
    this.setData({ threshold: val })
    this.sendCommand('set_threshold', { value: val })
  },

  // 冷却调整
  onCooldownChange(e) {
    const val = e.detail.value / 10
    this.setData({ cooldown: val })
    this.sendCommand('set_cooldown', { value: val })
  },

  // 开关震动
  toggleVibration(e) {
    this.setData({ vibrationEnabled: e.detail.value })
  },

  // 测试震动
  testVibrate(e) {
    const type = e.currentTarget.dataset.type
    this.sendCommand('vibrate', { 
      pattern: type, 
      level: 0.6, 
      duration_ms: type === 'long' ? 500 : 300 
    })
    wx.showToast({ title: '已发送震动', icon: 'none' })
  },

  // 发送命令
  sendCommand(cmd, params) {
    const ble = app.getBLE()
    if (ble.connected) {
      ble.send(Object.assign({ cmd: cmd }, params))
    }
  }
})
