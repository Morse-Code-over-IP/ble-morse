//
//  ViewController.h
//  Morse
//
//  Created by Dr. Gerolf Ziegenhain on 05.01.15.
//  Copyright (c) 2015 Dr. Gerolf Ziegenhain. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <AudioUnit/AudioUnit.h>
#include "cwprotocol.h"
#include "GCDAsyncUdpSocket.h"

#define TX_WAIT  5000
#define TX_TIMEOUT 240.0
#define KEEPALIVE_CYCLE 100

#define MAXDATASIZE 1024 // max number of bytes we can get at once

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
    int fd_socket;
    struct command_packet_format connect_packet;
    struct command_packet_format disconnect_packet;
    struct data_packet_format id_packet;
    struct data_packet_format rx_data_packet;
    struct data_packet_format tx_data_packet;
    
    int tx_sequence, rx_sequence;
    long tx_timer;

    int last_message;
    char last_sender[16];
    
    // Networking
    GCDAsyncUdpSocket *udpSocket;
}


@property (weak, nonatomic) IBOutlet UIButton *mybutton;
@property (weak, nonatomic) IBOutlet UILabel *txt1;
@property (weak, nonatomic) IBOutlet UIImageView *img;
@property (weak, nonatomic) IBOutlet UIImageView *img2;
- (void)beep;
- (void)stop;
- (void)connectMorse;
- (void)disconnectMorse;
- (void)message:(int)msg;
- (void)identifyclient;
- (void)mainloop;
- (void)initCWvars;
-(void)tapresp:(UITapGestureRecognizer *)sender;

@end

