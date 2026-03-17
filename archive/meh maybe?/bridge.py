import asyncio
from bleak import BleakScanner
from pythonosc import udp_client

# --- CONFIG ---
OSC_IP = "127.0.0.1"
OSC_PORT = 57120  # DEFAULT SUPERCOLLIDER PORT
OSC_ADDR = "/ble/data"

client = udp_client.SimpleUDPClient(OSC_IP, OSC_PORT)

def normalize_rssi(rssi):
    """Maps -100 (silent/far) to -30 (loud/close) into a 0.0 to 1.0 float."""
    val = (rssi + 100) / 70 
    return max(0.0, min(1.0, val))

def detection_callback(device, advertisement_data):
    address = device.address
    name = device.name or "Unknown"
    rssi = advertisement_data.rssi
    norm_rssi = normalize_rssi(rssi)

    # Sending: Address, Name, Raw RSSI, Normalized RSSI
    client.send_message(OSC_ADDR, [address, name, rssi, norm_rssi])
    print(f"📡 Sent to SC: {name} | Norm: {norm_rssi:.2f}")

async def main():
    print(f"🚀 BLE -> SuperCollider Bridge")
    print(f"Target: {OSC_IP}:{OSC_PORT}")
    
    scanner = BleakScanner(detection_callback=detection_callback)
    async with scanner:
        while True:
            await asyncio.sleep(0.1) # High frequency scanning

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\nStopping...")
