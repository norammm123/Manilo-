// pages/profile-edit/profile-edit.js - 完善资料
const app = getApp()

Page({
  data: {
    avatarPath: '',
    nickname: '',
    saving: false
  },

  onLoad() {
    // 预填已保存的信息
    const userInfo = wx.getStorageSync('userInfo') || {}
    this.setData({
      avatarPath: userInfo.avatar || '',
      nickname: userInfo.nickname || ''
    })
  },

  // 选择微信头像
  onChooseAvatar(e) {
    const tempPath = e.detail.avatarUrl
    // 临时路径存不住, 保存到本地文件
    wx.saveFile({
      tempFilePath: tempPath,
      success: (res) => {
        this.setData({ avatarPath: res.savedFilePath })
      },
      fail: () => {
        this.setData({ avatarPath: tempPath })
      }
    })
  },

  // 输入昵称(微信键盘会提示一键填入微信昵称)
  onNickname(e) {
    this.setData({ nickname: e.detail.value })
  },

  save() {
    if (this.data.saving) return
    const nickname = this.data.nickname.trim() || '微信用户'
    const userInfo = wx.getStorageSync('userInfo') || {}
    userInfo.nickname = nickname
    if (this.data.avatarPath) {
      userInfo.avatar = this.data.avatarPath
    }
    wx.setStorageSync('userInfo', userInfo)

    this.setData({ saving: true })
    wx.showToast({ title: '保存成功', icon: 'success' })
    setTimeout(() => {
      wx.switchTab({ url: '/pages/index/index' })
    }, 800)
  },

  skip() {
    wx.switchTab({ url: '/pages/index/index' })
  }
})
