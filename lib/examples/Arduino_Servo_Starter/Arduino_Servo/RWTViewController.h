//
//  RWTViewController.h
//  Arduino_Servo
//
//  Created by Owen Lacy Brown on 5/21/14.
//  Copyright (c) 2014 Razeware LLC. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface RWTViewController : UIViewController
@property (weak, nonatomic) IBOutlet UIImageView *imgBluetoothStatus;
@property (weak, nonatomic) IBOutlet UISlider *positionSlider;

- (IBAction)positionSliderChanged:(UISlider *)sender;

@end
