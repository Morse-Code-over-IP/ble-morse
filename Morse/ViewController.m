
//  ViewController.m
//  Morse
//
//  Created by Dr. Gerolf Ziegenhain on 05.01.15.
//  Copyright (c) 2015 Dr. Gerolf Ziegenhain. All rights reserved.
//

#import "ViewController.h"
#import <AudioToolbox/AudioToolbox.h>
#import <netdb.h>
#import <netinet/in.h>
#import <netinet6/in6.h>
#import <arpa/inet.h>
#include "cwprotocol.h"

#import <AVFoundation/AVFoundation.h>

#include <mach/clock.h>
#include <mach/mach.h>


//#undef DEBUG
#define TX

OSStatus RenderTone(
                    void *inRefCon,
                    AudioUnitRenderActionFlags 	*ioActionFlags,
                    const AudioTimeStamp 		*inTimeStamp,
                    UInt32 						inBusNumber,
                    UInt32 						inNumberFrames,
                    AudioBufferList 			*ioData)

{
    // Fixed amplitude is good enough for our purposes
    const double amplitude = 0.25;
    
    // Get the tone parameters out of the view controller
    ViewController *vviewController =
    (__bridge ViewController *)inRefCon;
    double theta = vviewController->theta;
    double theta_increment = 2.0 * M_PI * vviewController->frequency / vviewController->sampleRate;
    
    // This is a mono tone generator so we only need the first buffer
    const int channel = 0;
    Float32 *buffer = (Float32 *)ioData->mBuffers[channel].mData;
    
    // Generate the samples
    for (UInt32 frame = 0; frame < inNumberFrames; frame++)
    {
        buffer[frame] = sin(theta) * amplitude;
        
        theta += theta_increment;
        if (theta > 2.0 * M_PI)
        {
            theta -= 2.0 * M_PI;
        }
    }
    
    // Store the theta back in the view controller
    vviewController->theta = theta;
    
    return noErr;
}

void ToneInterruptionListener(void *inClientData, UInt32 inInterruptionState)
{
    ViewController *vviewController =
    (__bridge ViewController *)inClientData;
    
    [vviewController stop];
}

@interface ViewController ()



@end

@implementation ViewController
@synthesize txt1;
@synthesize txt_server;
@synthesize txt_status;
@synthesize txt_channel;
@synthesize txt_id;
@synthesize webview;


@synthesize img;
@synthesize img2;
@synthesize mybutton;
@synthesize btn_circuit;

// connect to server and send my id.
- (void)
identifyclient
{
    tx_sequence++;
    id_packet.sequence = tx_sequence;

    NSData *cc = [NSData dataWithBytes:&connect_packet length:sizeof(connect_packet)];
    NSData *ii = [NSData dataWithBytes:&id_packet length:sizeof(id_packet)];

    [udpSocket sendData:cc toHost:host port:port withTimeout:-1 tag:tx_sequence];
    [udpSocket sendData:ii toHost:host port:port withTimeout:-1 tag:tx_sequence];
}


- (void)connectMorse
{
    char hostname[64] = "mtc-kob.dyndns.org"; // FIXME - make global
    char port1[16] = "7890";
    char id[SIZE_ID] = "iOS/DG6FL, intl. Morse"; // FIXME - make global
    int channel = 33;
    
    prepare_id (&id_packet, id);
    prepare_tx (&tx_data_packet, id);
    connect_packet.channel = channel;
    
    txt_server.text = [NSString stringWithFormat:@"srv: %s:%s", hostname, port1];
    txt_channel.text = [NSString stringWithFormat:@"ch: %d", channel];
    txt_id.text = [NSString stringWithFormat:@"id: %s", id];
   
    udpSocket = [[GCDAsyncUdpSocket alloc] initWithDelegate:self delegateQueue:dispatch_get_main_queue()];
    
    NSError *error = nil;
    if (![udpSocket bindToPort:0 error:&error])
    {
        NSLog(@"error");
        return;
    }
    if (![udpSocket beginReceiving:&error])
    {
        NSLog(@"error");
        return;
    }
    [self identifyclient];
}

- (void)disconnectMorse
{
}

- (void)createToneUnit
{
    // Configure the search parameters to find the default playback output unit
    // (called the kAudioUnitSubType_RemoteIO on iOS but
    // kAudioUnitSubType_DefaultOutput on Mac OS X)
    AudioComponentDescription defaultOutputDescription;
    defaultOutputDescription.componentType = kAudioUnitType_Output;
    defaultOutputDescription.componentSubType = kAudioUnitSubType_RemoteIO;
    defaultOutputDescription.componentManufacturer = kAudioUnitManufacturer_Apple;
    defaultOutputDescription.componentFlags = 0;
    defaultOutputDescription.componentFlagsMask = 0;
    
    // Get the default playback output unit
    AudioComponent defaultOutput = AudioComponentFindNext(NULL, &defaultOutputDescription);
    NSAssert(defaultOutput, @"Can't find default output");
    
    // Create a new unit based on this that we'll use for output
    OSErr err = AudioComponentInstanceNew(defaultOutput, &toneUnit);
    NSAssert1(toneUnit, @"Error creating unit: %hd", err);
    
    // Set our tone rendering function on the unit
    AURenderCallbackStruct input;
    input.inputProc = RenderTone;
    input.inputProcRefCon = (__bridge void *)(self);
    err = AudioUnitSetProperty(toneUnit,
                               kAudioUnitProperty_SetRenderCallback,
                               kAudioUnitScope_Input,
                               0,
                               &input,
                               sizeof(input));
    NSAssert1(err == noErr, @"Error setting callback: %hd", err);
    
    // Set the format to 32 bit, single channel, floating point, linear PCM
    const int four_bytes_per_float = 4;
    const int eight_bits_per_byte = 8;
    AudioStreamBasicDescription streamFormat;
    streamFormat.mSampleRate = sampleRate;
    streamFormat.mFormatID = kAudioFormatLinearPCM;
    streamFormat.mFormatFlags =
    kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved;
    streamFormat.mBytesPerPacket = four_bytes_per_float;
    streamFormat.mFramesPerPacket = 1;
    streamFormat.mBytesPerFrame = four_bytes_per_float;
    streamFormat.mChannelsPerFrame = 1;
    streamFormat.mBitsPerChannel = four_bytes_per_float * eight_bits_per_byte;
    err = AudioUnitSetProperty (toneUnit,
                                kAudioUnitProperty_StreamFormat,
                                kAudioUnitScope_Input,
                                0,
                                &streamFormat,
                                sizeof(AudioStreamBasicDescription));
    NSAssert1(err == noErr, @"Error setting stream format: %hd", err);
}

- (void)beep
{
    if (toneUnit)
    {
        AudioOutputUnitStop(toneUnit);
        AudioUnitUninitialize(toneUnit);
        AudioComponentInstanceDispose(toneUnit);
        toneUnit = nil;
        
    }
    else
    {
        [self createToneUnit];
        
        // Stop changing parameters on the unit
        OSErr err = AudioUnitInitialize(toneUnit);
        NSAssert1(err == noErr, @"Error initializing unit: %hd", err);
        
        // Start playback
        err = AudioOutputUnitStart(toneUnit);
        NSAssert1(err == noErr, @"Error starting unit: %hd", err);
       
    }
    
}

- (void)initCWvars
{
    connect_packet.channel = DEFAULT_CHANNEL;
    connect_packet.command = CON;
    disconnect_packet.channel = 0;
    disconnect_packet.command = DIS;
    tx_sequence = 0;
    last_message = 0;
    tx_timeout = 0;
    last_message = 0;
    circuit = LATCHED;
    
    host = @"mtc-kob.dyndns.org";
    port = 7890;

}

- (void)switchcircuit
{
    if (circuit == LATCHED)
    {
        circuit = UNLATCHED;
        [self unlatch];
    }
    else
    {
        circuit = LATCHED;
        [self latch];
    }
}

- (void)viewDidLoad {
    // Image Stuff
    UIImage *image1 = [UIImage imageNamed:@"one.png"];
    UIImage *image2 = [UIImage imageNamed:@"key.jpg"];
    [img setImage:image1];
    [img2 setImage:image2];
    
    [super viewDidLoad];
    // Do any additional setup after loading the view, typically from a nib.
    
    // Audi stuff
    sampleRate = 44100;

    //BOOL activated = [[AVAudioSession sharedInstance] setActive:YES error:&error];
    OSStatus result = AudioSessionInitialize(NULL, NULL, ToneInterruptionListener, (__bridge void *)(self));
    if (result == kAudioSessionNoError)
    {
        UInt32 sessionCategory = kAudioSessionCategory_MediaPlayback;
        AudioSessionSetProperty(kAudioSessionProperty_AudioCategory, sizeof(sessionCategory), &sessionCategory);
    }
    AudioSessionSetActive(true);
    
    // Tap recog
    
   /* UITapGestureRecognizer * tapr = [[ UITapGestureRecognizer alloc] initWithTarget:self action:@selector(tapresp:)];
    tapr.numberOfTapsRequired  = 1;
    tapr.numberOfTouchesRequired = 1;
    [self.view addGestureRecognizer:tapr];
    */
    
    [mybutton addTarget:self action:@selector(buttonIsDown) forControlEvents:UIControlEventTouchDown];
    [mybutton addTarget:self action:@selector(buttonWasReleased) forControlEvents:UIControlEventTouchUpInside];
    
    [btn_circuit addTarget:self action:@selector(switchcircuit) forControlEvents:UIControlEventTouchDown];
    
    [self initCWvars];
    [self connectMorse];
    
    frequency = 800;
    [self beep]; // hack: must be run once for initialization
    usleep(100*1000);
    AudioOutputUnitStop(toneUnit);
    
    // Display web stuff
    NSString *urlAddress = @"http://mtc-kob.dyndns.org";
    NSURL *url = [NSURL URLWithString:urlAddress];
    NSURLRequest *requestObj = [NSURLRequest requestWithURL:url];
    [webview loadRequest:requestObj];

    myTimer = [NSTimer scheduledTimerWithTimeInterval: KEEPALIVE_CYCLE/100 target: self selector: @selector(sendkeepalive:) userInfo: nil repeats: YES];
    
    // does not work yet [self play_clack];
}


- (void) message:(int) msg
{
    switch(msg){
        case 1:
            if(last_message == msg) return;
            if(last_message == 2) NSLog(@"\n");
            last_message = msg;
            txt_status.text = [NSString stringWithFormat:@"Transmitting"];
            break;
        case 2:
            if(last_message == msg && strncmp(last_sender, rx_data_packet.id, 3) == 0) return;
            else {
                if(last_message == 2) NSLog(@"\n");
                last_message = msg;
                strncpy(last_sender, rx_data_packet.id, 3);
                txt_status.text = [NSString stringWithFormat:@"recv: (%s).",rx_data_packet.id];
            }
            break;
        case 3:
            txt_status.text = [NSString stringWithFormat:@"latched by %s.",rx_data_packet.id];
            break;
        case 4:
            txt_status.text = [NSString stringWithFormat:@"unlatched by %s.",rx_data_packet.id];
            break;
        default:
            break;
    }
}


-(BOOL) playSoundFXnamed: (NSString*) vSFXName Loop: (BOOL) vLoop
{
    NSError *error;
    NSBundle* bundle = [NSBundle mainBundle];
    NSString* bundleDirectory = (NSString*)[bundle bundlePath];
    NSURL *url = [NSURL fileURLWithPath:[bundleDirectory stringByAppendingPathComponent:vSFXName]];
    AVAudioPlayer *audioPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:url error:&error];
    
    if(vLoop)
        audioPlayer.numberOfLoops = -1;
    else
        audioPlayer.numberOfLoops = 0;
    
    BOOL success = YES;
    
    if (audioPlayer == nil)
    {
        success = NO;
        NSLog(@"no");
    }
    else
    {
        success = [audioPlayer play];
        NSLog(@"yes");
    }
    return success;
}


- (void)play_clack
{
    NSLog(@"play clack");
    [self playSoundFXnamed:@"clack48.wav" Loop: NO];
#ifdef okok
    SystemSoundID completeSound;
    NSURL *audioPath = [[NSBundle mainBundle] URLForResource:@"clack48.wav" withExtension:@"wav"];
    AudioServicesCreateSystemSoundID((__bridge CFURLRef)audioPath, &completeSound);
    AudioServicesPlaySystemSound (completeSound);
#endif
}

- (void)udpSocket:(GCDAsyncUdpSocket *)sock didReceiveData:(NSData *)data
      fromAddress:(NSData *)address
withFilterContext:(id)filterContext
{
    int i;
    int translate = 0;
    int audio_status = 1;
    
    [data getBytes:&rx_data_packet length:sizeof(rx_data_packet)];
#ifdef DEBUG1
        NSLog(@"length: %i\n", rx_data_packet.length);
        NSLog(@"id: %s\n", rx_data_packet.id);
        NSLog(@"sequence no.: %i\n", rx_data_packet.sequence);
        NSLog(@"version: %s\n", rx_data_packet.status);
        NSLog(@"n: %i\n", rx_data_packet.n);
        NSLog(@"code:\n");
        for(i = 0; i < SIZE_CODE; i++)NSLog(@"%i ", rx_data_packet.code[i]); NSLog(@"\n");
#endif
    txt_status.text = [NSString stringWithFormat:@"recv from: %s", rx_data_packet.id];
        if(rx_data_packet.n > 0 && rx_sequence != rx_data_packet.sequence){
            [self message:2];
            if(translate == 1){
                txt_status.text = [NSString stringWithFormat:@"%s",rx_data_packet.status];
            }
            rx_sequence = rx_data_packet.sequence;
            for(i = 0; i < rx_data_packet.n; i++){
                switch(rx_data_packet.code[i]){
                    case 1:
                        [self message:3];
                        break;
                    case 2:
                        [self message:4];
                        break;
                    default:
                        if(audio_status == 1)
                        {
                            int length = rx_data_packet.code[i];
                            if(length == 0 || abs(length) > 2000) {
                            }
                            else
                            {
                                if(length < 0) {
                                    AudioOutputUnitStop(toneUnit);
                                    usleep(abs(length)*1000.);
                                    AudioOutputUnitStop(toneUnit);
                                }
                                else
                                {
                                    AudioOutputUnitStart(toneUnit);
                                    usleep(abs(length)*1000.);
                                    AudioOutputUnitStop(toneUnit);
                                }
                            }
                        }
                        break;
                }
            }
        }
    



}


- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (void)stop
{
    [self disconnectMorse];
    
    if (toneUnit)
    {
        // Stop sound

    }
}

- (void)viewDidUnload {
    [self stop];
    AudioSessionSetActive(false);
    
}


-(void)tapresp:(UITapGestureRecognizer *)sender{
    NSLog(@"tapped");
    AudioOutputUnitStop(toneUnit);
 
}
/* portable time, as listed in https://gist.github.com/jbenet/1087739  */
void current_utc_time(struct timespec *ts) {
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    ts->tv_sec = mts.tv_sec;
    ts->tv_nsec = mts.tv_nsec;
}
/* a better clock() in milliseconds */
long
fastclock(void)
{
    struct timespec t;
    long r;
    
    current_utc_time (&t);
    r = t.tv_sec * 1000;
    r = r + t.tv_nsec / 1000000;
    return r;
}

-(void)buttonIsDown
{
    AudioOutputUnitStart(toneUnit);
    
    key_press_t1 = fastclock();
    tx_timeout = 0;
    int timing = (int) ((key_press_t1 - key_release_t1) * -1); // negative timing
    if (timing > TX_WAIT) timing = TX_WAIT; // limit to timeout
    tx_data_packet.n++;
    tx_data_packet.code[tx_data_packet.n - 1] = timing;
    //NSLog(@"timing: %d", timing);
    [self message:1];
}

-(void)buttonWasReleased
{
    AudioOutputUnitStop(toneUnit);
    key_release_t1 = fastclock();
    
    int timing =(int) ((key_release_t1 - key_press_t1) * 1); // positive timing
    if (abs(timing) > TX_WAIT) timing = -TX_WAIT; // limit to timeout FIXME this is the negative part
    if (tx_data_packet.n == SIZE_CODE) NSLog(@"warning: packet is full");
    tx_data_packet.n++;
    tx_data_packet.code[tx_data_packet.n - 1] = timing;
    //NSLog(@"timing: %d", timing);

    
    //NSLog(@"mark: %i\n", tx_data_packet.code[tx_data_packet.n -1]);
    /* TBD = TIMEOUT FOR keypress maximal 5 seconds
    while(1){
        ioctl(fd_serial, TIOCMGET, &serial_status);
        if(serial_status & TIOCM_DSR) break;
        tx_timeout = fastclock() - key_release_t1;
        if(tx_timeout > TX_TIMEOUT) return;
    }
    key_press_t1 = fastclock();
    if(tx_data_packet.n == SIZE_CODE) {
        NSLog(@"irmc: warning packet is full.\n");
        return;
    }
*/
    [self send_data];
}


-(void) send_data
{

    //if (tx_data_packet.code[0]>0) return; // assert first pause
    
    if(tx_data_packet.n == 2 ) return; // assert only two packages
    
    tx_sequence++;
    tx_data_packet.sequence = tx_sequence;

    [self send_tx_packet];
    tx_data_packet.n = 0;

}

- (void) send_tx_packet
{
    int i;
    NSData *cc = [NSData dataWithBytes:&tx_data_packet length:sizeof(tx_data_packet)];
    for(i = 0; i < 5; i++) [udpSocket sendData:cc toHost:host port:port withTimeout:-1 tag:tx_sequence];
#if DEBUG
    NSLog(@"sent seq %d n %d (%d,%d).", tx_sequence, tx_data_packet.n,
          tx_data_packet.code[0] ,
          tx_data_packet.code[1]
          );
#endif
}
- (void)latch
{
    tx_sequence++;
    tx_data_packet.sequence = tx_sequence;
    tx_data_packet.code[0] = -1;
    tx_data_packet.code[1] = 1;
    tx_data_packet.n = 2;
    
    [self send_tx_packet];
    
    tx_data_packet.n = 0;
    NSLog(@"latch");
}

-(void) unlatch
{
    tx_sequence++;
    tx_data_packet.sequence = tx_sequence;
    tx_data_packet.code[0] = -1;
    tx_data_packet.code[1] = 2;
    tx_data_packet.n = 2;
    
    [self send_tx_packet];
    
    tx_data_packet.n = 0;
    NSLog(@"unlatch");
    
}

-(void) sendkeepalive:(NSTimer*)t
{
    //NSLog(@"Keepalive");
    [self identifyclient];
}

@end
