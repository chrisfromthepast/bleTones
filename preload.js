const { contextBridge, ipcRenderer } = require('electron');

// Expose protected methods that allow the renderer process to use
// the ipcRenderer without exposing the entire object
contextBridge.exposeInMainWorld('electronBLE', {
    startScan: () => ipcRenderer.invoke('start-ble-scan'),
    stopScan: () => ipcRenderer.invoke('stop-ble-scan'),
    checkSupport: () => ipcRenderer.invoke('check-ble-support'),
    onAdvertisement: (callback) => {
        ipcRenderer.on('ble-advertisement', (event, data) => callback(data));
    }
});
