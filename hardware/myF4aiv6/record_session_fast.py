"""High-speed STM32MP2 recorder for F4 -> HC-08 -> ttySTM9.

Capture phase only drains raw UART bytes into RAM. CSV parsing and file output
happen after Ctrl+C, so parsing/disk latency cannot make the UART reader fall
behind during a short data-collection session.

Accepted F4 frames:
    0xAA + CSV(11 features) + 0xBB
    0xAA + CSV(11 features + uint16 sequence) + 0xBB

Usage:
    python3 record_session_fast.py [output.csv] [serial_port]

Example:
    python3 record_session_fast.py session_A_to_Z.csv /dev/ttySTM9
"""

import array
import fcntl
import os
import select
import sys
import termios
import time


BAUD = 115200
SOURCE_HZ = 100.0
FEATURE_COUNT = 11
MAX_PAYLOAD = 159
READ_SIZE = 65536
TIOCGICOUNT = 0x545D
MODE_BYTES = {0xFF: 1, 0xEE: 2, 0xFE: 3}


def uart_counters(fd):
    """Read Linux serial_icounter_struct when supported by the tty driver."""
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


def delta(current, baseline, field):
    if current is None or baseline is None:
        return 0
    return current[field] - baseline[field]


def open_uart(port):
    fd = os.open(port, os.O_RDONLY | os.O_NOCTTY | os.O_NONBLOCK)
    try:
        # Prevent another process from opening the tty after this recorder.
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


def capture(fd):
    poller = select.poll()
    poller.register(fd, select.POLLIN | select.POLLERR | select.POLLHUP)
    raw = bytearray()
    started = time.monotonic()
    last_report = started
    last_size = 0
    baseline = uart_counters(fd)

    print("[采集] 仅接收原始字节；按 Ctrl+C 停止并离线解析")
    try:
        while True:
            events = poller.poll(100)
            for _, event in events:
                if event & select.POLLHUP:
                    raise OSError("串口设备断开(POLLHUP)")

                # Drain until EAGAIN instead of reading only once per loop.
                while True:
                    try:
                        chunk = os.read(fd, READ_SIZE)
                    except BlockingIOError:
                        break
                    if not chunk:
                        break
                    raw.extend(chunk)

            now = time.monotonic()
            if now - last_report >= 1.0:
                current = uart_counters(fd)
                interval = now - last_report
                byte_rate = (len(raw) - last_size) / interval
                uart_text = ""
                if current is not None:
                    uart_text = (
                        f"  uart_ore={delta(current, baseline, 'overrun')}"
                        f" frame={delta(current, baseline, 'frame')}"
                        f" parity={delta(current, baseline, 'parity')}"
                        f" buf_ore={delta(current, baseline, 'buf_overrun')}"
                    )
                print(
                    f"\rraw={len(raw):9d} B  rate={byte_rate:8.1f} B/s{uart_text}",
                    end="",
                    flush=True,
                )
                last_size = len(raw)
                last_report = now
    except KeyboardInterrupt:
        pass

    elapsed = time.monotonic() - started
    final_counters = uart_counters(fd)
    print(f"\n[采集结束] {len(raw)}字节, {elapsed:.2f}秒, 平均{len(raw) / max(elapsed, 1e-9):.1f} B/s")
    return raw, elapsed, baseline, final_counters


def parse_frames(raw):
    frames = []
    mode = 0
    parse_fail = 0
    resync = 0
    noise = 0
    position = 0
    size = len(raw)

    while position < size:
        head = raw.find(b"\xAA", position)
        if head < 0:
            for value in raw[position:]:
                if value in MODE_BYTES:
                    mode = MODE_BYTES[value]
                elif value != 0:
                    noise += 1
            break

        for value in raw[position:head]:
            if value in MODE_BYTES:
                mode = MODE_BYTES[value]
            elif value != 0:
                noise += 1

        tail = raw.find(b"\xBB", head + 1)
        nested_head = raw.find(b"\xAA", head + 1)

        # ASCII payload never contains 0xAA. If a new head appears before the
        # tail, discard only the damaged frame and preserve the new one.
        if nested_head >= 0 and (tail < 0 or nested_head < tail):
            parse_fail += 1
            resync += 1
            position = nested_head
            continue

        if tail < 0:
            if size - head > MAX_PAYLOAD + 2:
                parse_fail += 1
                resync += 1
            break

        if tail - head - 1 > MAX_PAYLOAD:
            parse_fail += 1
            resync += 1
            position = head + 1
            continue

        payload = bytes(raw[head + 1:tail])
        position = tail + 1
        try:
            parts = payload.decode("ascii").split(",")
            if len(parts) not in (FEATURE_COUNT, FEATURE_COUNT + 1):
                raise ValueError("字段数量错误")
            features = [int(value) for value in parts[:FEATURE_COUNT]]
            sequence = int(parts[FEATURE_COUNT]) & 0xFFFF if len(parts) == 12 else None
        except (UnicodeDecodeError, ValueError):
            parse_fail += 1
            continue

        frames.append((mode, sequence, features))

    return frames, parse_fail, resync, noise


def analyze_sequences(frames):
    lost = 0
    duplicates = 0
    restart_or_reordered = 0
    last = None
    first = None

    for _, sequence, _ in frames:
        if sequence is None:
            continue
        if first is None:
            first = sequence
        if last is not None:
            distance = (sequence - last) & 0xFFFF
            if distance == 0:
                duplicates += 1
            elif distance < 0x8000:
                lost += distance - 1
            else:
                restart_or_reordered += 1
        last = sequence

    return first, lost, duplicates, restart_or_reordered


def write_outputs(csv_path, raw, frames, first_sequence):
    raw_path = csv_path + ".raw.bin"
    with open(raw_path, "wb", buffering=1024 * 1024) as output:
        output.write(raw)

    with open(csv_path, "w", encoding="ascii", newline="", buffering=1024 * 1024) as output:
        output.write(
            "sample_time,mode,sequence,adc1,adc2,adc3,adc4,adc5,"
            "ax,ay,az,gx,gy,gz\n"
        )
        lines = []
        for index, (mode, sequence, features) in enumerate(frames):
            if sequence is not None and first_sequence is not None:
                sample_index = (sequence - first_sequence) & 0xFFFF
            else:
                sample_index = index
            sample_time = sample_index / SOURCE_HZ
            sequence_text = "" if sequence is None else str(sequence)
            lines.append(
                f"{sample_time:.6f},{mode},{sequence_text},"
                + ",".join(str(value) for value in features)
                + "\n"
            )
            if len(lines) >= 4096:
                output.writelines(lines)
                lines.clear()
        if lines:
            output.writelines(lines)
    return raw_path


def main():
    csv_path = sys.argv[1] if len(sys.argv) > 1 else "session_frames_fast.csv"
    port = sys.argv[2] if len(sys.argv) > 2 else "/dev/ttySTM9"

    print(f"[串口] {port} @ {BAUD} (termios/poll/exclusive)")
    print(f"[输出] {csv_path}")
    try:
        fd = open_uart(port)
    except Exception as exc:
        print(f"[失败] 无法打开串口: {exc}")
        return 1

    try:
        raw, elapsed, baseline, final_counters = capture(fd)
    except Exception as exc:
        print(f"\n[失败] 采集异常: {exc}")
        os.close(fd)
        return 2
    finally:
        try:
            os.close(fd)
        except OSError:
            pass

    print("[解析] 正在离线解析帧...")
    frames, parse_fail, resync, noise = parse_frames(raw)
    first_sequence, lost, duplicates, reordered = analyze_sequences(frames)
    expected = len(frames) + lost
    loss_rate = 100.0 * lost / expected if expected else 0.0
    raw_path = write_outputs(csv_path, raw, frames, first_sequence)

    print(f"[结果] 正确帧: {len(frames)}")
    print(f"[结果] 序号丢帧: {lost} ({loss_rate:.2f}%)")
    print(f"[结果] 解析失败: {parse_fail}, 重同步: {resync}, 噪声字节: {noise}")
    print(f"[结果] 重复: {duplicates}, 复位/乱序: {reordered}")
    print(f"[结果] 接收帧率: {len(frames) / max(elapsed, 1e-9):.2f} fps")
    if final_counters is not None:
        print(
            "[UART] "
            f"overrun={delta(final_counters, baseline, 'overrun')}, "
            f"frame={delta(final_counters, baseline, 'frame')}, "
            f"parity={delta(final_counters, baseline, 'parity')}, "
            f"buf_overrun={delta(final_counters, baseline, 'buf_overrun')}"
        )
    else:
        print("[UART] tty驱动不支持TIOCGICOUNT，无法读取硬件错误计数")
    print(f"[文件] CSV: {csv_path}")
    print(f"[文件] 原始串口数据: {raw_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
