// utils/tts.js - 语音合成(微信同声传译插件 textToSpeech)
let plugin = null

function getPlugin() {
  if (plugin) return plugin
  try {
    plugin = requirePlugin('WechatSI')
    return plugin
  } catch (e) {
    return null
  }
}

// 是否可用
function available() {
  return !!getPlugin()
}

let audio = null

// 播放一段文本语音
function speak(text, onEnd) {
  const p = getPlugin()
  if (!p) {
    if (onEnd) onEnd(false)
    return
  }
  p.textToSpeech({
    lang: 'zh_CN',
    tts: true,
    content: text,
    success: (res) => {
      if (!res || !res.filename) {
        if (onEnd) onEnd(false)
        return
      }
      if (audio) {
        audio.destroy()
      }
      audio = wx.createInnerAudioContext()
      audio.src = res.filename
      audio.play()
      if (onEnd) {
        audio.onEnded(() => onEnd(true))
      }
    },
    fail: () => {
      if (onEnd) onEnd(false)
    }
  })
}

// 停止播放
function stop() {
  if (audio) {
    try { audio.stop() } catch (e) {}
  }
}

module.exports = {
  available: available,
  speak: speak,
  stop: stop
}
