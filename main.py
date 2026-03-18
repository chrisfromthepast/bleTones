import asyncio
import eel
from bleak import BleakScanner

eel.init('web')


def detection_callback(device, advertisement_data):
    address = device.address
    name = device.name or 'Unknown'
    rssi = advertisement_data.rssi
    # eel.funcName(args)() — the trailing () executes the JS call synchronously
    eel.processBleSignal(address, name, rssi)()


async def ble_scan_loop():
    async with BleakScanner(detection_callback):
        while True:
            await asyncio.sleep(1)


async def main():
    eel.start('index.html', size=(1024, 768), block=False)
    await ble_scan_loop()


if __name__ == '__main__':
    asyncio.run(main())
