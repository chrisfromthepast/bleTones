import asyncio
from bleak import BleakScanner
from pythonosc import udp_client

# --- CONFIG ---
OSC_IP = "127.0.0.1"
OSC_PORT = 57120
OSC_ADDR = "/ble/data"

client = udp_client.SimpleUDPClient(OSC_IP, OSC_PORT)
last_sent_rssi = {} # Prevents flooding SC with identical data

def normalize_rssi(rssi):
    # Map -100 (far) to -30 (close) -> 0.0 to 1.0
    val = (rssi + 100) / 70 
    return float(max(0.0, min(1.0, val)))

def detection_callback(device, advertisement_data):
    addr = str(device.address)
    rssi = int(advertisement_data.rssi)
    
    # 🚨 STABILITY: Only send if signal changed significantly
    prev = last_sent_rssi.get(addr, 0)
    if abs(rssi - prev) > 2:
        norm = normalize_rssi(rssi)
        # Send: [Address, Name, Raw_RSSI, Normalized_0to1]
        client.send_message(OSC_ADDR, [addr, str(device.name or "???"), rssi, norm])
        last_sent_rssi[addr] = rssi
        print(f"📡 Sent: {addr} | Norm: {norm:.2f}")

async def main():
    print(f"🚀 Smooth Bridge Active. Sending to SC on {OSC_PORT}")
    scanner = BleakScanner(detection_callback=detection_callback)
    async with scanner:
        while True:
            await asyncio.sleep(0.1) # 10Hz scanning is plenty

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nStopping...")
