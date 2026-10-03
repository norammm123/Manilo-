// pages/login/login.js - 登录页
const app = getApp()

Page({
  data: {
    wxLogging: false
  },

  onLoad() {
    // 已登录直接进首页
    if (wx.getStorageSync('isLogin')) {
      this.enterApp()
    }
  },

  // 微信一键登录
  wechatLogin() {
    if (this.data.wxLogging) return
    this.setData({ wxLogging: true })

    wx.login({
      success: (res) => {
        // code 可用于后端换 openid(对接后端后再实现)
        wx.setStorageSync('wxCode', res.code)
        this.finishLogin({
          nickname: '微信用户',
          loginType: 'wechat',
          code: res.code
        }, true) // 微信用户 → 去完善资料(填昵称/头像)
      },
      fail: () => {
        this.setData({ wxLogging: false })
        wx.showToast({ title: '登录失败，请重试', icon: 'none' })
      }
    })
  },

  // 游客进入
  guestLogin() {
    this.finishLogin({
      nickname: '游客',
      loginType: 'guest',
      code: ''
    }, false) // 游客 → 直接进首页
  },

  finishLogin(userInfo, needProfile) {
    wx.setStorageSync('userInfo', userInfo)
    wx.setStorageSync('isLogin', true)
    if (needProfile) {
      wx.redirectTo({ url: '/pages/profile-edit/profile-edit' })
    } else {
      this.enterApp()
    }
  },

  enterApp() {
    wx.switchTab({ url: '/pages/index/index' })
  }
})
