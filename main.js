const { app, BrowserWindow, ipcMain } = require('electron');
const path = require('path');
const noble = require('@abandonware/noble');

let mainWindow;
let isScanning = false;
let bleInitialized = false;

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

// Initialize Noble event listeners once
function initializeBLE() {
    if (bleInitialized) return;
    bleInitialized = true;
    
    noble.on('stateChange', async (state) => {
        console.log('Noble state changed:', state);
    });
    
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
    console.log('Starting BLE scan...');
    
    // Initialize BLE event listeners (only once)
    initializeBLE();
    
    return new Promise((resolve, reject) => {
        if (noble.state === 'poweredOn') {
            noble.startScanningAsync([], true) // Allow duplicates
                .then(() => {
                    isScanning = true;
                    console.log('BLE scanning started successfully');
                    resolve({ success: true, message: 'BLE scanning started' });
                })
                .catch(reject);
        } else if (noble.state === 'poweredOff' || noble.state === 'unsupported') {
            reject(new Error(`Bluetooth is ${noble.state}. Please turn on Bluetooth.`));
        } else {
            // Wait for state to become poweredOn
            const timeout = setTimeout(() => {
                reject(new Error('Bluetooth initialization timeout. Please check your Bluetooth adapter.'));
            }, 5000);
            
            const stateHandler = async (state) => {
                if (state === 'poweredOn') {
                    clearTimeout(timeout);
                    noble.removeListener('stateChange', stateHandler);
                    try {
                        await noble.startScanningAsync([], true);
                        isScanning = true;
                        console.log('BLE scanning started successfully');
                        resolve({ success: true, message: 'BLE scanning started' });
                    } catch (error) {
                        reject(error);
                    }
                } else if (state === 'poweredOff' || state === 'unsupported') {
                    clearTimeout(timeout);
                    noble.removeListener('stateChange', stateHandler);
                    reject(new Error(`Bluetooth is ${state}. Please turn on Bluetooth.`));
                }
            };
            
            noble.on('stateChange', stateHandler);
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
