
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


@synthesize img;
@synthesize img2;
@synthesize mybutton;

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
    char id[SIZE_ID] = "iOS GZ"; // FIXME - make global
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
        printf("error");
        return;
    }
    if (![udpSocket beginReceiving:&error])
    {
        printf("error");
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
    tx_timer = 0;
    last_message = 0;
    tx_timeout = 0;
    last_message = 0;
    
    host = @"mtc-kob.dyndns.org";
    port = 7890;

}

- (void)viewDidLoad {
    // Image Stuff
    UIImage *image1 = [UIImage imageNamed:@"one.png"];
    UIImage *image2 = [UIImage imageNamed:@"two.png"];
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
    
    [self initCWvars];
    [self connectMorse];
    
    frequency = 800;
    [self beep]; // hack: must be run once for initialization
    usleep(100*1000);
    AudioOutputUnitStop(toneUnit);
}


- (void) message:(int) msg
{
    switch(msg){
        case 1:
            if(last_message == msg) return;
            if(last_message == 2) printf("\n");
            last_message = msg;
            txt_status.text = [NSString stringWithFormat:@"Transmitting"];
            break;
        case 2:
            if(last_message == msg && strncmp(last_sender, rx_data_packet.id, 3) == 0) return;
            else {
                if(last_message == 2) printf("\n");
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

- (void)udpSocket:(GCDAsyncUdpSocket *)sock didReceiveData:(NSData *)data
      fromAddress:(NSData *)address
withFilterContext:(id)filterContext
{
    int i;
    int translate = 0;
    int audio_status = 1;
    
    [data getBytes:&rx_data_packet length:sizeof(rx_data_packet)];
#ifdef DEBUG1
        printf("length: %i\n", rx_data_packet.length);
        printf("id: %s\n", rx_data_packet.id);
        printf("sequence no.: %i\n", rx_data_packet.sequence);
        printf("version: %s\n", rx_data_packet.status);
        printf("n: %i\n", rx_data_packet.n);
        printf("code:\n");
        for(i = 0; i < SIZE_CODE; i++)printf("%i ", rx_data_packet.code[i]); printf("\n");
#endif
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
    printf("tapped");
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
    //printf("down");
    AudioOutputUnitStart(toneUnit);
    
    key_press_t1 = fastclock();
    tx_timeout = 0;
    int timing = (int) ((key_press_t1 - key_release_t1) * -1); // negative timing
    tx_data_packet.n++;
    tx_data_packet.code[tx_data_packet.n - 1] = timing;
    //printf("timing: %d", timing);
        //printf("space: %i\n", tx_data_packet.code[tx_data_packet.n -1]);
    
     tx_timer = TX_WAIT;
     [self message:1];
}

-(void)buttonWasReleased
{
    //printf("up");
    AudioOutputUnitStop(toneUnit);
    key_release_t1 = fastclock();
    
    int timing =(int) ((key_release_t1 - key_press_t1) * 1); // positive timing
    tx_data_packet.n++;
    tx_data_packet.code[tx_data_packet.n - 1] = timing;
    //printf("timing: %d", timing);

    
    //printf("mark: %i\n", tx_data_packet.code[tx_data_packet.n -1]);
    /* TBD = TIMEOUT FOR keypress
    while(1){
        ioctl(fd_serial, TIOCMGET, &serial_status);
        if(serial_status & TIOCM_DSR) break;
        tx_timeout = fastclock() - key_release_t1;
        if(tx_timeout > TX_TIMEOUT) return;
    }
    key_press_t1 = fastclock();
    if(tx_data_packet.n == SIZE_CODE) {
        printf("irmc: warning packet is full.\n");
        return;
    }
*/
    [self send_data];
}



-(void) send_data
{
    int  i;

    if(tx_timer > 0) tx_timer--;
    
        if(tx_data_packet.n > 1 ){
            tx_sequence++;
            tx_data_packet.sequence = tx_sequence;
            NSData *cc = [NSData dataWithBytes:&tx_data_packet length:sizeof(tx_data_packet )];

            for(i = 0; i < 5; i++) [udpSocket sendData:cc toHost:host port:port withTimeout:-1 tag:tx_sequence];
                //send(fd_socket, &tx_data_packet, SIZE_DATA_PACKET, 0);
#if DEBUG
            printf("irmc: sent data packet.\n");
#endif
            tx_data_packet.n = 0;
        }
        
}

/* TBD Keepalive separate
 if(keepalive_t < 0 && tx_timer == 0){
 #if DEBUG
 printf("keep alive sent.\n");
 #endif
 [self identifyclient];
 keepalive_t = KEEPALIVE_CYCLE;
 }
 if(tx_timer == 0) {
 keepalive_t--;
 usleep(50);
 }
*/


@end
