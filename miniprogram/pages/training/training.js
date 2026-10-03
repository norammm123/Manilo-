// pages/training/training.js - 训练页面
const app = getApp()

Page({
  data: {
    currentMode: 'echo',
    modeTitle: '回声映射',
    modeSubtitle: '声音到手语',
    currentWord: { text: '', emoji: '', pinyin: '' },
    score: 0,
    targetWord: '你好',
    isRecording: false,
    chatList: [],
    aiSuggestion: '',
    trainingCount: 12,
    avgScore: 87,
    duration: 8
  },

  onLoad() {
    this.loadWord()
  },

  // 切换模式
  switchMode(e) {
    const mode = e.currentTarget.dataset.mode
    const titles = {
      echo: { title: '回声映射', subtitle: '声音到手语' },
      speak: { title: '主动发声', subtitle: '发音纠错' },
      chat: { title: '情景对话', subtitle: '角色扮演练习' }
    }
    this.setData({
      currentMode: mode,
      modeTitle: titles[mode].title,
      modeSubtitle: titles[mode].subtitle
    })
  },

  // 加载词汇
  loadWord() {
    const words = [
      { text: '你好', emoji: '👋', pinyin: 'nǐ hǎo' },
      { text: '谢谢', emoji: '🙏', pinyin: 'xiè xie' },
      { text: '再见', emoji: '👋', pinyin: 'zài jiàn' },
      { text: '水', emoji: '💧', pinyin: 'shuǐ' }
    ]
    const word = words[Math.floor(Math.random() * words.length)]
    this.setData({ currentWord: word, targetWord: word.text })
  },

  // 播放声音
  playSound() {
    const word = this.data.currentWord.text || this.data.targetWord
    this.playWordAudio(word)
  },

  // 播放指定词汇发音(本地 mp3)
  playWordAudio(word) {
    const audio = wx.createInnerAudioContext()
    audio.src = '/static/audio/' + word + '.mp3'
    audio.onError((err) => {
      console.warn('音频播放失败:', err)
      wx.showToast({ title: '暂无该词发音', icon: 'none' })
    })
    audio.play()
    this._audio = audio
  },

  // 播放鼓励音
  playPraise() {
    const audio = wx.createInnerAudioContext()
    audio.src = '/static/audio/很标准.mp3'
    audio.play()
    this._praise = audio
  },

  onUnload() {
    if (this._audio) this._audio.destroy()
    if (this._praise) this._praise.destroy()
  },

  // 开始录音
  startRecord() {
    // 先播一遍目标词发音当示范
    this.playWordAudio(this.data.targetWord)
    this.setData({ isRecording: true })
    setTimeout(() => {
      const score = Math.floor(Math.random() * 40) + 60
      this.setData({ 
        isRecording: false, 
        score: score,
        aiSuggestion: score >= 80 ? '发音很标准！继续保持。' : '注意口型，再试一次。'
      })
      // 高分播放鼓励音
      if (score >= 80) {
        setTimeout(() => this.playPraise(), 600)
      } else {
        // 低分再播一次目标音, 方便跟读
        setTimeout(() => this.playWordAudio(this.data.targetWord), 600)
      }
    }, 2000)
  },

  // 开始对话（预设场景台词）
  startChat() {
    this.setData({
      chatList: [
        { type: 'ai', text: '同学你好，我们开始今天的发音练习吧。' },
        { type: 'me', text: '我想练习手语。' },
        { type: 'ai', text: '好的，先把手套戴上，我们从简单词汇开始！' }
      ]
    })
  }
})
