# Quick Start Guide for bleTones

## 🚀 Easiest Way to Start

### Windows Users
1. Double-click `start.bat`
2. Wait for the app to launch
3. Click "Begin Experience" and enjoy!

### Mac/Linux Users
1. Double-click `start.sh` (or run `./start.sh` in terminal)
2. Wait for the app to launch
3. Click "Begin Experience" and enjoy!

### Manual Launch (All Platforms)
```bash
npm install
npm start
```

## ⚡ First Time Setup

The first time you run the app, it will:
1. Check for Node.js (install from https://nodejs.org/ if needed)
2. Install dependencies (~2-5 minutes)
3. Launch the bleTones desktop app

**Subsequent launches** are instant - just run the script again!

## 📱 Using the App

1. Click **"Begin Experience"** to enable audio
2. Choose your mode:
   - **🖱️ Mouse/Touch**: Move your mouse to create sounds
   - **📡 BLE Devices**: Use Bluetooth devices (click "🔍 Real BLE" to scan)
3. Try **"🎭 Demo Mode"** for simulated Bluetooth signals (no real BLE devices needed)
4. Open **⚙️ Settings** to customize instruments, flavors, and parameters

## 🆚 Desktop App vs Web Browser

| Feature | Desktop App | Web Browser |
|---------|-------------|-------------|
| How to run | `npm start` or start.sh/start.bat | Open `index.html` |
| BLE Scanning | ✅ Full native access | ⚠️ Limited/experimental |
| Setup | One-time npm install | None |
| All nearby devices | ✅ Yes | ❌ No (requires pairing or flags) |
| Performance | ✅ Excellent | ✅ Good |
| Settings popup | ✅ Yes | ✅ Yes |

**Recommendation:** Use the desktop app for the best BLE experience!

## 🔧 Troubleshooting

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

**Web browser BLE not working**
- Use the desktop app instead (recommended)
- Or try Demo Mode for simulated BLE experience
- See README.md for advanced browser troubleshooting
