#!/usr/bin/env python3
"""
OSC Test Sender for bleTones

Simulates BLE devices by sending OSC /ble/rssi messages to localhost:9000.
Use this to test bleTones without actual Bluetooth hardware.

Usage:
    python3 osc_test_sender.py              # Run with defaults (continuous)
    python3 osc_test_sender.py --once       # Send once and exit
    python3 osc_test_sender.py --devices 8  # Simulate 8 devices
    python3 osc_test_sender.py --help       # Show all options
"""

import argparse
import math
import random
import socket
import struct
import sys
import time


def osc_string(s: str) -> bytes:
    """Encode a string as an OSC string (null-terminated, 4-byte aligned)."""
    b = s.encode("utf-8") + b"\x00"
    while len(b) % 4 != 0:
        b += b"\x00"
    return b


def osc_int32(val: int) -> bytes:
    """Encode a 32-bit integer as big-endian bytes."""
    return struct.pack(">i", val)


def build_osc_message(device_name: str, rssi: int) -> bytes:
    """Build an OSC /ble/rssi message packet."""
    packet = osc_string("/ble/rssi")
    packet += osc_string(",si")  # Type tag: string, int32
    packet += osc_string(device_name)
    packet += osc_int32(rssi)
    return packet


def send_rssi(sock: socket.socket, addr: tuple, device_name: str, rssi: int, verbose: bool):
    """Send a single /ble/rssi message."""
    packet = build_osc_message(device_name, rssi)
    sock.sendto(packet, addr)
    if verbose:
        print(f"  {device_name}: {rssi} dBm")


def main():
    parser = argparse.ArgumentParser(
        description="Simulate BLE devices for bleTones testing",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python3 osc_test_sender.py              # Run continuously with 4 simulated devices
  python3 osc_test_sender.py --once       # Send one snapshot and exit
  python3 osc_test_sender.py --devices 8  # Simulate 8 devices
  python3 osc_test_sender.py --host 192.168.1.10  # Send to different host
"""
    )
    parser.add_argument("--host", default="127.0.0.1", help="Target host (default: 127.0.0.1)")
    parser.add_argument("--port", type=int, default=9000, help="Target UDP port (default: 9000)")
    parser.add_argument("--devices", type=int, default=4, help="Number of simulated devices (default: 4)")
    parser.add_argument("--interval", type=float, default=0.1, help="Send interval in seconds (default: 0.1)")
    parser.add_argument("--once", action="store_true", help="Send one batch and exit")
    parser.add_argument("-v", "--verbose", action="store_true", help="Print each message sent")
    args = parser.parse_args()

    # Device names to simulate
    device_names = [
        "iPhone",
        "Galaxy Watch",
        "AirPods Pro",
        "iPad",
        "Apple Watch",
        "Pixel Buds",
        "Fitbit",
        "MacBook",
        "Surface Headphones",
        "Tile Tracker",
        "Android Phone",
        "Smart TV",
    ]
    
    # Select subset of devices
    selected_devices = device_names[:args.devices]
    
    # Base RSSI for each device (closer devices = higher RSSI)
    device_base_rssi = {}
    for i, name in enumerate(selected_devices):
        # Spread devices from -40 (very close) to -80 (moderate distance)
        base = -40 - (i * 10)
        device_base_rssi[name] = max(-90, base)

    addr = (args.host, args.port)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    print(f"bleTones OSC Test Sender")
    print(f"========================")
    print(f"Target: {args.host}:{args.port}")
    print(f"Devices: {len(selected_devices)}")
    print(f"Interval: {args.interval}s")
    print()

    if args.once:
        print("Sending single snapshot...")
        for name in selected_devices:
            rssi = device_base_rssi[name] + random.randint(-5, 5)
            send_rssi(sock, addr, name, rssi, verbose=True)
        print("Done.")
        return

    print("Sending continuously (Ctrl+C to stop)...")
    print()

    start_time = time.time()
    iteration = 0
    
    try:
        while True:
            elapsed = time.time() - start_time
            iteration += 1
            
            if args.verbose:
                print(f"[{elapsed:.1f}s] Sending snapshot #{iteration}:")
            
            for name in selected_devices:
                # Simulate RSSI variations:
                # - Slow sine wave (walking around)
                # - Random jitter (measurement noise)
                base = device_base_rssi[name]
                wave = math.sin(elapsed * 0.3 + hash(name) % 100) * 8  # Slow oscillation
                jitter = random.randint(-3, 3)  # Small random noise
                rssi = int(base + wave + jitter)
                rssi = max(-100, min(-30, rssi))  # Clamp to valid range
                
                send_rssi(sock, addr, name, rssi, args.verbose)
            
            if args.verbose:
                print()
            
            time.sleep(args.interval)
            
    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        sock.close()


if __name__ == "__main__":
    main()
