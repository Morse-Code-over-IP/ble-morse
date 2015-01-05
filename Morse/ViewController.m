//
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
@synthesize img;
@synthesize img2;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// connect to server and send my id.
- (void)
identifyclient
{
    tx_sequence++;
    id_packet.sequence = tx_sequence;
    send(fd_socket, &connect_packet, SIZE_COMMAND_PACKET, 0);
    send(fd_socket, &id_packet, SIZE_DATA_PACKET, 0);
}

- (void)connectMorse
{
    char hostname[64] = "mtc-kob.dyndns.org";
    char id[SIZE_ID] = "iOS GZ";
    int channel = 33;
    char port[16] = "7890";
    
    prepare_id (&id_packet, id);
    prepare_tx (&tx_data_packet, id);
    connect_packet.channel = channel;
    
    txt1.text = [NSString stringWithFormat:@"Connecting to %s on %s \rdoes not show with channel %d and id %s ", hostname, port, channel, id];
  
#ifdef OLD_SOCKET
    struct addrinfo hints, *servinfo, *p;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; /* ipv4 or ipv6 */
    hints.ai_socktype = SOCK_DGRAM;
    int rv;
    if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
        txt1.text = [NSString stringWithFormat:@"getaddrinfo: %s",gai_strerror(rv)];
       //error fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
       //error return 1;
    }

    
    /* Find the first free socket */
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((fd_socket = socket(p->ai_family, p->ai_socktype,
                                p->ai_protocol)) == -1) {
            txt1.text = [NSString stringWithFormat:@"socket"];
            // error perror("irmc: socket");
            continue;
        }
        
        if (connect(fd_socket, p->ai_addr, p->ai_addrlen) == -1) {
            close(fd_socket);
            txt1.text = [NSString stringWithFormat:@"connetct"];
            //error perror("irmc: connect");
            continue;
        }
        
        break;
    }

    fcntl(fd_socket, F_SETFL, O_NONBLOCK);
    if (p == NULL) {
        txt1.text = [NSString stringWithFormat:@"failed to connect"];
        //erroro fprintf(stderr, "irmc: failed to connect\n");
        //error return 2;
    }
    
    char s[INET6_ADDRSTRLEN];
    
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
              s, sizeof s);
    txt1.text = [NSString stringWithFormat:@"connect to %s", s];
    //message: printf("irmc: connected to %s\n", s);

    freeaddrinfo(servinfo); /* all done with this structure */
#else
    // UDP Stuff - the new way
    udpSocket = [[GCDAsyncUdpSocket alloc] initWithDelegate:self delegateQueue:dispatch_get_main_queue()];
    
    NSError *error = nil;
    if (![udpSocket bindToPort:0 error:&error]) //check ff of dit werkt!
    {
        return;
    }
    if (![udpSocket beginReceiving:&error])
    {
        return;
    }
#endif
    
    [self identifyclient];
}

- (void)udpSocket:(GCDAsyncUdpSocket *)sock didReceiveData:(NSData *)data
      fromAddress:(NSData *)address
withFilterContext:(id)filterContext
{
    NSLog(@"Did Receive Data");
    NSString *msg = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
    if (msg)
    {
        NSLog(@"Message: %@",msg);
    }
    else
    {
        NSString *host = nil;
        uint16_t port = 0;
        [GCDAsyncUdpSocket getHost:&host port:&port fromAddress:address];
        
        NSLog(@"Unknown Message: %@:%hu", host, port);
    }
}

- (void)disconnectMorse
{
    send(fd_socket, &disconnect_packet, SIZE_COMMAND_PACKET, 0);
    close(fd_socket);
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
    
    
    [self initCWvars];
    [self connectMorse];
    frequency = 800;
    [self beep];
   
    sleep(1);
    [self beep];
    //[self mainloop];
}


- (void) message:(int) msg
{
    switch(msg){
        case 1:
            if(last_message == msg) return;
            if(last_message == 2) printf("\n");
            last_message = msg;
            txt1.text = [NSString stringWithFormat:@"Transmitting"];
            break;
        case 2:
            if(last_message == msg && strncmp(last_sender, rx_data_packet.id, 3) == 0) return;
            else {
                if(last_message == 2) printf("\n");
                last_message = msg;
                strncpy(last_sender, rx_data_packet.id, 3);
                txt1.text = [NSString stringWithFormat:@"recv: (%s).",rx_data_packet.id];
            }
            break;
        case 3:
            txt1.text = [NSString stringWithFormat:@"irmc: circuit was latched by %s.",rx_data_packet.id];
            break;
        case 4:
            txt1.text = [NSString stringWithFormat:@"irmc: circuit was unlatched by %s.",rx_data_packet.id];
            break;
        default:
            break;
    }
}

- (void)mainloop
{
    char buf[MAXDATASIZE];
    int numbytes = 0,i;
    int translate = 0;
    int audio_status = 1;
    int keepalive_t = 0;


    /* Main Loop */
    for(;;) {
#ifdef TX
        if(tx_timer == 0)
            if((numbytes = recv(fd_socket, buf, MAXDATASIZE-1, 0)) == -1)
                usleep(250);
#endif
        if(numbytes == SIZE_DATA_PACKET && tx_timer == 0){
            memcpy(&rx_data_packet, buf, SIZE_DATA_PACKET);
#if DEBUG
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
                    txt1.text = [NSString stringWithFormat:@"%s",rx_data_packet.status];
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
                                        // beep me pause beep(0.0, abs(length)/1000.);
                                        frequency = 0;
                                        [self beep];
                                        usleep(abs(length)/1000.);
                                        [self beep];
                                    }
                                    else
                                    {
                                        // beep me beep(1000.0, length/1000.);
                                        frequency = 1000;
                                        [self beep];
                                        usleep(abs(length)/1000.);
                                        [self beep];
                                    }
                                }
                            }
                            break;
                    }
                }
            }
        }
        
#ifdef TX
        if(tx_timer > 0) tx_timer--;
        if(tx_data_packet.n > 1 ){
            tx_sequence++;
            tx_data_packet.sequence = tx_sequence;
            for(i = 0; i < 5; i++) send(fd_socket, &tx_data_packet, SIZE_DATA_PACKET, 0);
#if DEBUG		
            printf("irmc: sent data packet.\n");
#endif
            tx_data_packet.n = 0;
        }
        
        /*ioctl(fd_serial,TIOCMGET, &serial_status);
        if(serial_status & TIOCM_DSR){
            txloop();
            tx_timer = TX_WAIT;
            [self message:1];
        }*/
#endif
        
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
 
    } /* End of mainloop */
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
    /* The following stuff does not exist
    self.frequencyLabel = nil;
    self.playButton = nil;
    self.frequencySlider = nil;
    */
    [self stop];
    AudioSessionSetActive(false);
    
}



@end
