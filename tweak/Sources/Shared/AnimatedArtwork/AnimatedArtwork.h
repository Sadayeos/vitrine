#import <AVFoundation/AVFoundation.h>
#import <UIKit/UIKit.h>
#import <objc/runtime.h>
#import "Core/SGCore.h"
#import "AnimatedArtwork.h"
#import "Headers/SPTPlayer.h"

// Clave pública para la preferencia global
extern NSString *const SGKeyAnimatedCoversEnabled;

@interface SGAnimatedArtworkViewManager : NSObject
@property (nonatomic, strong) AVQueuePlayer *player;
@property (nonatomic, strong) AVPlayerLooper *looper;
@property (nonatomic, strong) AVPlayerLayer *playerLayer;
@property (nonatomic, weak) UIView *activeContainer;
@property (nonatomic, strong) SGMotionFollower *follower;
@property (nonatomic, copy) NSString *currentURI;

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

    // Si ya estamos reproduciendo exactamente este archivo en el mismo contenedor, solo ajustamos el marco
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

    // Adaptación idéntica a Apple Music: Borde redondeado y recorte automático del recuadro
    container.layer.cornerRadius = 12.0;
    container.layer.masksToBounds = YES;
    self.playerLayer.cornerRadius = 12.0;
    self.playerLayer.masksToBounds = YES;

    // Agregamos el playerLayer directamente como subcapa de la carátula
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

#pragma mark - Observer de Cambios de Estado y Redimensionamiento (Horizontal / Vertical)

%hook UIView

- (void)layoutSubviews {
    %orig;

    // Verificamos si la portada animada está encendida en las configuraciones
    if (!SGFlag(SGKeyAnimatedCoversEnabled, YES)) {
        [[SGAnimatedArtworkViewManager shared] detach];
        return;
    }

    NSString *className = NSStringFromClass(self.class);
    // Identificamos las clases contenedoras del artwork nativo de Spotify
    if ([className containsString:@"SPTNowPlayingArtworkView"] ||
        [className containsString:@"NowPlayingCoverArtView"] ||
        [className containsString:@"ArtworkCell"]) {
        
        SGAnimatedArtworkViewManager *manager = [SGAnimatedArtworkViewManager shared];
        if (manager.playerLayer && (manager.activeContainer == self || manager.activeContainer == nil)) {
            manager.activeContainer = self;
            if (manager.playerLayer.superlayer != self.layer) {
                [self.layer addSublayer:manager.playerLayer];
            }
            // Mantenemos la capa reajustada instantáneamente al recuadro (soportando modo horizontal y letras)
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

#pragma mark - Manejo de Pistas e Integración con SGMotionFollower

%hook SPTNowPlayingModel

- (void)player:(id)player stateDidChange:(SPTPlayerState *)state {
    %orig;
    
    if (!SGFlag(SGKeyAnimatedCoversEnabled, YES)) {
        [[SGAnimatedArtworkViewManager shared] detach];
        return;
    }

    SGAnimatedArtworkViewManager *manager = [SGAnimatedArtworkViewManager shared];
    
    // Inicializamos el follower de movimiento si aún no está configurado
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

#pragma mark - Suscripción a la Notificación de Cambio de Estado (Menú Contextual)

static void animatedArtworkDidChangeNotification(CFNotificationCenterRef center, void *observer, CFStringRef name, const void *object, CFDictionaryRef userInfo) {
    dispatch_async(dispatch_get_main_queue(), ^{
        SGAnimatedArtworkViewManager *manager = [SGAnimatedArtworkViewManager shared];
        if (!SGFlag(SGKeyAnimatedCoversEnabled, YES)) {
            [manager detach];
        } else if (manager.follower) {
            [manager.follower restart];
        }
    });
}

%ctor {
    %init;
    
    // Registrar el listener para actualizar en tiempo real al tocar el botón en el menú contextual
    [NSNotificationCenter.defaultCenter addObserverForName:@"spotifyglass.animatedArtworkDidChange"
                                                  object:nil
                                                   queue:NSOperationQueue.mainQueue
                                              usingBlock:^(NSNotification * _Nonnull note) {
        SGAnimatedArtworkViewManager *manager = [SGAnimatedArtworkViewManager shared];
        if (!SGFlag(SGKeyAnimatedCoversEnabled, YES)) {
            [manager detach];
        } else if (manager.follower) {
            [manager.follower restart];
        }
    }];
}
