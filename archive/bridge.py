import asyncio
import signal
from bleak import BleakScanner
from pythonosc import udp_client

# --- CONFIGURATION ---
OSC_IP = "127.0.0.1"
OSC_PORT = 8000
OSC_ADDR = "/ble/rssi"

# Optional: Set to a specific name like "MyDevice" to filter noise
# If None, it sends EVERYTHING it finds
TARGET_DEVICE_NAME = None 

# --- SETUP ---
client = udp_client.SimpleUDPClient(OSC_IP, OSC_PORT)

def normalize_rssi(rssi):
    """Maps -100...-30 dBm to a 0.0...1.0 range for easier use in OSC apps."""
    return max(0.0, min(1.0, (rssi + 100) / 70))

def detection_callback(device, advertisement_data):
    name = device.name or "Unknown"
    address = device.address
    rssi = advertisement_data.rssi
    
    # Filter by name if TARGET_DEVICE_NAME is set
    if TARGET_DEVICE_NAME and TARGET_DEVICE_NAME not in name:
        return

    # Send Address, Raw RSSI, and Normalized (0-1) value
    client.send_message(OSC_ADDR, [address, rssi, normalize_rssi(rssi)])
    
    print(f"📡 {name} [{address}] | RSSI: {rssi} dBm")

async def main():
    print(f"🚀 BLE-to-OSC Bridge Started")
    print(f"📡 Sending to {OSC_IP}:{OSC_PORT} on address '{OSC_ADDR}'")
    print("Press Ctrl+C to stop.\n")

    scanner = BleakScanner(detection_callback=detection_callback)
    
    async with scanner:
        # Keep the scanner running forever
        while True:
            await asyncio.sleep(1)

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n👋 Stopping bridge...")
