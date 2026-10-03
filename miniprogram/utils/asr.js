// utils/asr.js - 语音识别: 录音文件上传后端识别(需后端提供音频上传接口)
// 后端需提供: POST /api/asr/upload (multipart, 字段 audio) -> { text: "识别文字" }
// 后端如路径不同, 改 UPLOAD_PATH 即可

const UPLOAD_PATH = '/api/asr/upload'

// 上传录音文件给后端, 返回识别文本
function recognizeByUpload(filePath) {
  const app = getApp()
  return new Promise((resolve, reject) => {
    wx.uploadFile({
      url: app.globalData.baseUrl + UPLOAD_PATH,
      filePath: filePath,
      name: 'audio',
      timeout: 30000,
      success: (res) => {
        try {
          const data = JSON.parse(res.data)
          const text = (data && (data.text || data.transcription || data.result)) || ''
          resolve(text)
        } catch (e) {
          reject(new Error('后端返回格式错误: ' + String(res.data).slice(0, 100)))
        }
      },
      fail: (err) => reject(err)
    })
  })
}

// 兼容旧接口(插件模式已废弃, 统一走上传)
function available() {
  return true
}

function start() {
  return null
}

function stop() {}

module.exports = {
  available: available,
  recognizeByUpload: recognizeByUpload,
  start: start,
  stop: stop
}
