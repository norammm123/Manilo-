// pages/training/training.js - 训练页面
const app = getApp()
const asr = require('../../utils/asr.js')
const tts = require('../../utils/tts.js')

Page({
  data: {
    currentMode: 'echo',
    modeTitle: '回声映射',
    modeSubtitle: '声音到手语',
    currentWord: { text: '', emoji: '', pinyin: '' },
    score: 0,
    targetWord: '你好',
    isRecording: false,
    countdown: 0,
    recordingText: '',
    asrReady: false,
    chatList: [],
    chatRecording: false,
    chatThinking: false,
    // 跟读剧本
    scriptScenes: [
      {
        name: '问候', icon: '👋',
        sentences: [
          { text: '你好，很高兴认识你。', audio: 's1' },
          { text: '早上好，今天天气不错。', audio: 's2' },
          { text: '谢谢你的帮助。', audio: 's3' }
        ]
      },
      {
        name: '点餐', icon: '🍜',
        sentences: [
          { text: '我要一杯水。', audio: 's4' },
          { text: '谢谢，不用了。', audio: 's5' },
          { text: '这个很好吃。', audio: 's6' }
        ]
      },
      {
        name: '购物', icon: '🛍️',
        sentences: [
          { text: '这个多少钱？', audio: 's7' },
          { text: '太贵了，便宜一点吧。', audio: 's8' },
          { text: '我买这个。', audio: 's9' }
        ]
      },
      {
        name: '校园', icon: '🏫',
        sentences: [
          { text: '我们一起去上学吧。', audio: 's10' },
          { text: '老师好。', audio: 's11' },
          { text: '再见，明天见。', audio: 's12' }
        ]
      }
    ],
    currentScene: null,       // 当前选择的情景
    scriptSentences: [],      // 当前情景的句子
    scriptIndex: 0,
    scriptScores: [],
    scriptDone: false,
    scriptAvg: 0,
    aiSuggestion: '',
    trainingCount: 12,
    avgScore: 87,
    duration: 8
  },

  onLoad() {
    this.loadWord()
    this.setData({ asrReady: asr.available(), ttsReady: tts.available() })
  },

  // 切换模式
  switchMode(e) {
    const mode = e.currentTarget.dataset.mode
    const titles = {
      echo: { title: '回声映射', subtitle: '声音到手语' },
      speak: { title: '主动发声', subtitle: '发音纠错' },
      chat: { title: '跟读剧本', subtitle: '逐句跟读练习' }
    }
    this.setData({
      currentMode: mode,
      modeTitle: titles[mode].title,
      modeSubtitle: titles[mode].subtitle
    })
    // 首次进入跟读剧本, 初始化剧本
    if (mode === 'chat' && this.data.scriptIndex === 0 && this.data.scriptScores.length === 0) {
      this.initScript()
    }
  },

  // 跟读剧本: 初始化为"情景选择"模式
  initScript() {
    this.setData({
      currentScene: null,
      scriptMode: 'chat',  // 兼容旧 data
      scriptIndex: 0,
      scriptScores: [],
      scriptDone: false,
      scriptAvg: 0,
      score: 0,
      recordingText: ''
    })
  },

  // 选择情景剧本
  chooseScene(e) {
    const idx = e.currentTarget.dataset.idx
    const scene = this.data.scriptScenes[idx]
    this.setData({
      currentScene: scene,
      scriptSentences: scene.sentences,
      scriptIndex: 0,
      scriptScores: [],
      scriptDone: false,
      scriptAvg: 0,
      score: 0,
      recordingText: ''
    })
  },

  // 返回情景列表
  backToScenes() {
    this.setData({ currentScene: null, scriptDone: false, isRecording: false, score: 0 })
  },

  // 播放当前句示范音
  playScriptSound() {
    const sentence = this.data.scriptSentences[this.data.scriptIndex]
    if (!sentence) return
    const audio = wx.createInnerAudioContext()
    audio.src = '/static/audio/scripts/' + sentence.audio + '.m4a'
    audio.onError(() => wx.showToast({ title: '暂无示范音', icon: 'none' }))
    audio.play()
    this._scriptAudio = audio
  },

  // 下一句/完成
  nextScript() {
    if (this.data.isRecording) return
    const next = this.data.scriptIndex + 1
    if (next >= this.data.scriptSentences.length) {
      // 完成: 计算平均分
      const scores = this.data.scriptScores
      const avg = scores.length
        ? Math.round(scores.reduce((a, b) => a + b, 0) / scores.length)
        : 0
      this.setData({ scriptDone: true, scriptAvg: avg })
    } else {
      this.setData({
        scriptIndex: next,
        score: 0,
        recordingText: ''
      })
    }
  },

  // 重练当前剧本
  restartScript() {
    this.setData({
      scriptIndex: 0,
      scriptScores: [],
      scriptDone: false,
      scriptAvg: 0,
      score: 0,
      recordingText: ''
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
    audio.src = '/static/audio/' + word + '.m4a'
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
    audio.src = '/static/audio/很标准.m4a'
    audio.play()
    this._praise = audio
  },

  onUnload() {
    if (this._audio) this._audio.destroy()
    if (this._praise) this._praise.destroy()
    if (this._countTimer) {
      clearInterval(this._countTimer)
      this._countTimer = null
    }
  },

  // 开始/停止录音 (按一次开始, 再按一次停止)
  // 录音是"必选权限": 直接调 recorder.start() 系统会自动弹授权框,
  // 不需要 wx.authorize, 也不需要在 app.json 声明 permission(声明了反而报"无效")
  startRecord() {
    if (this.data.isRecording) {
      this.stopRecord()
      return
    }
    wx.getSetting({
      success: (res) => {
        const auth = res.authSetting['scope.record']
        if (auth === false) {
          // 明确拒绝过 → 引导去设置页
          wx.showModal({
            title: '麦克风权限被拒绝',
            content: '你之前拒绝了麦克风授权，录音无法进行。\n请到设置页开启麦克风权限。',
            confirmText: '去设置',
            success: (r) => {
              if (r.confirm) wx.openSetting()
            }
          })
          return
        }
        // 未拒绝(含从未询问) → 直接录音, 系统自动弹授权
        this.doStartRecord()
      },
      fail: () => this.doStartRecord()
    })
  },

  // 真正开始录音
  doStartRecord() {
    this.setData({ isRecording: true, countdown: 0, score: 0, recordingText: '' })
    this._levels = []
    // 录音(getRecorderManager): 参数用最兼容组合,
    // 不设 frameSize(真机帧回调格式不可靠), 评分走"文件大小+时长"通道
    this._recorder = wx.getRecorderManager()
    this._recorder.onStop((res) => {
      this.setData({ isRecording: false })
      this._fileInfo = {
        size: res.fileSize || 0,
        duration: res.duration || 0
      }
      this.scoreFromLevels()
    })
    this._recorder.onError((err) => {
      console.error('录音错误:', err)
      this.setData({ isRecording: false })
      wx.showModal({
        title: '录音失败',
        content: (err && err.errMsg) ? err.errMsg : '无法录音，请检查麦克风',
        showCancel: false
      })
    })
    this._recorder.start({
      duration: 60000,
      sampleRate: 44100,
      numberOfChannels: 1,
      encodeBitRate: 192000,
      format: 'aac',
      frameSize: 10
    })
  },

  // 手动停止录音
  stopRecord() {
    if (this._recorder) {
      this._recorder.stop()
    }
  },

  // 录音完成: 双通道评分(音量分析 或 文件大小判定)
  scoreFromLevels() {
    const levels = this._levels || []
    const info = this._fileInfo || {}

    // 通道A: PCM 音量分析(真机 frameBuffer 为标准 PCM 时最准)
    const usableLevels = levels.filter(v => v > 0)
    if (usableLevels.length > 10) {
      const startIdx = Math.floor(usableLevels.length * 0.15)
      const endIdx = Math.floor(usableLevels.length * 0.75)
      const mid = usableLevels.slice(startIdx, endIdx)
      if (mid.length > 0) {
        const sorted = mid.slice().sort((a, b) => a - b)
        const median = sorted[Math.floor(sorted.length / 2)]
        const p75 = sorted[Math.floor(sorted.length * 0.75)]
        const voiceFrames = mid.filter(v => v > 800).length
        const voiceRatio = voiceFrames / mid.length

        if (median > 800) {
          // PCM 有效且确实有声音 → 精确评分
          const loudness = Math.min(1, (p75 - 800) / 20000)
          const stability = Math.min(1, voiceRatio * 1.2)
          const score = Math.max(45, Math.min(95, Math.round(45 + loudness * 35 + stability * 15)))
          this.setData({ score: score, recordingText: '(音量检测) ' + score + ' 分' })
          this.finishScoring(score, '')
          return
        }
      }
    }

    // 通道B: 文件大小 + 时长判定 (真机通用, 说话的文件更大)
    const size = info.size || 0
    const durationMs = info.duration || 0
    // 经验阈值: 16kHz 24kbps mp3, 静音 1 秒 ≈ 2~3KB, 说话 1 秒 ≈ 4~6KB
    const seconds = durationMs / 1000
    let score = 0
    let text = ''
    if (size <= 0) {
      text = '录音失败，未生成音频文件'
    } else if (seconds < 0.5) {
      text = '录音太短，请说完再停止'
    } else if (size < seconds * 2500) {
      text = '没听到声音，请靠近麦克风再试'
    } else {
      // 有声音: 按文件密度(字节/秒)评分, 说话密度越高分越高
      const density = size / Math.max(1, seconds)
      const ratio = Math.min(1, (density - 2500) / 4000)
      score = Math.max(45, Math.min(95, Math.round(45 + ratio * 40)))
      text = '音量检测评分'
    }
    this.setData({ score: score, recordingText: '(' + text + ') ' + (score || 0) + ' 分' })
    this.finishScoring(score, '')
  },

  // 真实录音 + 音量检测(无插件降级): 只分析录音中段, 归一化降波动
  recordWithVolume() {
    const recorder = wx.getRecorderManager()
    const levels = []      // 记录每帧音量
    let frameCount = 0
    const FRAME_MS = 10    // frameSize: 10 (每帧10ms? 实际为16ms量级, 这里按帧计数)

    recorder.onFrameRecorded((res) => {
      const buf = res.frameBuffer
      if (buf.byteLength < 2) return
      const int16 = new Int16Array(buf.buffer)
      let sum = 0
      const step = Math.max(1, Math.floor(int16.length / 24))
      for (let i = 0; i < int16.length; i += step) {
        sum += Math.abs(int16[i])
      }
      const avg = sum / Math.ceil(int16.length / step)
      levels.push(avg)
      frameCount++
    })

    recorder.onStop((res) => {
      // 只取中段 60% (跳过前15%防止示范音残留, 跳过后25%防收尾噪声)
      const total = levels.length
      const startIdx = Math.floor(total * 0.15)
      const endIdx = Math.floor(total * 0.75)
      const mid = levels.slice(startIdx, endIdx)
      if (mid.length === 0) {
        this.recordResult(0, 0, 0)
        return
      }

      // 排序取中位数 + P75, 抗干扰
      const sorted = mid.slice().sort((a, b) => a - b)
      const median = sorted[Math.floor(sorted.length / 2)]
      const p75 = sorted[Math.floor(sorted.length * 0.75)]
      const voiceFrames = mid.filter(v => v > 800).length
      const voiceRatio = voiceFrames / mid.length

      // 归一化评分: 中位数与P75结合, 上限95, 无声音=0
      let score = 0
      if (median > 800) {
        const loudness = Math.min(1, (p75 - 800) / 20000)
        const stability = Math.min(1, voiceRatio * 1.2)
        score = Math.max(40, Math.min(95, Math.round(45 + loudness * 35 + stability * 15)))
      }
      this.recordResult(score, median, voiceRatio)
    })

    recorder.start({
      duration: 60000,      // 手动停止, 给足 60 秒
      sampleRate: 16000,
      numberOfChannels: 1,
      encodeBitRate: 96000,
      format: 'mp3',
      frameSize: 10
    })
    this._recorder = recorder
  },

  // 音量检测结果
  recordResult(score, median, voiceRatio) {
    let text = '(无识别插件, 按音量检测)'
    if (median <= 800) {
      text = '没听到声音，请靠近麦克风再试'
    } else if (voiceRatio < 0.4) {
      text = '声音断断续续，请连续说完一个词'
    }
    this.setData({
      isRecording: false,
      score: score,
      recordingText: text
    })
    this.finishScoring(score, '')
  },

  // 识别结果回调
  onRecognized(text) {
    this._asrSession = null
    const target = this.data.targetWord
    const heard = (text || '').trim()
    const score = this.compareText(target, heard)
    this.setData({
      isRecording: false,
      recordingText: heard || '(未识别到内容)',
      score: score
    })
    this.finishScoring(score, heard)
  },

  // 文本相似度评分 (编辑距离)
  compareText(target, heard) {
    if (!heard) return 0
    const lev = (a, b) => {
      const m = a.length, n = b.length
      const dp = []
      for (let i = 0; i <= m; i++) dp.push([i].concat(new Array(n).fill(0)))
      for (let j = 0; j <= n; j++) dp[0][j] = j
      for (let i = 1; i <= m; i++) {
        for (let j = 1; j <= n; j++) {
          dp[i][j] = Math.min(
            dp[i - 1][j] + 1,
            dp[i][j - 1] + 1,
            dp[i - 1][j - 1] + (a[i - 1] === b[j - 1] ? 0 : 1)
          )
        }
      }
      return dp[m][n]
    }
    const max = Math.max(target.length, heard.length) || 1
    const sim = 1 - lev(target, heard) / max
    return Math.max(0, Math.min(100, Math.round(sim * 100)))
  },

  // 评分后续: 反馈 + 剧本计分
  finishScoring(score, heard) {
    const target = this.data.targetWord
    let suggestion = ''
    if (score >= 80) {
      suggestion = '发音很标准！继续保持。'
      setTimeout(() => this.playPraise(), 600)
    } else if (score >= 50) {
      suggestion = '有进步！注意口型和声调，再试一次。'
      setTimeout(() => this.playWordAudio(target), 800)
    } else {
      suggestion = '先听示范音，放慢语速再跟读一次。'
      setTimeout(() => this.playWordAudio(target), 800)
    }
    this.setData({ aiSuggestion: suggestion })

    // 跟读剧本模式: 记录当前句得分
    if (this.data.currentMode === 'chat') {
      const scores = this.data.scriptScores.slice()
      // 记录到当前 index (覆盖重复练习)
      scores[this.data.scriptIndex] = score
      this.setData({ scriptScores: scores })
    }
  }
})
