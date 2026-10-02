"""拼音手指字母数据采集 — 串口接收F4数据 → 保存到 raw/{LETTER}/

用法: python3 receive_pinyin_data.py
交互式: 选择字母 → 做手势 → 采集200帧 → 保存

F4输出: 0xAA + CSV(11字段) + 0xBB, 115200baud
保存: raw/{LETTER}/{LETTER}_{NNN}.txt (200行 × 11列)
"""
import os
import sys
import time
from collections import deque

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from gesture_map import GESTURES

# === 配置 ===
SERIAL_PORT = '/dev/ttySTM9'
SERIAL_BAUD = 115200
TRAIN_FRAMES = 200
NUM_FEATURES = 11
RAW_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'raw')

LABELS = sorted(GESTURES.keys(), key=lambda x: (len(x), x))


class SerialReader:
    """串口读取器: 08 00 二进制帧分隔 + CSV文本(11字段)"""

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
        """返回 11 元素 float list 或 None

        F4发送格式: 0xAA [ASCII CSV 11值] 0xBB
        CSV示例: 1845,-320,1200,890,-450,123,456,789,10,-20,30
        """
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

        # 丢弃 0xAA 之前的无效字节
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
        """清空串口缓冲区旧数据, duration为清空时长(秒)"""
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


def show_letters():
    """打印30个字母选择菜单"""
    print("\n可选手势 (30类):")
    count = 0
    for name in LABELS:
        g = GESTURES[name]
        dyn = " [动态]" if g["type"] == "dynamic" else ""
        print(f"  {name:<5s} {g['desc']}{dyn}")
        count += 1
        if count % 3 == 0:
            print()
    if count % 3 != 0:
        print()


def get_count(letter):
    """获取某个字母已有样本数"""
    d = os.path.join(RAW_DIR, letter)
    if not os.path.isdir(d):
        return 0
    return len([f for f in os.listdir(d) if f.endswith('.txt')])


def collect_sample(reader, letter, quiet=False):
    """采集一条样本: drain → 收集200帧 → 返回 ([frames], 耗时)"""
    reader.drain()

    if not quiet:
        print(f"\n[{letter}] 手势: {GESTURES[letter]['desc']}", end='', flush=True)

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

        # 进度条
        pct = len(frames) / TRAIN_FRAMES
        bar_len = 30
        filled = int(bar_len * pct)
        bar = '#' * filled + '-' * (bar_len - filled)
        if len(frames) == 1 or len(frames) % 20 == 0:
            print(f"\r  [{bar}] {len(frames)}/{TRAIN_FRAMES}", end='', flush=True)

    elapsed = time.time() - t0
    fps = TRAIN_FRAMES / elapsed
    print(f"\r  [{letter}] {len(frames)}帧 {elapsed:.1f}s ({fps:.0f}fps)", flush=True)

    return frames, elapsed


def save_sample(letter, frames, count):
    """保存帧数据到 raw/{LETTER}/{LETTER}_{count:03d}.txt"""
    d = os.path.join(RAW_DIR, letter)
    os.makedirs(d, exist_ok=True)

    # 文件名: A_001.txt, A_002.txt, ...
    fname = f"{letter}_{count:03d}.txt"
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
    """打印各类别已有样本统计"""
    print("\n当前数据统计:")
    total = 0
    for name in LABELS:
        cnt = get_count(name)
        total += cnt
        marker = "  <-- !" if cnt == 0 else ""
        print(f"  {name:4s}: {cnt:3d}{marker}")
    print(f"  {'总计':4s}: {total}")
    return total


def confirm(prompt="确认? [Y/n/r/q] "):
    """用户确认: y=确认, n=否认, r=重录, q=退出"""
    while True:
        resp = input(prompt).strip().lower()
        if resp in ('', 'y', 'yes'):
            return 'y'
        if resp in ('n', 'no'):
            return 'n'
        if resp in ('r', 'retry'):
            return 'r'
        if resp in ('q', 'quit', 'exit'):
            return 'q'
        print("  输入: y=保存 n=丢弃 r=重录 q=退出")


def main():
    print("=== 拼音手指字母数据采集 ===")
    print(f"串口: {SERIAL_PORT} @ {SERIAL_BAUD}")
    print(f"保存目录: {RAW_DIR}")
    print(f"每样本: {TRAIN_FRAMES}帧")
    os.makedirs(RAW_DIR, exist_ok=True)

    # 串口
    reader = SerialReader(SERIAL_PORT, SERIAL_BAUD)
    if not reader.open():
        print("[FAIL] 串口打开失败")
        return

    show_letters()
    print_stats()

    print("\n提示: 输入字母选择手势 → 输入采集轮数 → 保持手势自动采集")
    print("      exit=退出  stats=统计  list=手势列表\n")

    try:
        while True:
            # --- 选择字母 ---
            letter = None
            while letter is None:
                resp = input("选择字母: ").strip().upper()
                if resp in ('EXIT', 'QUIT', ''):
                    reader.close()
                    print("\n已退出")
                    return
                if resp == 'STATS':
                    print_stats()
                    continue
                if resp == 'LIST':
                    show_letters()
                    continue
                if resp in LABELS:
                    letter = resp
                else:
                    print(f"  无效字母 '{resp}'")

            existing = get_count(letter)
            g = GESTURES[letter]
            dyn_tag = " [动态]" if g["type"] == "dynamic" else ""
            print(f"\n[{letter}] {g['desc']}{dyn_tag} | 已有 {existing} 样本")
            if g["type"] == "dynamic":
                print("  *** 动态手势! 每轮请重做完整轨迹 ***")

            # --- 选择轮数 ---
            n_rounds = 0
            while n_rounds <= 0:
                resp = input(f"采集几轮? [1-20, 默认5]: ").strip()
                if resp == '':
                    n_rounds = 5
                elif resp.upper() in ('EXIT', 'QUIT'):
                    reader.close()
                    return
                elif resp.upper() == 'STATS':
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
            print(f"[{letter}] 批量采集 x{n_rounds} 轮")
            if g["type"] == "static":
                print("保持手势不动, 脚本自动连续采集...")
            print(f"{'='*50}")

            success = 0
            for rnd in range(n_rounds):
                # 动态手势提示
                if g["type"] == "dynamic":
                    input(f"第 {rnd+1}/{n_rounds} 轮: 请做好 [{letter}] 手势, 回车开始...")

                result = collect_sample(reader, letter, quiet=True)
                if result[0] is None:
                    print(f"  第 {rnd+1}/{n_rounds} 轮超时, 跳过")
                    continue

                frames, elapsed = result
                cnt = get_count(letter) + 1
                save_sample(letter, frames, cnt)
                success += 1

            print(f"\n[{letter}] 完成 {success}/{n_rounds} 轮, 共 {get_count(letter)} 样本")
            print_stats()

    except KeyboardInterrupt:
        print("\n")
    finally:
        reader.close()
        print_stats()
        print("已退出")


if __name__ == '__main__':
    main()
