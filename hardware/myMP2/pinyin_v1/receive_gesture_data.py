"""手势词语数据采集 — 串口接收F4数据 → 保存到 raw/{WORD}/

用法: python3 receive_gesture_data.py
交互式: 选择词语 → 做手势 → 采集200帧 → 保存

F4输出: 0xAA + CSV(11字段) + 0xBB, 115200baud
保存: raw_gesture/{WORD}/{WORD}_{NNN}.txt (200行 × 11列)
"""
import os
import sys
import time

# === 配置 ===
SERIAL_PORT = '/dev/ttySTM9'
SERIAL_BAUD = 115200
TRAIN_FRAMES = 200
NUM_FEATURES = 11
RAW_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'raw_gesture')

# 17个手势词语 + 简要描述
GESTURES = {
    "不":     "一手直立掌心向外，左右摆动几下",
    "你":     "一手食指指向对方",
    "叫":     "一手伸拇小指，在嘴边做呼喊状",
    "哪儿":   "一手食指在空中画问号",
    "在":     "一手拇食中指捏合，向下一点",
    "天":     "一手食指竖立，空中画圆弧",
    "好":     "一手伸出拇指",
    "妈妈":   "一手食指指胸部，然后掌心向下胸前转一圈",
    "很":     "一手食指横伸，拇指尖抵于食指根部，向下一顿",
    "我":     "一手食指指自己",
    "打电话": "一手拇小指伸出，靠近耳边做打电话状",
    "早上":   "一手四指并拢与拇指相捏，然后张开",
    "是":     "一手食中指相叠，向前下方点动一下",
    "有":     "一手拇食指伸直，掌心向上，食指弯动两下",
    "来":     "一手掌心向下，向内挥动一下",
    "要":     "一手掌心向上，向怀里移动",
    "认识":   "一手食中指从眼前向前方伸出",
}

LABELS = sorted(GESTURES.keys())


class SerialReader:
    """串口读取器: 0xAA 帧头 + CSV文本(11字段) + 0xBB 帧尾"""

    def __init__(self, port=SERIAL_PORT, baud=SERIAL_BAUD):
        self.port = port
        self.baud = baud
        self.fd = None
        self.buf = bytearray()
        self._backend = 'termios'
        self.frame_ok = 0
        self.frame_fail = 0

    def open(self):
        try:
            import serial
            self.ser = serial.Serial(
                port=self.port, baudrate=self.baud, timeout=0.01,
                bytesize=serial.EIGHTBITS, parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE)
            self._backend = 'pyserial'
            print(f"[串口] {self.port} @ {self.baud} (pyserial)")
            return True
        except ImportError:
            pass
        except Exception as e:
            print(f"[串口] pyserial失败: {e}")

        try:
            import termios
            fd = os.open(self.port, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
            tty = termios.tcgetattr(fd)
            baud_map = {9600: termios.B9600, 115200: termios.B115200,
                        921600: termios.B921600, 460800: termios.B460800,
                        57600: termios.B57600, 38400: termios.B38400,
                        19200: termios.B19200}
            tty[4] = baud_map.get(self.baud, termios.B9600)
            tty[5] = tty[4]
            tty[2] = termios.CLOCAL | termios.CREAD | termios.CS8
            tty[0] = 0; tty[1] = 0; tty[3] = 0
            tty[6][termios.VMIN] = 0
            tty[6][termios.VTIME] = 0
            termios.tcsetattr(fd, termios.TCSANOW, tty)
            self.fd = fd
            self._backend = 'termios'
            print(f"[串口] {self.port} @ {self.baud} (termios)")
            return True
        except Exception as e:
            print(f"[串口] 打开失败: {e}")
            return False

    def _read_bytes(self):
        if self._backend == 'pyserial':
            try:
                n = self.ser.in_waiting
                if n > 0:
                    data = self.ser.read(n)
                    if data:
                        self.buf.extend(data)
            except Exception:
                pass
        else:
            try:
                data = os.read(self.fd, 512)
                if data:
                    self.buf.extend(data)
            except (BlockingIOError, OSError):
                pass

    def read_frame(self):
        """返回 11 元素 float list 或 None"""
        self._read_bytes()

        # 清理开头连续的 00 噪声字节
        zero_end = 0
        for b in self.buf:
            if b == 0x00:
                zero_end += 1
            else:
                break
        if zero_end > 0:
            del self.buf[:zero_end]

        # 跳过模式字节 0xFF / 0xEE
        while len(self.buf) > 0 and self.buf[0] in (0xFF, 0xEE):
            del self.buf[0]

        # 找帧头 0xAA
        aa_idx = -1
        for i in range(len(self.buf)):
            if self.buf[i] == 0xAA:
                aa_idx = i
                break

        if aa_idx < 0:
            if len(self.buf) > 5000:
                self.buf.clear()
            return None

        if aa_idx > 0:
            del self.buf[:aa_idx]

        # 找帧尾 0xBB
        bb_idx = -1
        for i in range(1, len(self.buf)):
            if self.buf[i] == 0xBB:
                bb_idx = i
                break

        if bb_idx < 0:
            return None

        # 提取 ASCII CSV payload
        payload = bytes(self.buf[1:bb_idx])
        del self.buf[:bb_idx + 1]

        try:
            text = payload.decode('ascii', errors='ignore').strip()
            if not text:
                return None
            parts = text.split(',')
            if len(parts) < NUM_FEATURES:
                self.frame_fail += 1
                return None
            frame = [float(p) for p in parts[:NUM_FEATURES]]
            self.frame_ok += 1
            return frame
        except (ValueError, UnicodeDecodeError):
            self.frame_fail += 1
            return None

    def drain(self, duration=1.0):
        self.buf.clear()
        t0 = time.time()
        while time.time() - t0 < duration:
            self._read_bytes()
            self.buf.clear()
            time.sleep(0.005)

    def close(self):
        if self._backend == 'pyserial' and hasattr(self, 'ser'):
            self.ser.close()
        elif self.fd is not None:
            try:
                os.close(self.fd)
            except OSError:
                pass


def show_words():
    print("\n可选手势词语 (17类):")
    for i, name in enumerate(LABELS):
        print(f"  {i+1:2d}. {name:6s} — {GESTURES[name]}")


def get_count(word):
    d = os.path.join(RAW_DIR, word)
    if not os.path.isdir(d):
        return 0
    return len([f for f in os.listdir(d) if f.endswith('.txt')])


def collect_sample(reader, word, quiet=False):
    """采集一条样本: drain → 收集200帧 → 返回 ([frames], 耗时)"""
    reader.drain()

    if not quiet:
        print(f"\n[{word}] 手势: {GESTURES[word]}", end='', flush=True)

    frames = []
    t0 = time.time()
    timeout = 15.0

    while len(frames) < TRAIN_FRAMES:
        frame = reader.read_frame()
        if frame is None:
            if time.time() - t0 > timeout:
                print(f"\n[超时] 15秒未收到足够帧 (已收到 {len(frames)}/{TRAIN_FRAMES})")
                return None, time.time() - t0
            time.sleep(0.01)
            continue

        frames.append(frame)

        pct = len(frames) / TRAIN_FRAMES
        bar_len = 30
        filled = int(bar_len * pct)
        bar = '#' * filled + '-' * (bar_len - filled)
        if len(frames) == 1 or len(frames) % 20 == 0:
            print(f"\r  [{bar}] {len(frames)}/{TRAIN_FRAMES}", end='', flush=True)

    elapsed = time.time() - t0
    fps = TRAIN_FRAMES / elapsed
    print(f"\r  [{word}] {len(frames)}帧 {elapsed:.1f}s ({fps:.0f}fps)", flush=True)

    return frames, elapsed


def save_sample(word, frames, count):
    d = os.path.join(RAW_DIR, word)
    os.makedirs(d, exist_ok=True)

    fname = f"{word}_{count:03d}.txt"
    fpath = os.path.join(d, fname)

    header = "adc1,adc2,adc3,adc4,adc5,ax,ay,az,gx,gy,gz"
    lines = [header]
    for frame in frames:
        lines.append(','.join(f'{v:.6f}' for v in frame))

    with open(fpath, 'w', encoding='ascii') as f:
        f.write('\n'.join(lines))

    size_kb = os.path.getsize(fpath) / 1024
    print(f"  已保存: {fpath} ({size_kb:.1f} KB)")
    return fpath


def print_stats():
    print("\n当前数据统计:")
    total = 0
    for name in LABELS:
        cnt = get_count(name)
        total += cnt
        marker = "  <-- !" if cnt == 0 else ""
        print(f"  {name:6s}: {cnt:3d}{marker}")
    print(f"  {'总计':6s}: {total}")
    return total


def main():
    print("=== 手势词语数据采集 ===")
    print(f"串口: {SERIAL_PORT} @ {SERIAL_BAUD}")
    print(f"保存目录: {RAW_DIR}")
    print(f"每样本: {TRAIN_FRAMES}帧")
    os.makedirs(RAW_DIR, exist_ok=True)

    reader = SerialReader(SERIAL_PORT, SERIAL_BAUD)
    if not reader.open():
        print("[FAIL] 串口打开失败")
        return

    show_words()
    print_stats()

    print("\n提示: 输入词语选择手势 → 输入采集轮数 → 做手势自动采集")
    print("      exit=退出  stats=统计  list=手势列表\n")

    try:
        while True:
            # --- 选择词语 ---
            word = None
            while word is None:
                resp = input("选择词语: ").strip()
                if resp.lower() in ('exit', 'quit', ''):
                    reader.close()
                    print("\n已退出")
                    return
                if resp.lower() == 'stats':
                    print_stats()
                    continue
                if resp.lower() == 'list':
                    show_words()
                    continue
                if resp in LABELS:
                    word = resp
                else:
                    # 尝试数字选择
                    try:
                        idx = int(resp) - 1
                        if 0 <= idx < len(LABELS):
                            word = LABELS[idx]
                        else:
                            print(f"  无效编号 '{resp}'")
                    except ValueError:
                        print(f"  无效词语 '{resp}'")

            existing = get_count(word)
            desc = GESTURES[word]
            print(f"\n[{word}] {desc} | 已有 {existing} 样本")

            # --- 选择轮数 ---
            n_rounds = 0
            while n_rounds <= 0:
                resp = input(f"采集几轮? [1-20, 默认5]: ").strip()
                if resp == '':
                    n_rounds = 5
                elif resp.lower() in ('exit', 'quit'):
                    reader.close()
                    return
                elif resp.lower() == 'stats':
                    print_stats()
                    continue
                else:
                    try:
                        n_rounds = int(resp)
                        if n_rounds < 1 or n_rounds > 50:
                            print("  请输入 1-50")
                            n_rounds = 0
                    except ValueError:
                        print("  请输入数字")

            # --- 批量采集 ---
            print(f"\n{'='*50}")
            print(f"[{word}] 批量采集 x{n_rounds} 轮")
            print("保持手势不动, 脚本自动连续采集...")
            print(f"{'='*50}")

            success = 0
            for rnd in range(n_rounds):
                result = collect_sample(reader, word, quiet=True)
                if result[0] is None:
                    print(f"  第 {rnd+1}/{n_rounds} 轮超时, 跳过")
                    continue

                frames, elapsed = result
                cnt = get_count(word) + 1
                save_sample(word, frames, cnt)
                success += 1

            print(f"\n[{word}] 完成 {success}/{n_rounds} 轮, 共 {get_count(word)} 样本")
            print_stats()

    except KeyboardInterrupt:
        print("\n")
    finally:
        reader.close()
        print_stats()
        print("已退出")


if __name__ == '__main__':
    main()
