# Python Version Archive - v1.0.0-python

This document provides instructions for accessing and using the archived Python-heavy version of bleTones.

## 📦 Release Information

**Version:** v1.0.0-python  
**Release Date:** March 19, 2026  
**Purpose:** Archive snapshot before major language refactor  
**Status:** Stable, feature-complete, archived

## 🔖 Accessing This Version

### Option 1: Via Git Tag (Recommended)
```bash
# Clone the repository
git clone https://github.com/chrisfromthepast/bleTones.git
cd bleTones

# Checkout the Python version tag
git checkout v1.0.0-python

# Install Python dependencies
pip install -r requirements.txt

# Run the app
python main.py
```

### Option 2: Download Release Archive
1. Visit: https://github.com/chrisfromthepast/bleTones/releases/tag/v1.0.0-python
2. Download the source code (zip or tar.gz)
3. Extract and follow installation instructions in README.md

### Option 3: Clone Specific Commit
```bash
git clone https://github.com/chrisfromthepast/bleTones.git
cd bleTones
git checkout v1.0.0-python
```

## 🚀 Quick Start

### Python Desktop App (Recommended)
```bash
# Install dependencies
pip install eel bleak

# Run the app
python main.py
```

### Electron Desktop App
```bash
# Install dependencies
npm install

# Run the app
npm start
```

### SuperCollider Integration
```bash
# Install Python dependencies
pip install bleak python-osc

# Start BLE bridge
python supercollider/bridge.py

# In SuperCollider, open and evaluate any .scd file from supercollider/ directory
```

## 📚 Documentation

All documentation is included in the release:
- **README.md** - Comprehensive usage guide
- **QUICKSTART.md** - Quick start instructions
- **RELEASE_NOTES.md** - Detailed release notes (this version)
- **requirements.txt** - Python dependencies

## 🎯 What's Preserved

This archive contains:

### Python Components
- `main.py` - Desktop app with eel + bleak
- `supercollider/bridge.py` - OSC bridge for SuperCollider
- `requirements.txt` - All Python dependencies documented

### Node.js/Electron Components
- `main.js` - Electron main process
- `preload.js` - Electron preload script
- `package.json` - Node dependencies and build config
- `start.sh` / `start.bat` - Launcher scripts

### Audio Synthesis
- `index.html` - Complete Web Audio implementation (114KB)
- `web/index.html` - Duplicate for Python app
- 12 unique instruments across 4 flavors
- Real-time BLE device-to-pitch mapping

### SuperCollider Presets
- `woods.scd` - Melodic organic tones
- `flavor-ambient.scd` - Atmospheric sustained sounds
- `flavor-percussive.scd` - Rhythmic percussion
- `flavor-ethereal.scd` - High dreamy tones
- `sines.scd`, `songly.scd`, `sprites.scd` - Experimental presets

### Configuration
- `Info.plist` - macOS Bluetooth permissions
- `.gitignore` - Git ignore rules
- `LICENSE` - MIT License

## 🔧 System Requirements

- **Python:** 3.8 or higher
- **Node.js:** 14 or higher (for Electron app)
- **SuperCollider:** 3.11 or higher (optional, for .scd integration)
- **Bluetooth:** BLE capable hardware
- **OS:** macOS 10.15+, Windows 10+, or modern Linux

## 🐛 Troubleshooting

### Python App Won't Start
```bash
# Ensure Python dependencies are installed
pip install -r requirements.txt

# Verify Python version
python --version  # Should be 3.8+

# Run with verbose output
python main.py
```

### Electron App Won't Start
```bash
# Clean install dependencies
rm -rf node_modules package-lock.json
npm install

# Run
npm start
```

### BLE Scanning Not Working
- **macOS:** Ensure Info.plist is properly configured
- **All platforms:** Enable Bluetooth in system settings
- **Try Demo Mode:** Click "🎭 Demo Mode" to verify audio works

## 🔄 Migrating From This Version

If the repository has been refactored to a new language after this release:

1. **To use the Python version:** Checkout tag `v1.0.0-python`
2. **To use the new version:** Checkout `main` branch
3. **To compare:** Use git diff or compare branches

```bash
# Stay on Python version
git checkout v1.0.0-python

# Switch to new version
git checkout main

# Compare changes
git diff v1.0.0-python main
```

## 📝 Why This Archive Exists

This release was created to preserve the fully functional Python-based implementation before a major repository refactor to experiment with a new language. All features documented here represent the stable, production-ready Python/JavaScript/Node.js version of bleTones.

## 🤝 Contributing

For contributions to this archived version:
1. Fork the repository
2. Create a branch from tag `v1.0.0-python`
3. Make your changes
4. Submit PR targeting a new branch (not main)

Note: Active development may have moved to a new implementation. Check the main branch for current development.

## 📧 Support

For issues specific to this Python version:
- Check documentation: README.md, QUICKSTART.md
- Review RELEASE_NOTES.md for known limitations
- Open GitHub issue with tag `v1.0.0-python`

## 🔗 Related Resources

- **GitHub Repository:** https://github.com/chrisfromthepast/bleTones
- **Python eel:** https://github.com/python-eel/Eel
- **Bleak (Python BLE):** https://github.com/hbldh/bleak
- **SuperCollider:** https://supercollider.github.io/
- **Web Audio API:** https://developer.mozilla.org/en-US/docs/Web/API/Web_Audio_API

---

**Archived by:** Copilot Workspace  
**Archive Date:** March 19, 2026  
**License:** MIT (see LICENSE file)
