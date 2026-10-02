"""录制F4原始帧数据到文件 — 用于离线分析数据质量
用法: python3 record_session.py [输出文件名]
默认: session_frames.csv
每行: timestamp,mode,adc1,adc2,adc3,adc4,adc5,ax,ay,az,gx,gy,gz
mode: 0=未知, 1=手势(0xFF), 2=拼音(0xEE), 3=语音(0xFE)
"""
import os
import sys
import time

SERIAL_PORT = '/dev/ttySTM9'
SERIAL_BAUD = 115200
NUM_FEATURES = 11


class SerialReader:
    def __init__(self):
        self.buf = bytearray()
        self.frame_ok = 0
        self.frame_fail = 0

    def open(self):
        try:
            import serial
            self.ser = serial.Serial(SERIAL_PORT, SERIAL_BAUD, timeout=0.01)
            self._backend = 'pyserial'
            print(f'[串口] {SERIAL_PORT} @ {SERIAL_BAUD} (pyserial)')
            return True
        except Exception:
            pass
        try:
            import termios
            self.fd = os.open(SERIAL_PORT, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
            tty = termios.tcgetattr(self.fd)
            tty[4] = termios.B115200
            tty[5] = tty[4]
            tty[2] = termios.CLOCAL | termios.CREAD | termios.CS8
            tty[0] = 0; tty[1] = 0; tty[3] = 0
            tty[6][termios.VMIN] = 0
            tty[6][termios.VTIME] = 0
            termios.tcsetattr(self.fd, termios.TCSANOW, tty)
            self._backend = 'termios'
            print(f'[串口] {SERIAL_PORT} @ {SERIAL_BAUD} (termios)')
            return True
        except Exception as e:
            print(f'[串口] 打开失败: {e}')
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
        self._read_bytes()
        # 清前导零
        zero_end = 0
        for b in self.buf:
            if b == 0x00: zero_end += 1
            else: break
        if zero_end > 0:
            del self.buf[:zero_end]

        mode_val = 0
        while len(self.buf) > 0 and self.buf[0] in (0xFF, 0xEE, 0xFE):
            b = self.buf[0]
            mode_val = {0xFF: 1, 0xEE: 2, 0xFE: 3}[b]
            del self.buf[0]

        aa_idx = -1
        for i in range(len(self.buf)):
            if self.buf[i] == 0xAA:
                aa_idx = i
                break
        if aa_idx < 0:
            if len(self.buf) > 500: self.buf.clear()
            return mode_val, None
        if aa_idx > 0:
            del self.buf[:aa_idx]

        bb_idx = -1
        for i in range(1, len(self.buf)):
            if self.buf[i] == 0xBB:
                bb_idx = i
                break
        if bb_idx < 0:
            return mode_val, None

        payload = bytes(self.buf[1:bb_idx])
        del self.buf[:bb_idx + 1]

        try:
            text = payload.decode('ascii', errors='ignore').strip()
            if not text:
                self.frame_fail += 1
                return mode_val, None
            parts = text.split(',')
            if len(parts) < NUM_FEATURES:
                self.frame_fail += 1
                return mode_val, None
            frame = [float(p) for p in parts[:NUM_FEATURES]]
            self.frame_ok += 1
            return mode_val, frame
        except (ValueError, UnicodeDecodeError):
            self.frame_fail += 1
            return mode_val, None

    def close(self):
        if self._backend == 'pyserial':
            self.ser.close()
        elif hasattr(self, 'fd') and self.fd is not None:
            try: os.close(self.fd)
            except OSError: pass


def main():
    outfile = sys.argv[1] if len(sys.argv) > 1 else 'session_frames.csv'
    print(f'=== F4 数据录制 ===')
    print(f'输出: {outfile}')
    print(f'每行: timestamp, mode, {",".join(f"adc{i+1}" for i in range(5))}, ax, ay, az, gx, gy, gz')
    print('按 Ctrl+C 停止\n')

    reader = SerialReader()
    if not reader.open():
        return

    f = open(outfile, 'w')
    f.write('timestamp,mode,adc1,adc2,adc3,adc4,adc5,ax,ay,az,gx,gy,gz\n')
    t0 = time.time()
    last_print = time.time()
    last_mode = 0

    try:
        while True:
            mode_val, frame = reader.read_frame()
            if frame is None:
                time.sleep(0.001)
                continue

            elapsed = time.time() - t0
            f.write(f'{elapsed:.6f},{mode_val},{",".join(f"{v:.0f}" for v in frame)}\n')

            if mode_val != 0 and mode_val != last_mode:
                last_mode = mode_val
                mode_names = {0: '?', 1: '手势', 2: '拼音', 3: '语音'}
                name = mode_names.get(mode_val, str(mode_val))
                print(f'\n>>> 模式: {name} (帧#{reader.frame_ok})')

            now = time.time()
            if now - last_print >= 1.0:
                print(f'\r已录 {reader.frame_ok} 帧, 失败 {reader.frame_fail}, '
                      f'已运行 {elapsed:.0f}s', end='', flush=True)
                last_print = now

    except KeyboardInterrupt:
        pass
    finally:
        f.close()
        reader.close()
        elapsed = time.time() - t0
        print(f'\n\n录制完成!')
        print(f'总帧数: {reader.frame_ok}, 失败: {reader.frame_fail}')
        print(f'时长: {elapsed:.0f}s')
        print(f'文件: {outfile} ({os.path.getsize(outfile)} bytes)')


if __name__ == '__main__':
    main()
