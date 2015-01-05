//
//  ViewController.m
//  Morse
//
//  Created by Dr. Gerolf Ziegenhain on 05.01.15.
//  Copyright (c) 2015 Dr. Gerolf Ziegenhain. All rights reserved.
//

#import "ViewController.h"

@interface ViewController ()



@end

@implementation ViewController
@synthesize img;
@synthesize img2;

- (void)viewDidLoad {
    UIImage *image1 = [UIImage         imageNamed:@"one.png"];
    
        UIImage *image2 = [UIImage         imageNamed:@"two.png"];
    [img setImage:image1];
    [img2 setImage:image2];
    
    
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

@end
