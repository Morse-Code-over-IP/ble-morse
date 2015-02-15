//
//  BTService.h
//  Arduino_Servo
//
//  Created by Owen Lacy Brown on 5/21/14.
//  Copyright (c) 2014 Razeware LLC. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreBluetooth/CoreBluetooth.h>

/* Services & Characteristics UUIDs */
//#define RWT_BLE_SERVICE_UUID		[CBUUID UUIDWithString:@"B8E06067-62AD-41BA-9231-206AE80AB550"] - aka Service Name
//&&#define RWT_POSITION_CHAR_UUID		[CBUUID UUIDWithString:@"BF45E40A-DE2A-4BC8-BBA0-E5D6065F1B4B"] // aka tx character - gatt profile

// icd: https://github.com/michaelkroll/BLE-Shield/blob/master/firmware/BLE-Shield-v2.0.0/BLE-Shield_gatt.xml
// icd nrf8001: https://devzone.nordicsemi.com/documentation/nrf51/6.0.0/s110/html/a00066.html
#define RWT_BLE_SERVICE_UUID		[CBUUID UUIDWithString:@"6E400001-B5A3-F393-E0A9-E50E24DCCA9E"] // uart service name
#define TX_CHARACTERISTIC_UUID      [CBUUID UUIDWithString:@"6E400002-B5A3-F393-E0A9-E50E24DCCA9E"]
#define RX_CHARACTERISTIC_UUID      [CBUUID UUIDWithString:@"6E400003-B5A3-F393-E0A9-E50E24DCCA9E"]

/*
 func uartServiceUUID()->CBUUID{ return CBUUID(string:      "6e400001-b5a3-f393-e0a9-e50e24dcca9e") }
 func txCharacteristicUUID()->CBUUID{ return CBUUID(string: "6e400002-b5a3-f393-e0a9-e50e24dcca9e") }
 func rxCharacteristicUUID()->CBUUID{ return CBUUID(string: "6e400003-b5a3-f393-e0a9-e50e24dcca9e") }
*/

/* Notifications */
static NSString* const RWT_BLE_SERVICE_CHANGED_STATUS_NOTIFICATION = @"kBLEServiceChangedStatusNotification";


/* BTService */
@interface BTService : NSObject <CBPeripheralDelegate>

- (instancetype)initWithPeripheral:(CBPeripheral *)peripheral;
- (void)reset;
- (void)startDiscoveringServices;

- (void)writePosition:(UInt8)position;

@end
