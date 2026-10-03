// pages/lexicon/lexicon.js - 词库学习
// 本地词库(有发音的词汇来自 static/audio/ 下的 mp3)
const WORDS = [
  { word: '你好', pinyin: 'nǐ hǎo', emoji: '👋', audio: true },
  { word: '谢谢', pinyin: 'xiè xie', emoji: '🙏', audio: true },
  { word: '再见', pinyin: 'zài jiàn', emoji: '👋', audio: true },
  { word: '水', pinyin: 'shuǐ', emoji: '💧', audio: true },
  { word: '妈妈', pinyin: 'mā ma', emoji: '👩', audio: true },
  { word: '小猫', pinyin: 'xiǎo māo', emoji: '🐱', audio: true },
  { word: '早上好', pinyin: 'zǎo shang hǎo', emoji: '🌅', audio: true },
  { word: '很高兴认识你', pinyin: 'hěn gāo xìng rèn shi nǐ', emoji: '🤝', audio: true },
  { word: '我', pinyin: 'wǒ', emoji: '🙋', audio: false },
  { word: '你', pinyin: 'nǐ', emoji: '👉', audio: false },
  { word: '爸爸', pinyin: 'bà ba', emoji: '👨', audio: false },
  { word: '老师', pinyin: 'lǎo shī', emoji: '🧑‍🏫', audio: false },
  { word: '朋友', pinyin: 'péng you', emoji: '👫', audio: false },
  { word: '吃饭', pinyin: 'chī fàn', emoji: '🍚', audio: false },
  { word: '睡觉', pinyin: 'shuì jiào', emoji: '😴', audio: false },
  { word: '学校', pinyin: 'xué xiào', emoji: '🏫', audio: false },
  { word: '家', pinyin: 'jiā', emoji: '🏠', audio: false },
  { word: '爱', pinyin: 'ài', emoji: '❤️', audio: false }
]

Page({
  data: {
    keyword: '',
    filteredWords: WORDS
  },

  onSearch(e) {
    const keyword = e.detail.value.trim()
    const filteredWords = WORDS.filter(w => !keyword || w.word.includes(keyword))
    this.setData({ keyword, filteredWords })
  },

  playWord(e) {
    const word = e.currentTarget.dataset.word
    const item = WORDS.find(w => w.word === word)
    if (!item || !item.audio) {
      wx.showToast({ title: '该词暂无发音', icon: 'none' })
      return
    }
    const audio = wx.createInnerAudioContext()
    audio.src = '/static/audio/' + word + '.mp3'
    audio.onError(() => wx.showToast({ title: '发音加载失败', icon: 'none' }))
    audio.play()
  }
})
