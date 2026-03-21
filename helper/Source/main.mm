/**
 * bleTones BLE Helper – macOS
 *
 * Scans for nearby BLE devices using CoreBluetooth and forwards each device's
 * RSSI to the bleTones plugin over OSC/UDP (localhost:9000).
 *
 * Build:  see helper/CMakeLists.txt
 * Run:    open bleTones\ Helper.app   (or launch from terminal)
 *
 * The app is a LSUIElement (no Dock icon) – it runs silently in the background.
 * macOS will prompt for Bluetooth permission on first launch (Info.plist entry).
 */

#import <Foundation/Foundation.h>
#import <CoreBluetooth/CoreBluetooth.h>

#include "OSCSender.h"

static constexpr int    kOSCPort          = 9000;
static constexpr double kSendIntervalSecs = 0.1; // 10 Hz

// -----------------------------------------------------------------------------
@interface BLEScanner : NSObject <CBCentralManagerDelegate>

@property (nonatomic, strong) CBCentralManager*                      central;
@property (nonatomic, strong) NSMutableDictionary<NSUUID*, NSString*>* names;
@property (nonatomic, strong) NSMutableDictionary<NSUUID*, NSNumber*>* rssiMap;

- (NSDictionary<NSString*, NSNumber*>*)snapshotByName;

@end

@implementation BLEScanner

- (instancetype)init
{
    if ((self = [super init]))
    {
        _names   = [NSMutableDictionary dictionary];
        _rssiMap = [NSMutableDictionary dictionary];
        _central = [[CBCentralManager alloc] initWithDelegate:self
                                                        queue:dispatch_get_main_queue()];
    }
    return self;
}

- (void)centralManagerDidUpdateState:(CBCentralManager*)central
{
    if (central.state == CBManagerStatePoweredOn)
    {
        NSLog (@"[bleTones helper] Bluetooth powered on – scanning…");
        [central scanForPeripheralsWithServices:nil
                                        options:@{ CBCentralManagerScanOptionAllowDuplicatesKey: @YES }];
    }
    else
    {
        NSLog (@"[bleTones helper] Bluetooth unavailable (state=%ld)", (long)central.state);
    }
}

- (void)       centralManager:(CBCentralManager* __unused)central
        didDiscoverPeripheral:(CBPeripheral*)peripheral
            advertisementData:(NSDictionary<NSString*, id>*)advertisementData
                         RSSI:(NSNumber*)RSSI
{
    NSUUID*   uuid = peripheral.identifier;
    NSString* name = peripheral.name
                  ?: advertisementData[CBAdvertisementDataLocalNameKey];

    if (!name || name.length == 0)
        name = [NSString stringWithFormat:@"BLE-%@",
                [uuid.UUIDString substringToIndex:8]];

    @synchronized (self)
    {
        _names[uuid]   = name;
        _rssiMap[uuid] = RSSI;
    }
}

- (NSDictionary<NSString*, NSNumber*>*)snapshotByName
{
    NSMutableDictionary* snap = [NSMutableDictionary dictionary];
    @synchronized (self)
    {
        for (NSUUID* uuid in _rssiMap)
            snap[_names[uuid]] = _rssiMap[uuid];
    }
    return snap;
}

@end

// -----------------------------------------------------------------------------
int main (int /*argc*/, const char* /*argv*/[])
{
    @autoreleasepool
    {
        NSLog (@"bleTones BLE Helper starting – OSC → localhost:%d", kOSCPort);

        OSCSender sender;
        if (! sender.open ("127.0.0.1", kOSCPort))
        {
            NSLog (@"[bleTones helper] ERROR: could not open UDP socket");
            return 1;
        }

        BLEScanner* scanner = [[BLEScanner alloc] init];

        // Periodic send: snapshot current RSSI table and push via OSC
        [NSTimer scheduledTimerWithTimeInterval:kSendIntervalSecs
                                        repeats:YES
                                          block:^(NSTimer* __unused t)
        {
            NSDictionary<NSString*, NSNumber*>* snap = [scanner snapshotByName];
            for (NSString* name in snap)
                sender.sendBLERSSI (name.UTF8String, snap[name].intValue);
        }];

        [[NSRunLoop mainRunLoop] run];
    }
    return 0;
}
