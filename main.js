const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');
const noble = require('@abandonware/noble');

let mainWindow;
let isScanning = false;

function createWindow() {
    mainWindow = new BrowserWindow({
        width: 1200,
        height: 800,
        webPreferences: {
            nodeIntegration: false,
            contextIsolation: true,
            preload: path.join(__dirname, 'preload.js')
        },
        title: 'bleTones - Bluetooth Sound Experience',
        backgroundColor: '#1a1a2e'
    });

    mainWindow.loadFile('electron.html');
    
    // Open DevTools in development
    // mainWindow.webContents.openDevTools();

    mainWindow.on('closed', () => {
        mainWindow = null;
        if (isScanning) {
            noble.stopScanning();
        }
    });
}

app.whenReady().then(createWindow);

app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') {
        app.quit();
    }
});

app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
        createWindow();
    }
});

// Handle BLE scanning requests from renderer
ipcMain.handle('start-ble-scan', async () => {
    return new Promise((resolve, reject) => {
        console.log('Starting BLE scan...');
        
        noble.on('stateChange', async (state) => {
            console.log('Noble state changed:', state);
            if (state === 'poweredOn') {
                try {
                    await noble.startScanningAsync([], true); // Allow duplicates
                    isScanning = true;
                    console.log('BLE scanning started successfully');
                    resolve({ success: true, message: 'BLE scanning started' });
                } catch (error) {
                    console.error('Error starting scan:', error);
                    reject(error);
                }
            } else {
                reject(new Error(`Bluetooth is ${state}. Please turn on Bluetooth.`));
            }
        });
        
        // Handle discovery
        noble.on('discover', (peripheral) => {
            const rssi = peripheral.rssi;
            const name = peripheral.advertisement.localName || 'Unknown';
            const id = peripheral.id || peripheral.address;
            
            // Send to renderer
            if (mainWindow) {
                mainWindow.webContents.send('ble-advertisement', {
                    id: id,
                    name: name,
                    rssi: rssi
                });
            }
        });
        
        // Trigger state check
        if (noble.state === 'poweredOn') {
            noble.startScanningAsync([], true)
                .then(() => {
                    isScanning = true;
                    console.log('BLE scanning started successfully');
                    resolve({ success: true, message: 'BLE scanning started' });
                })
                .catch(reject);
        }
    });
});

ipcMain.handle('stop-ble-scan', async () => {
    if (isScanning) {
        await noble.stopScanning();
        isScanning = false;
        console.log('BLE scanning stopped');
    }
    return { success: true, message: 'BLE scanning stopped' };
});

ipcMain.handle('check-ble-support', async () => {
    return {
        supported: true,
        state: noble.state
    };
});
