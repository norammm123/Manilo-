"""PinyinTracker — 滑动窗口多数投票 + 字母累积 + PinyinIME 转换 + 长按触发"""
import time
from collections import Counter, deque

from pinyin_ime import PinyinIME

VOTE_WINDOW = 7
VOTE_MAJORITY = 4
CONFIDENCE_THRESH = 0.6
MIN_MARGIN = 0.10
LETTER_COOLDOWN = 0.8
HOLD_THRESHOLD = 4  # 同一字母重复确认4次触发 (~3秒按住)


class PinyinTracker:
    """滑动窗口投票确认字母 + 长按触发IME转换"""

    def __init__(self, letter_cooldown=LETTER_COOLDOWN,
                 vote_window=VOTE_WINDOW, vote_majority=VOTE_MAJORITY,
                 min_margin=MIN_MARGIN, confidence_thresh=CONFIDENCE_THRESH):
        self.letter_cooldown = letter_cooldown
        self.vote_window = vote_window
        self.vote_majority = vote_majority
        self.min_margin = min_margin
        self.confidence_thresh = confidence_thresh

        self.letter_buffer = []      # 累积的字母列表 ["n","i","h","a","o"]
        self.sentence_parts = []     # 每次 IME flush 后的中文片段 ["你好","世界"]
        self.last_letter = None
        self.last_letter_time = time.time()
        self.cooldown_until = 0.0
        self.vote_history = deque(maxlen=vote_window)
        self.ime = PinyinIME()
        self._pending_sentence = ""
        self.letter_thresholds = {}  # 逐字母阈值覆盖
        self.hold_count = 0          # 同一字母连续确认计数
        self.hold_triggered = False  # 防止重复触发

    def get_threshold(self, label):
        return self.letter_thresholds.get(label, self.confidence_thresh)

    def feed(self, label, confidence, margin=0.0):
        """返回 (triggered: bool, confirmed_letter: str | None)

        triggered: 长按触发, 调用方应执行 flush_to_ime()
        confirmed_letter: 本次确认的字母 (None 表示未确认)
        """
        now = time.time()
        thresh = self.get_threshold(label)

        if margin < self.min_margin:
            return False, None

        # 冷却期内同字母跳过 (用户保持手势)
        if now < self.cooldown_until and label == self.last_letter:
            return False, None

        if confidence < thresh:
            self.vote_history.append(('', 0))
            return False, None

        self.vote_history.append((label, confidence))

        if len(self.vote_history) < self.vote_window:
            return False, None

        valid = [(l, c) for l, c in self.vote_history if l != '']
        if not valid:
            return False, None

        vote_counts = Counter(l for l, _ in valid)
        vote_confs = {}
        for l, c in valid:
            vote_confs[l] = vote_confs.get(l, []) + [c]

        best_label, best_count = vote_counts.most_common(1)[0]
        best_avg_conf = sum(vote_confs[best_label]) / len(vote_confs[best_label])
        best_thresh = self.get_threshold(best_label)

        if best_count < self.vote_majority or best_avg_conf < best_thresh:
            return False, None

        # 同一字母再次确认 → 累积长按计数
        if best_label == self.last_letter:
            if not self.hold_triggered:
                self.hold_count += 1
                if self.hold_count >= HOLD_THRESHOLD and len(self.letter_buffer) >= 2:
                    self.hold_triggered = True
                    self.vote_history.clear()
                    self.hold_count = 0
                    self.last_letter_time = now
                    return True, None  # 长按触发!
            self.vote_history.clear()
            return False, None

        # 新字母确认
        self.hold_count = 0
        self.hold_triggered = False
        self.vote_history.clear()
        self.last_letter_time = now
        self.cooldown_until = now + self.letter_cooldown
        self.last_letter = best_label
        self.letter_buffer.append(best_label)

        return False, best_label

    def flush_to_ime(self):
        """将累积字母通过 IME 转换为中文, 追加到句子中。
        字母数 < 2 时不触发; 只清除已转换的部分, 未匹配字母保留。
        """
        if len(self.letter_buffer) < 2:
            return "", [], self._pending_sentence

        letters = ''.join(self.letter_buffer).lower()
        chinese, syllables, consumed_len = self.ime.process(letters)

        if not chinese or chinese.isascii():
            return "", [], self._pending_sentence

        # 清除已消耗的字母 (含无效字符, 用 consumed_len 而非音节长度和)
        self.letter_buffer = self.letter_buffer[consumed_len:]
        self.last_letter = None
        self.vote_history.clear()
        self.cooldown_until = 0.0
        self.hold_count = 0
        self.hold_triggered = False

        self.sentence_parts.append(chinese)
        self._pending_sentence = ''.join(self.sentence_parts)

        return chinese, syllables, self._pending_sentence

    def force_flush(self):
        """强制转换 (网页/远程触发)"""
        return self.flush_to_ime()

    def reset(self):
        """清空所有累积状态"""
        self.letter_buffer.clear()
        self.sentence_parts.clear()
        self.last_letter = None
        self.vote_history.clear()
        self.cooldown_until = 0.0
        self.last_letter_time = time.time()
        self._pending_sentence = ""
        self.hold_count = 0
        self.hold_triggered = False

    def get_pinyin_state(self):
        """返回完整拼音状态, 供 MQTT /pinyin 发布"""
        letters = ''.join(self.letter_buffer).lower()
        ime_result = self.ime.process(letters) if letters else ("", [], 0)
        syllables_preview = ime_result[1]
        chinese_preview = ime_result[0]

        return {
            "letters": letters,
            "syllables": syllables_preview,
            "chinese_preview": chinese_preview,
            "sentence_parts": self.sentence_parts.copy(),
            "current_sentence": self._pending_sentence,
            "last_letter": self.last_letter,
            "pending_letters": ' '.join(self.letter_buffer),
        }
