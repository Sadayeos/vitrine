// AnimatedArtwork.h
#import <UIKit/UIKit.h>

#define SGKeyLockScreenArtwork @"spotifyglass.lockscreen.artwork"
#define SGKeyLockScreenMotion @"spotifyglass.lockscreen.motion"
#define SGKeyLockScreenLyricsStyle @"spotifyglass.lockscreen.lyricsStyle"
#define SGKeyMotionLowData @"spotifyglass.motion.lowdata"
#define SGKeyMotionSources @"spotifyglass.motion.sources"

typedef NS_ENUM(NSInteger, SGLockArtwork) {
    SGLockArtworkOff = 0,
    SGLockArtworkMotion,
    SGLockArtworkLyrics,
    SGLockArtworkEverySong,
};

typedef NS_ENUM(NSInteger, SGMotionShape) {
    SGMotionSquare,
    SGMotionTall,
};

SGLockArtwork SGLockScreenArtwork(void);

void SGMotionAlbumCover(NSString *artist, NSString *album, SGMotionShape shape, CGFloat pixels, void (^done)(NSURL *file));
void SGMotionArtistLogo(NSString *artist, CGFloat pixels, void (^done)(UIImage *logo));
CGFloat SGMotionPixels(void);
void SGMotionSongsWithISRC(NSString *isrc, void (^done)(NSArray *songs));

NSArray<NSString *> *SGMotionSourceOrder(void);
BOOL SGMotionAppleMusicOn(void);
NSURL *SGMotionCanvasIn(NSDictionary *metadata);
void SGMotionClipFor(NSString *uri, NSURL *canvas, NSString *artist, NSString *album, SGMotionShape shape, CGFloat pixels, void (^done)(NSURL *file, NSString *source));

@class SPTPlayerState;
@interface SGMotionFollower : NSObject
- (instancetype)initWithBegin:(BOOL (^)(NSString *uri, SPTPlayerState *state))begin
                        found:(void (^)(NSString *uri, NSURL *file))found;
- (void)restart;
@end

@class SGModRow;
SGModRow *SGMotionSourcesRow(void);

void SGMotionFile(NSURL *remote, void (^done)(NSURL *file));
NSURL *SGMotionMadeFile(NSString *key);
void SGMotionPoster(NSURL *file, void (^done)(UIImage *poster));

NSArray *SGLockScreenMotionRows(void);
SGModRow *SGLockScreenArtworkNeedsRow(void);

NSString *SGMotionNameKey(NSString *name);
NSString *SGMotionSearchName(NSString *name);
NSString *SGMotionStreamIn(NSString *master, CGFloat pixels);
NSString *SGMotionWholeFileIn(NSString *media);
