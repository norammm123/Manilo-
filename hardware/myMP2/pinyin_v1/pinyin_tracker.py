"""PinyinTracker — 滑动窗口多数投票 + 字母累积 + PinyinIME 转换 + 长按触发"""
import time
from collections import Counter, deque

from pinyin_ime import PinyinIME

VOTE_WINDOW = 7
VOTE_MAJORITY = 4
CONFIDENCE_THRESH = 0.6
MIN_MARGIN = 0.10
LETTER_COOLDOWN = 0.6
POST_COMMIT_COOLDOWN = 2.0  # 提交后: 上一组末尾字母冷却, 防"手势未变"被带进下一组
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

        # consumed_len 是"字符数"; 缓冲区元素可能是 CH/SH/ZH/NG 等多字符标签,
        # 先把字符消耗量换算回元素数, 避免切片越界 (IndexError)
        consumed_elements = 0
        chars_so_far = 0
        for el in self.letter_buffer:
            if chars_so_far >= consumed_len:
                break
            chars_so_far += len(el)
            consumed_elements += 1
        if chars_so_far > consumed_len:
            consumed_elements -= 1  # 只消耗了半截多字符元素时, 保留该元素

        # 保留末字母: 提交后开启同字母冷却, 防止"手势未变"把上一个字母带进下一组
        last_consumed = self.letter_buffer[consumed_elements - 1] if consumed_elements > 0 else None
        # 清除已消耗的字母 (含无效字符)
        self.letter_buffer = self.letter_buffer[consumed_elements:]
        self.last_letter = last_consumed
        self.cooldown_until = time.time() + POST_COMMIT_COOLDOWN
        self.vote_history.clear()
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

    def reset_sentence(self):
        """清空已提交句子 (每次播报后调用, 防止屏幕/网页端句子不断叠加)"""
        self.sentence_parts.clear()
        self._pending_sentence = ""

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
