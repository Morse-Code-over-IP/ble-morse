//
//  BTService.m
//  Arduino_Servo
//
//  Created by Owen Lacy Brown on 5/21/14.
//  Copyright (c) 2014 Razeware LLC. All rights reserved.
//
// source: http://www.raywenderlich.com/73306/arduino-tutorial-integrating-bluetooth-le-and-ios

#import "BTService.h"


@interface BTService()
@property (strong, nonatomic) CBPeripheral *peripheral;
@property (strong, nonatomic) CBCharacteristic *positionCharacteristic;
@property (strong, nonatomic) CBCharacteristic *rxCharacteristic;
@property (strong, nonatomic) CBCharacteristic *txCharacteristic;
@end

@implementation BTService

#pragma mark - Lifecycle

- (instancetype)initWithPeripheral:(CBPeripheral *)peripheral {
  self = [super init];
  if (self) {
    self.peripheral = peripheral;
    [self.peripheral setDelegate:self];
  }
  return self;
}

- (void)dealloc {
  [self reset];
}

- (void)startDiscoveringServices {
    NSLog(@"Start discovering...");

  [self.peripheral discoverServices:@[RWT_BLE_SERVICE_UUID]];
}

- (void)reset {
  
  if (self.peripheral) {
    self.peripheral = nil;
  }
  
  // Deallocating therefore send notification
  [self sendBTServiceNotificationWithIsBluetoothConnected:NO];
}

#pragma mark - CBPeripheralDelegate

- (void)peripheral:(CBPeripheral *)peripheral didDiscoverServices:(NSError *)error {
  NSArray *services = nil;
    NSArray *uuidsForBTService = @[RX_CHARACTERISTIC_UUID, TX_CHARACTERISTIC_UUID];

    NSLog(@"discover devices");
  
  if (peripheral != self.peripheral) {
    NSLog(@"Wrong Peripheral.\n");
    return ;
  }
  
  if (error != nil) {
    NSLog(@"Error %@\n", error);
    return ;
  }
  
  services = [peripheral services];
  if (!services || ![services count]) {
    NSLog(@"No Services");
    return ;
  }
  
  for (CBService *service in services) {
    if ([[service UUID] isEqual:RWT_BLE_SERVICE_UUID]) {
    NSLog(@"found my service (uart)");
        [peripheral discoverCharacteristics:uuidsForBTService forService:service];
          NSLog(@"discovering done (uart)");
    }
  }
}

- (void)peripheral:(CBPeripheral *)peripheral didDiscoverCharacteristicsForService:(CBService *)service error:(NSError *)error {
    NSArray     *characteristics    = [service characteristics];
    NSLog(@"did discover characterstics");
    
    if (peripheral != self.peripheral) {
        NSLog(@"Wrong Peripheral.\n");
        return ;
    }
    
    if (error != nil) {
        NSLog(@"Error %@\n", error);
        return ;
    }
    
    for (CBCharacteristic *characteristic in characteristics) {
        NSLog(@"chk charac");
        if ([[characteristic UUID] isEqual:RX_CHARACTERISTIC_UUID]) {
            NSLog(@"Rx characteristic found");
            self.positionCharacteristic = characteristic;
            self.rxCharacteristic = characteristic;
            [self.peripheral setNotifyValue:TRUE forCharacteristic:characteristic];
            // Send notification that Bluetooth is connected and all required characteristics are discovered
            [self sendBTServiceNotificationWithIsBluetoothConnected:YES];
        }
        if ([[characteristic UUID] isEqual:TX_CHARACTERISTIC_UUID]) {
            NSLog(@"Tx characteristic found");
            self.txCharacteristic = characteristic;
        }
    }
    
    if (self.rxCharacteristic != nil && self.txCharacteristic != nil) {
        NSLog(@"found both rx and tx");
    }
}



- (void)peripheral:(CBPeripheral *)peripheral didUpdateNotificationStateForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error
{
    if (error) {
        NSLog(@"Error changing notification state: %@", error.localizedDescription);
        return;
    }
    
    // Notification has started
    if (characteristic.isNotifying) {
        NSLog(@"Notification began on %@", characteristic);
    }
    
    NSLog(@"recv: event");
    if (characteristic == self.rxCharacteristic) {
        NSLog(@"input");
    }
}

- (void)peripheral:(CBPeripheral *)peripheral didUpdateValueForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error {
    if (error) {
        NSLog(@"Error reading characteristics: %@", [error localizedDescription]);
        return;
    }
    
    if (characteristic.value != nil) {
        NSData *data = characteristic.value;
        //NSString s = NSString(bytes: &data, length: dataLength, encoding: NSUTF8StringEncoding)
        //value here.
        NSLog(@"there is data");
    }
}


- (void)writePosition:(UInt8)position {
  // TODO: Add implementation
}

- (void)sendBTServiceNotificationWithIsBluetoothConnected:(BOOL)isBluetoothConnected {
    NSLog(@"connected...");
  NSDictionary *connectionDetails = @{@"isConnected": @(isBluetoothConnected)};
  [[NSNotificationCenter defaultCenter] postNotificationName:RWT_BLE_SERVICE_CHANGED_STATUS_NOTIFICATION object:self userInfo:connectionDetails];
}

@end
