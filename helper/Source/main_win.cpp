/**
 * bleTones BLE Helper – Windows
 *
 * Scans for nearby BLE devices using WinRT Windows.Devices.Bluetooth and
 * forwards each device's RSSI to the bleTones plugin over OSC/UDP (localhost:9000).
 *
 * Requirements:
 *   - Windows 10 (build 1903+) or Windows 11
 *   - C++17, compiled with /await or /std:c++17
 *
 * Build:  see helper/CMakeLists.txt
 * Run:    bleTones_helper.exe   (no window; output goes to console / debugger)
 */

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Devices.Bluetooth.h>
#include <winrt/Windows.Devices.Bluetooth.Advertisement.h>

#include <chrono>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

#include "OSCSender.h"

using namespace winrt;
using namespace Windows::Devices::Bluetooth::Advertisement;

static constexpr int kOSCPort = 9000;

static std::unordered_map<std::string, int16_t> g_devices;
static std::mutex                                g_mutex;

int main()
{
    winrt::init_apartment();

    OSCSender sender;
    if (! sender.open ("127.0.0.1", kOSCPort))
    {
        std::cerr << "[bleTones helper] ERROR: could not open UDP socket\n";
        return 1;
    }

    std::cout << "[bleTones helper] Scanning BLE – sending OSC to localhost:"
              << kOSCPort << "\n";

    BluetoothLEAdvertisementWatcher watcher;
    watcher.ScanningMode (BluetoothLEScanningMode::Active);

    watcher.Received ([] (BluetoothLEAdvertisementWatcher const&,
                          BluetoothLEAdvertisementReceivedEventArgs const& args)
    {
        const int16_t rssi = args.RawSignalStrengthInDBm();

        // Prefer the advertised local name; fall back to a hex address label
        std::string name;
        auto localName = args.Advertisement().LocalName();
        if (! localName.empty())
        {
            name = winrt::to_string (localName);
        }
        else
        {
            char buf[20];
            snprintf (buf, sizeof (buf), "BLE-%012llX",
                      static_cast<unsigned long long> (args.BluetoothAddress()));
            name = buf;
        }

        std::lock_guard<std::mutex> lock (g_mutex);
        g_devices[name] = rssi;
    });

    watcher.Start();

    // Send OSC snapshot at 10 Hz.
    // To stop: terminate the process via Task Manager, Ctrl-C in the console,
    // or the parent process killing this executable. No cleanup is required —
    // the OS reclaims the UDP socket on exit.
    for (;;)
    {
        std::this_thread::sleep_for (std::chrono::milliseconds (100));

        std::lock_guard<std::mutex> lock (g_mutex);
        for (const auto& [name, rssi] : g_devices)
            sender.sendBLERSSI (name, rssi);
    }
}
