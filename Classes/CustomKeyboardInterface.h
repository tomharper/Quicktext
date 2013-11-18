//
//  CustomKeyboardInterface.h
//  Quicktext
//
//  Created by Tom Harper on 6/30/13.
//
//

#ifndef Quicktext_CustomKeyboardInterface_h
#define Quicktext_CustomKeyboardInterface_h

//
//  PKCustomKeyboard.h
//  PunjabiKeyboard
//
//  Created by Kulpreet Chilana on 7/19/12.
//  Copyright (c) 2012 Kulpreet Chilana. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import "PaintingView.h"

@interface CustomKeyboardI : UIView <UIInputViewAudioFeedback>

@property (strong, nonatomic) IBOutlet UIImageView *keyboardBackground;
@property (strong, nonatomic) IBOutletCollection(UIButton) NSArray *characterKeys;
@property (strong, nonatomic) IBOutletCollection(UIButton) NSArray *altButtons;
@property (strong, nonatomic) IBOutletCollection(UIButton) NSArray *shiftButtons;
@property (strong, nonatomic) IBOutlet UIButton *returnButton;
@property (strong, nonatomic) IBOutlet UIButton *deleteButton;
@property (strong, nonatomic) IBOutlet UIButton *dismissButton;
@property (strong) id<UITextInput> textView;

- (IBAction)returnPressed:(id)sender;
- (IBAction)shiftPressed:(id)sender;
- (IBAction)altPressed:(id)sender;
- (IBAction)dismissPressed:(id)sender;
- (IBAction)deletePressed:(id)sender;
- (IBAction)characterPressed:(id)sender;
- (IBAction)unShift;

@end

#endif
