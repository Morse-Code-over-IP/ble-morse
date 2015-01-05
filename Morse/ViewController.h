//
//  ViewController.h
//  Morse
//
//  Created by Dr. Gerolf Ziegenhain on 05.01.15.
//  Copyright (c) 2015 Dr. Gerolf Ziegenhain. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <AudioUnit/AudioUnit.h>


@interface ViewController : UIViewController
{
    UILabel *frequencyLabel;
    UIButton *playButton;
    UISlider *frequencySlider;
    AudioComponentInstance toneUnit;
    
@public
    double frequency;
    double sampleRate;
    double theta;
}


@property (weak, nonatomic) IBOutlet UIImageView *img;
@property (weak, nonatomic) IBOutlet UIImageView *img2;
- (void)beep;
- (void)stop;
- (void)connectMorse;
- (void)disconnectMorse;

@end

