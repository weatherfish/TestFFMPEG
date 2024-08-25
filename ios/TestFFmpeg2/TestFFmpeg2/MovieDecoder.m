//
//  MovieDecoder.m
//  TestFFmpeg2
//
//  Created by Felix on 2024/8/18.
//

#import <Foundation/Foundation.h>
#import "MovieDecoder.h"
#import "libavcodec/avcodec.h"
#import "libavformat/avformat.h"
#import "libavdevice/avdevice.h"
#import "libavfilter/avfilter.h"

@interface MovieDecoder ()

@end

@implementation MovieDecoder

- (void) decodeFrame
{
    NSMutableArray *result = [NSMutableArray array];
    AVPacket packet;
    
    CGFloat decodedDuration = 0;

    BOOL finished = NO;

    while(!finished){
        if(av_read_frame(_formatCtx, &packet) < 0){
            _isEOF = YES;
            break;
        }
        if(packat.stream_index == _videoStream){
            int pktSize = packet.size;
            
            while (pktSize > 0) {
                int gotframe = 0;
                int len = avcodec_decode_video(_videoCodecContext, _videoFrame,&gotframe, &packet);
                if(len < 0){
                    
                    break;
                }
                if(gotframe){
                    if (!_disableDeinterlacing && _videoFrame->interlaced_frame) {
                        avpicture_deinterlace((AVPicture*)_videoFrame,（AVPicture*)_videoFrame,_videoCodecContext->pix_fmt,
                        _videoCodecContext->width, _videoCodecContext->height);
                    }
                    VideoFrame *frame = [self handleVideoFrame];
                    if(frame){
                        [result addObject:frame];
                        
                        _position = frame.position;
                        decodedDuration += frame.duration;
                        if(decodedDuration > minDuration){
                            finished = YES;
                        }
                    }
                    if (len == 0) {
                        break;
                    }
                }
            }
        }
    }
}


