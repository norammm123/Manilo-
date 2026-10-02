"""Low-loss HC-08/F4 recorder for STM32MP2 (standard library only).

Accepted frames:
    0xAA + CSV(11 features) + 0xBB
    0xAA + CSV(11 features + uint16 sequence) + 0xBB

Usage:
    python3 record_session_v2.py [output.csv] [serial_port]
"""

import array
import fcntl
import os
import queue
import select
import sys
import termios
import threading
import time


BAUD = 115200
FEATURE_COUNT = 11
MAX_PAYLOAD = 159
READ_SIZE = 8192
MODE_BYTES = {0xFF: 1, 0xEE: 2, 0xFE: 3}
TIOCGICOUNT = 0x545D


def uart_counters(fd):
    """Return Linux UART driver counters when ttySTM supports TIOCGICOUNT."""
    values = array.array("i", [0] * 20)
    try:
        fcntl.ioctl(fd, TIOCGICOUNT, values, True)
    except OSError:
        return None
    return {
        "rx": values[4],
        "frame": values[6],
        "overrun": values[7],
        "parity": values[8],
        "brk": values[9],
        "buf_overrun": values[10],
    }


def counter_delta(current, baseline, name):
    if current is None or baseline is None:
        return 0
    return current[name] - baseline[name]


def open_serial(port):
    fd = os.open(port, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
    try:
        if hasattr(termios, "TIOCEXCL"):
            fcntl.ioctl(fd, termios.TIOCEXCL)

        attrs = termios.tcgetattr(fd)
        attrs[0] = 0
        attrs[1] = 0
        attrs[2] = termios.CLOCAL | termios.CREAD | termios.CS8
        attrs[3] = 0
        attrs[4] = termios.B115200
        attrs[5] = termios.B115200
        attrs[6][termios.VMIN] = 0
        attrs[6][termios.VTIME] = 0
        termios.tcsetattr(fd, termios.TCSANOW, attrs)
        termios.tcflush(fd, termios.TCIFLUSH)
        return fd
    except Exception:
        os.close(fd)
        raise


class SerialPump(threading.Thread):
    """Continuously drain the kernel tty buffer into a large userspace queue."""

    def __init__(self, fd, chunks, stop_event):
        super().__init__(daemon=True)
        self.fd = fd
        self.chunks = chunks
        self.stop_event = stop_event
        self.host_dropped_bytes = 0
        self.error = None

    def run(self):
        poller = select.poll()
        poller.register(self.fd, select.POLLIN | select.POLLERR | select.POLLHUP)
        try:
            while not self.stop_event.is_set():
                events = poller.poll(100)
                if not events:
                    continue

                for _, event in events:
                    if event & (select.POLLERR | select.POLLHUP):
                        raise OSError("serial device reported POLLERR/POLLHUP")

                    while True:
                        try:
                            data = os.read(self.fd, READ_SIZE)
                        except BlockingIOError:
                            break
                        if not data:
                            break
                        try:
                            self.chunks.put_nowait(data)
                        except queue.Full:
                            self.host_dropped_bytes += len(data)
        except Exception as exc:
            self.error = exc
            self.stop_event.set()


class FrameParser:
    def __init__(self):
        self.buffer = bytearray()
        self.current_mode = 0
        self.ok = 0
        self.fail = 0
        self.resync = 0
        self.noise_bytes = 0

    def _discard_prefix(self, length):
        for value in self.buffer[:length]:
            mode = MODE_BYTES.get(value)
            if mode is not None:
                self.current_mode = mode
            elif value != 0:
                self.noise_bytes += 1
        del self.buffer[:length]

    def feed(self, data):
        self.buffer.extend(data)
        result = []

        while True:
            head = self.buffer.find(b"\xAA")
            if head < 0:
                self._discard_prefix(len(self.buffer))
                break
            if head > 0:
                self._discard_prefix(head)

            tail = self.buffer.find(b"\xBB", 1)
            nested_head = self.buffer.find(b"\xAA", 1)

            # ASCII payload cannot contain 0xAA. A newer head before the tail
            # means that the previous tail/payload was damaged; preserve the
            # newer frame instead of swallowing two frames as the old parser did.
            if nested_head >= 0 and (tail < 0 or nested_head < tail):
                self.fail += 1
                self.resync += 1
                del self.buffer[:nested_head]
                continue

            if tail < 0:
                if len(self.buffer) > MAX_PAYLOAD + 2:
                    self.fail += 1
                    self.resync += 1
                    del self.buffer[0]
                    continue
                break

            payload = bytes(self.buffer[1:tail])
            del self.buffer[:tail + 1]

            try:
                parts = payload.decode("ascii").split(",")
                if len(parts) not in (FEATURE_COUNT, FEATURE_COUNT + 1):
                    raise ValueError("unexpected field count")
                features = [int(value) for value in parts[:FEATURE_COUNT]]
                sequence = int(parts[FEATURE_COUNT]) & 0xFFFF if len(parts) == 12 else None
            except (UnicodeDecodeError, ValueError):
                self.fail += 1
                continue

            self.ok += 1
            result.append((self.current_mode, sequence, features))

        return result


class LossCounter:
    def __init__(self):
        self.last = None
        self.lost = 0
        self.duplicates = 0
        self.restarts_or_reordered = 0

    def update(self, sequence):
        if sequence is None:
            return
        if self.last is not None:
            distance = (sequence - self.last) & 0xFFFF
            if distance == 0:
                self.duplicates += 1
            elif distance < 0x8000:
                self.lost += distance - 1
            else:
                self.restarts_or_reordered += 1
        self.last = sequence


def main():
    outfile = sys.argv[1] if len(sys.argv) > 1 else "session_frames_v2.csv"
    port = sys.argv[2] if len(sys.argv) > 2 else "/dev/ttySTM9"

    try:
        fd = open_serial(port)
    except Exception as exc:
        print(f"[串口] {port} 打开失败: {exc}")
        return 1

    baseline_uart = uart_counters(fd)
    chunks = queue.Queue(maxsize=1024)
    stop_event = threading.Event()
    pump = SerialPump(fd, chunks, stop_event)
    parser = FrameParser()
    loss = LossCounter()
    pump.start()

    started = time.monotonic()
    last_report = started
    last_ok = 0
    pending_lines = []

    print(f"[串口] {port} @ {BAUD} (termios/poll, exclusive)")
    print(f"[输出] {outfile}")
    print("按 Ctrl+C 停止")

    try:
        with open(outfile, "w", encoding="ascii", newline="") as output:
            output.write(
                "timestamp,mode,sequence,adc1,adc2,adc3,adc4,adc5,"
                "ax,ay,az,gx,gy,gz\n"
            )

            while not stop_event.is_set():
                try:
                    chunk = chunks.get(timeout=0.1)
                except queue.Empty:
                    chunk = b""

                if chunk:
                    for mode, sequence, features in parser.feed(chunk):
                        loss.update(sequence)
                        elapsed = time.monotonic() - started
                        sequence_text = "" if sequence is None else str(sequence)
                        pending_lines.append(
                            f"{elapsed:.6f},{mode},{sequence_text},"
                            + ",".join(str(value) for value in features)
                            + "\n"
                        )

                now = time.monotonic()
                if len(pending_lines) >= 256:
                    output.writelines(pending_lines)
                    pending_lines.clear()

                if now - last_report >= 1.0:
                    if pending_lines:
                        output.writelines(pending_lines)
                        pending_lines.clear()

                    interval = now - last_report
                    current_uart = uart_counters(fd)
                    expected = parser.ok + loss.lost
                    loss_rate = 100.0 * loss.lost / expected if expected else 0.0
                    uart_text = ""
                    if current_uart is not None:
                        uart_text = (
                            f"  uart_ore={counter_delta(current_uart, baseline_uart, 'overrun')}"
                            f" frame={counter_delta(current_uart, baseline_uart, 'frame')}"
                            f" buf_ore={counter_delta(current_uart, baseline_uart, 'buf_overrun')}"
                        )
                    print(
                        f"\rrx={parser.ok:7d}  fps={(parser.ok - last_ok) / interval:6.1f}  "
                        f"lost={loss.lost:6d} ({loss_rate:5.2f}%)  "
                        f"parse_fail={parser.fail:5d}  host_drop={pump.host_dropped_bytes:5d}"
                        f"{uart_text}",
                        end="",
                        flush=True,
                    )
                    last_ok = parser.ok
                    last_report = now

            if pending_lines:
                output.writelines(pending_lines)
    except KeyboardInterrupt:
        pass
    finally:
        stop_event.set()
        pump.join(timeout=0.3)
        os.close(fd)

    elapsed = time.monotonic() - started
    expected = parser.ok + loss.lost
    loss_rate = 100.0 * loss.lost / expected if expected else 0.0
    print(
        f"\n完成: rx={parser.ok}, lost={loss.lost} ({loss_rate:.2f}%), "
        f"parse_fail={parser.fail}, resync={parser.resync}, "
        f"host_drop_bytes={pump.host_dropped_bytes}, elapsed={elapsed:.1f}s"
    )
    if pump.error is not None:
        print(f"[读取线程错误] {pump.error}")
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
