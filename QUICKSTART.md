# Quick Start Guide for bleTones Standalone App

## 🚀 One-Click Launch

### Windows Users
1. Double-click `start.bat`
2. Wait for the app to launch
3. Enjoy!

### Mac/Linux Users
1. Double-click `start.sh` (or run `./start.sh` in terminal)
2. Wait for the app to launch
3. Enjoy!

### Manual Launch (All Platforms)
```bash
npm install
npm start
```

## ⚡ First Time Setup

The first time you run the app, it will:
1. Check for Node.js (install from https://nodejs.org/ if needed)
2. Install dependencies (~2-5 minutes)
3. Launch the bleTones app

**Subsequent launches** are instant - just run the script again!

## 📱 Using the App

1. **Click "Begin Experience"** when the app opens
2. **Choose "📡 BLE Devices"** mode
3. **Click "🔍 Real BLE"** to start scanning
4. The app will automatically detect all nearby Bluetooth devices
5. Move near BLE devices to hear sounds!

### Troubleshooting

**"Bluetooth is unpowered"**
- Turn on Bluetooth in your system settings
- Restart the app

**No sounds playing**
- Make sure Bluetooth devices are nearby (phones, watches, headphones)
- Try clicking "🎭 Demo Mode" to verify audio is working
- Check system volume

**App won't start**
- Ensure Node.js is installed: https://nodejs.org/
- Run `npm install` manually in the app directory
- Check for error messages in the terminal

## 🆚 Standalone App vs Web Browser

| Feature | Standalone App | Web Browser |
|---------|---------------|-------------|
| BLE Scanning | ✅ Full native access | ⚠️ Limited/experimental |
| Setup | One-time npm install | None |
| All nearby devices | ✅ Yes | ❌ No (requires pairing) |
| Browser flags | ✅ Not needed | ⚠️ Required for passive scan |
| Performance | ✅ Excellent | ✅ Good |

**Recommendation:** Use the standalone app for the best BLE experience!
