"""F4串口数据测试 — 实时显示接收到的帧数据"""
import os
import sys
import time
import struct
from collections import deque

SERIAL_PORT = '/dev/ttySTM9'
SERIAL_BAUD = 115200
NUM_FEATURES = 11

MODE_NAMES = {0xFF: '手势', 0xEE: '拼音', 0xFE: '语音'}
FEAT_NAMES = ['adc1中指', 'adc2拇指', 'adc3食指', 'adc4无名', 'adc5小指',
              'ax', 'ay', 'az', 'gx', 'gy', 'gz']


class SerialReader:
    def __init__(self):
        self.fd = None
        self.buf = bytearray()
        self.frame_ok = 0
        self.frame_fail = 0

    def open(self):
        try:
            import serial
            self.ser = serial.Serial(port=SERIAL_PORT, baudrate=SERIAL_BAUD, timeout=0.01)
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

    def read_bytes(self):
        try:
            if self._backend == 'pyserial':
                while self.ser.in_waiting:
                    b = self.ser.read(1)
                    if b:
                        self.buf.append(b[0])
            else:
                data = os.read(self.fd, 512)
                if data:
                    self.buf.extend(data)
        except (BlockingIOError, OSError):
            pass

    def read_frame(self):
        self.read_bytes()

        # 跳过开头噪声
        while len(self.buf) > 0 and self.buf[0] in (0x00, 0xFF, 0xEE, 0xFE):
            # 0xFF/0xEE/0xFE可能是模式字节，保存下来
            pass

        # 找模式字节 + 帧头 0xAA
        mode = None
        for i in range(len(self.buf) - 1):
            if self.buf[i] in (0xFF, 0xEE, 0xFE) and self.buf[i + 1] == 0xAA:
                mode = self.buf[i]
                del self.buf[:i]
                break
            elif self.buf[i] == 0xAA:
                mode = None  # 无模式字节
                del self.buf[:i]
                break

        if len(self.buf) < 2:
            return None, None

        if self.buf[0] == 0xAA:
            # 找帧尾 0xBB
            bb = -1
            for i in range(1, len(self.buf)):
                if self.buf[i] == 0xBB:
                    bb = i
                    break
            if bb < 0:
                if len(self.buf) > 1000:
                    self.buf.clear()
                return None, None

            payload = bytes(self.buf[1:bb])
            del self.buf[:bb + 1]

            try:
                text = payload.decode('ascii', errors='ignore').strip()
                if not text:
                    self.frame_fail += 1
                    return None, None
                parts = text.split(',')
                if len(parts) < NUM_FEATURES:
                    self.frame_fail += 1
                    return None, None
                frame = [float(p) for p in parts[:NUM_FEATURES]]
                self.frame_ok += 1
                return mode, frame
            except (ValueError, UnicodeDecodeError):
                self.frame_fail += 1
                return None, None
        else:
            del self.buf[0]
            return None, None


def main():
    print('=== F4 串口数据测试 ===')
    print(f'串口: {SERIAL_PORT} @ {SERIAL_BAUD}')
    print(f'特征: {NUM_FEATURES}维')
    print()

    # Windows 用 COM 端口
    if sys.platform == 'win32':
        import serial
        ser = serial.Serial('COM10', SERIAL_BAUD, timeout=0.01)
        buf = bytearray()
        ok = fail = 0
        last_print = time.time()
        last_mode = None
        frame_count = 0
        fps_counter = deque(maxlen=50)

        print(f'[串口] COM10 @ {SERIAL_BAUD} (pyserial)')
        print()

        try:
            while True:
                # 读取
                while ser.in_waiting:
                    b = ser.read(1)
                    if b:
                        buf.append(b[0])

                # 找帧头
                mode = None
                aa_idx = -1
                for i in range(len(buf)):
                    if i > 0 and buf[i - 1] in (0xFF, 0xEE, 0xFE) and buf[i] == 0xAA:
                        mode = buf[i - 1]
                        aa_idx = i
                        break
                    elif buf[i] == 0xAA:
                        aa_idx = i
                        break

                if aa_idx < 0:
                    if len(buf) > 2000:
                        buf.clear()
                    time.sleep(0.001)
                    continue

                if aa_idx > 1 and mode is None:
                    del buf[:aa_idx - 1]  # 保留可能的前一个模式字节
                    continue
                if aa_idx > 0 and mode is not None:
                    del buf[:aa_idx - 1]
                else:
                    del buf[:aa_idx]

                # 找帧尾
                bb = -1
                for i in range(1, min(len(buf), 500)):
                    if buf[i] == 0xBB:
                        bb = i
                        break

                if bb < 0:
                    if len(buf) > 500:
                        buf.clear()
                    time.sleep(0.001)
                    continue

                payload = bytes(buf[1:bb])
                del buf[:bb + 1]

                try:
                    text = payload.decode('ascii', errors='ignore').strip()
                    if not text:
                        fail += 1
                        continue
                    parts = text.split(',')
                    if len(parts) < NUM_FEATURES:
                        fail += 1
                        continue
                    frame = [float(p) for p in parts[:NUM_FEATURES]]
                    ok += 1
                    frame_count += 1
                    fps_counter.append(time.time())

                    # 模式切换提示
                    if mode is not None and mode != last_mode:
                        last_mode = mode
                        name = MODE_NAMES.get(mode, f'0x{mode:02X}')
                        print(f'\n>>> 模式切换: {name} <<<')

                    # 每秒打印一次统计
                    now = time.time()
                    if now - last_print >= 0.5:
                        # 计算FPS
                        if len(fps_counter) >= 2:
                            fps = (len(fps_counter) - 1) / (fps_counter[-1] - fps_counter[0])
                        else:
                            fps = 0

                        name = MODE_NAMES.get(last_mode, '?')
                        adc_str = ' '.join(f'{frame[i]:>7.0f}' for i in range(5))
                        acc_str = ' '.join(f'{frame[i]:>7.0f}' for i in range(5, 8))
                        gyr_str = ' '.join(f'{frame[i]:>7.0f}' for i in range(8, 11))

                        print(f'\r[{name}] FPS={fps:4.0f} | OK={ok} FAIL={fail} | '
                              f'ADC:[{adc_str}] ACC:[{acc_str}] GYR:[{gyr_str}]',
                              end='', flush=True)
                        last_print = now

                except (ValueError, UnicodeDecodeError):
                    fail += 1
                    continue

        except KeyboardInterrupt:
            print(f'\n\n总计: OK={ok} FAIL={fail}')
    else:
        # Linux 板端
        reader = SerialReader()
        if not reader.open():
            return

        ok = fail = 0
        last_print = time.time()
        last_mode = None

        print('等待F4数据... (Ctrl+C退出)\n')

        try:
            while True:
                mode, frame = reader.read_frame()
                if frame is None:
                    time.sleep(0.001)
                    continue

                ok += 1

                if mode is not None and mode != last_mode:
                    last_mode = mode
                    name = MODE_NAMES.get(mode, f'0x{mode:02X}')
                    print(f'\n>>> 模式切换: {name} <<<')

                now = time.time()
                if now - last_print >= 0.5:
                    name = MODE_NAMES.get(last_mode, '?')
                    adc_str = ' '.join(f'{frame[i]:>7.0f}' for i in range(5))
                    acc_str = ' '.join(f'{frame[i]:>7.0f}' for i in range(5, 8))
                    gyr_str = ' '.join(f'{frame[i]:>7.0f}' for i in range(8, 11))

                    print(f'\r[{name}] OK={ok} FAIL={fail} | '
                          f'ADC:[{adc_str}] ACC:[{acc_str}] GYR:[{gyr_str}]',
                          end='', flush=True)
                    last_print = now

        except KeyboardInterrupt:
            print(f'\n\n总计: OK={ok} FAIL={reader.frame_fail}')


if __name__ == '__main__':
    main()
