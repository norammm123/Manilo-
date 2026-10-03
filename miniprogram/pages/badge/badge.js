// pages/badge/badge.js - 成就徽章
const api = require('../../utils/api.js')

Page({
  data: {
    badges: [],
    earned: 0,
    analytics: {}
  },

  onLoad() {
    this.loadBadges()
  },

  async loadBadges() {
    try {
      const a = await api.getAnalytics()
      const badges = [
        {
          id: 'first', name: '初次尝试', emoji: '🌱',
          desc: '完成第一次练习',
          earned: (a.sessions_today || 0) + (a.daily ? a.daily.length : 0) > 0
        },
        {
          id: 'streak', name: '坚持一周', emoji: '🔥',
          desc: '连续 7 天训练',
          earned: false,
          progress: '连续训练中'
        },
        {
          id: 'words', name: '词汇达人', emoji: '📚',
          desc: '掌握 10 个词汇',
          earned: (a.vocabulary_count || 0) >= 10,
          progress: `已学 ${a.vocabulary_count || 0}/10`
        },
        {
          id: 'accuracy', name: '精准发音', emoji: '🎯',
          desc: '跟读准确率 ≥ 90%',
          earned: (a.follow_read_success_rate || 0) >= 0.9,
          progress: `当前 ${Math.round((a.follow_read_success_rate || 0) * 100)}%`
        },
        {
          id: 'sessions', name: '勤学不辍', emoji: '📈',
          desc: '累计 10 次练习',
          earned: (a.sessions_today || 0) >= 10,
          progress: `今日 ${a.sessions_today || 0}/10`
        },
        {
          id: 'fluent', name: '流利之星', emoji: '⭐',
          desc: '一次完成 5 句成句',
          earned: false,
          progress: '努力中'
        }
      ]
      const earnedCount = badges.filter(b => b.earned).length
      this.setData({ badges, earned: earnedCount, analytics: a })
    } catch (e) {
      // 后端不通时展示未解锁状态
      this.setData({
        badges: [
          { id: 'first', name: '初次尝试', emoji: '🌱', desc: '完成第一次练习', earned: false, progress: '待解锁' },
          { id: 'streak', name: '坚持一周', emoji: '🔥', desc: '连续 7 天训练', earned: false, progress: '待解锁' },
          { id: 'words', name: '词汇达人', emoji: '📚', desc: '掌握 10 个词汇', earned: false, progress: '待解锁' },
          { id: 'accuracy', name: '精准发音', emoji: '🎯', desc: '跟读准确率 ≥ 90%', earned: false, progress: '待解锁' },
          { id: 'sessions', name: '勤学不辍', emoji: '📈', desc: '累计 10 次练习', earned: false, progress: '待解锁' },
          { id: 'fluent', name: '流利之星', emoji: '⭐', desc: '一次完成 5 句成句', earned: false, progress: '待解锁' }
        ],
        earned: 0,
        analytics: { mastery_available: false }
      })
    }
  }
})
