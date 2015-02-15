//
//  RWTViewController.m
//  Arduino_Servo
//
//  Created by Owen Lacy Brown on 5/21/14.
//  Copyright (c) 2014 Razeware LLC. All rights reserved.
//

#import "RWTViewController.h"
#import "BTDiscovery.h"
#import "BTService.h"

@interface RWTViewController ()
@property (strong, nonatomic) NSTimer *timerTXDelay;
@property (nonatomic) BOOL allowTX;
@end

@implementation RWTViewController

#pragma mark - Lifecycle

- (void)viewDidLoad {
  [super viewDidLoad];
    NSLog(@"start at beg");

  // Rotate slider to vertical position
  UIView *superView = self.positionSlider.superview;
  [self.positionSlider removeFromSuperview];
  [self.positionSlider removeConstraints:self.view.constraints];
  self.positionSlider.translatesAutoresizingMaskIntoConstraints = YES;
  self.positionSlider.transform = CGAffineTransformMakeRotation(M_PI_2);
  [superView addSubview:self.positionSlider];
  
  // Set thumb image on slider
  [self.positionSlider setThumbImage:[UIImage imageNamed:@"Bar"] forState:UIControlStateNormal];
  
  self.allowTX = YES;
  
    NSLog(@"watch bt");
  // Watch Bluetooth connection
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(connectionChanged:) name:RWT_BLE_SERVICE_CHANGED_STATUS_NOTIFICATION object:nil];
  
  // Start the Bluetooth discovery process
  [BTDiscovery sharedInstance];
    NSLog(@"done load");
}

- (void)dealloc {
  
  [[NSNotificationCenter defaultCenter] removeObserver:self name:RWT_BLE_SERVICE_CHANGED_STATUS_NOTIFICATION object:nil];
}

- (void)viewWillDisappear:(BOOL)animated {
  [super viewWillDisappear:animated];
  
  [self stopTimerTXDelay];
}

#pragma mark - IBActions

- (IBAction)positionSliderChanged:(UISlider *)sender {
  // Since the slider value range is from 0 to 180, it can be sent directly to the Arduino board
  
  [self sendPosition:(uint8_t)sender.value];
}

#pragma mark - Private

- (void)connectionChanged:(NSNotification *)notification {
  // Connection status changed. Indicate on GUI.
  BOOL isConnected = [(NSNumber *) (notification.userInfo)[@"isConnected"] boolValue];
  
  dispatch_async(dispatch_get_main_queue(), ^{
    // Set image based on connection status
    self.imgBluetoothStatus.image = isConnected ? [UIImage imageNamed:@"Bluetooth_Connected"]: [UIImage imageNamed:@"Bluetooth_Disconnected"];
    
    if (isConnected) {
      // Send current slider position
      [self sendPosition:(uint8_t)self.positionSlider.value];
    }
  });
}


- (void)sendPosition:(uint8_t)position {
  // TODO: Add implementation
}

- (void)timerTXDelayElapsed {
  self.allowTX = YES;
  [self stopTimerTXDelay];
  
  // Send current slider position
  [self sendPosition:(uint8_t)self.positionSlider.value];
}

- (void)stopTimerTXDelay {
  if (!self.timerTXDelay) {
    return;
  }
  
  [self.timerTXDelay invalidate];
  self.timerTXDelay = nil;
}

@end
