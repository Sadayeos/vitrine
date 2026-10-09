// AnimatedArtwork.x
#import <AVFoundation/AVFoundation.h>
#import <UIKit/UIKit.h>
#import "Core/SGCore.h"
#import "AnimatedArtwork.h"
#import "Headers/SPTPlayer.h"

extern NSString *const SGKeyAnimatedCoversEnabled;

@interface SGAnimatedArtworkViewManager : NSObject
@property (nonatomic, strong) AVQueuePlayer *player;
@property (nonatomic, strong) AVPlayerLooper *looper;
@property (nonatomic, strong) AVPlayerLayer *playerLayer;
@property (nonatomic, weak) UIView *activeContainer;
@property (nonatomic, strong) SGMotionFollower *follower;

+ (instancetype)shared;
- (void)attachToView:(UIView *)container videoURL:(NSURL *)fileURL;
- (void)detach;
@end

@implementation SGAnimatedArtworkViewManager

+ (instancetype)shared {
    static SGAnimatedArtworkViewManager *instance;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [SGAnimatedArtworkViewManager new];
    });
    return instance;
}

- (void)attachToView:(UIView *)container videoURL:(NSURL *)fileURL {
    if (!container || !fileURL) {
        [self detach];
        return;
    }

    if (self.activeContainer == container && self.playerLayer && self.playerLayer.superlayer == container.layer) {
        [CATransaction begin];
        [CATransaction setDisableActions:YES];
        self.playerLayer.frame = container.bounds;
        [CATransaction commit];
        return;
    }

    [self detach];
    self.activeContainer = container;

    AVPlayerItem *item = [AVPlayerItem playerItemWithURL:fileURL];
    self.player = [AVQueuePlayer queuePlayerWithItems:@[item]];
    self.looper = [AVPlayerLooper playerLooperWithPlayer:self.player templateItem:item];

    self.playerLayer = [AVPlayerLayer playerLayerWithPlayer:self.player];
    self.playerLayer.videoGravity = AVLayerVideoGravityResizeAspectFill;
    self.playerLayer.frame = container.bounds;

    container.layer.cornerRadius = 12.0;
    container.layer.masksToBounds = YES;
    self.playerLayer.cornerRadius = 12.0;
    self.playerLayer.masksToBounds = YES;

    [container.layer addSublayer:self.playerLayer];
    [self.player play];
}

- (void)detach {
    if (self.player) {
        [self.player pause];
        self.player = nil;
    }
    self.looper = nil;
    if (self.playerLayer) {
        [self.playerLayer removeFromSuperlayer];
        self.playerLayer = nil;
    }
    self.activeContainer = nil;
}

@end

%hook UIView

- (void)layoutSubviews {
    %orig;

    if (!SGFlag(SGKeyAnimatedCoversEnabled, YES)) {
        [[SGAnimatedArtworkViewManager shared] detach];
        return;
    }

    NSString *className = NSStringFromClass(self.class);
    if ([className containsString:@"SPTNowPlayingArtworkView"] ||
        [className containsString:@"NowPlayingCoverArtView"] ||
        [className containsString:@"ArtworkCell"]) {
        
        SGAnimatedArtworkViewManager *manager = [SGAnimatedArtworkViewManager shared];
        if (manager.playerLayer && (manager.activeContainer == self || manager.activeContainer == nil)) {
            manager.activeContainer = self;
            if (manager.playerLayer.superlayer != self.layer) {
                [self.layer addSublayer:manager.playerLayer];
            }
            [CATransaction begin];
            [CATransaction setDisableActions:YES];
            manager.playerLayer.frame = self.bounds;
            [CATransaction commit];
        }
    }
}

- (void)didMoveToWindow {
    %orig;
    if (!self.window) {
        SGAnimatedArtworkViewManager *manager = [SGAnimatedArtworkViewManager shared];
        if (manager.activeContainer == self) {
            [manager detach];
        }
    }
}

%end

%hook SPTNowPlayingModel

- (void)player:(id)player stateDidChange:(SPTPlayerState *)state {
    %orig;
    
    if (!SGFlag(SGKeyAnimatedCoversEnabled, YES)) {
        [[SGAnimatedArtworkViewManager shared] detach];
        return;
    }

    SGAnimatedArtworkViewManager *manager = [SGAnimatedArtworkViewManager shared];
    if (!manager.follower) {
        manager.follower = [[SGMotionFollower alloc] initWithBegin:^BOOL(NSString *uri, SPTPlayerState *st) {
            return SGFlag(SGKeyAnimatedCoversEnabled, YES);
        } found:^(NSString *uri, NSURL *file) {
            dispatch_async(dispatch_get_main_queue(), ^{
                if (file) {
                    [manager attachToView:manager.activeContainer videoURL:file];
                } else {
                    [manager detach];
                }
            });
        }];
    }
}

%end

%ctor {
    %init;
}
