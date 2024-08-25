//
//  ViewController.m
//  TestFFmpeg2
//
//  Created by Felix on 2024/8/18.
//

#import "ViewController.h"

@interface ViewController ()

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    // Do any additional setup after loading the view.
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    
    if(self.presentingViewController)[self fullscreenMode:YES];
    
    if(_infoMode)[self showInfoView:NO animated:NO];
    
    _savedIdleTimer = [[UIApplication sharedApplication] isIdleTimerDisabled];
    
    [self showHUD: YES];
    
    if(_decoder){
        [self restorePlay];
    }else{
        [_activityIndicatorView startAnimating];
    }
}

-(void) play
{
    if (self.playing) {
        return;
    }
    if (!_decoder.validVideo && !_decoder.validAudio) {
        return;
    }
}

- (void) asyncDecodeFrames{
    if(self.decoding){
        return;
    }
    
    __weak ViewController *weakSelf = self;
    __weak MovieDecoder *weakDecoder = _decoder;
    
    const CGFloat duration = _decoder.isNetwork?.0f:0.1f;
    
    self.decoding = YES;
    dispatch_async(_dispatchQueue, ^{
        {
            __strong ViewController *strongSelf = weakSelf;
            if(!strongSelf.playing)return;
        }
        
        BOOL good = YES;
        while (good) {
            
        }
    })
    
}

@end
