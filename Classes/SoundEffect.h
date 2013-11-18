/*
     File: SoundEffect.h
 Abstract: SoundEffect is a class that loads and plays sound files.
  Version: 1.12
 Copyright (C) 2013 Apple Inc. All Rights Reserved.
 
*/

#import <UIKit/UIKit.h>
#import <AudioToolbox/AudioServices.h>

@interface SoundEffect : NSObject {
    SystemSoundID _soundID;
}

+ (id)soundEffectWithContentsOfFile:(NSString *)aPath;
- (id)initWithContentsOfFile:(NSString *)path;
- (void)play;

@end
