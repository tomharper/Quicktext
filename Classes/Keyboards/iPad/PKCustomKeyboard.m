//
//  PKCustomKeyboard.m
//  PunjabiKeyboard
//
//  Created by Kulpreet Chilana on 7/19/12.
//  Copyright (c) 2012 Kulpreet Chilana. All rights reserved.
//

#import "PKCustomKeyboard.h"

@interface PKCustomKeyboard ()

@property (nonatomic, assign, getter=isShifted) BOOL shifted;

@end

@implementation PKCustomKeyboard
@synthesize textView = _textView;

- (id)init {
	
	CGRect rect = [[UIScreen mainScreen] bounds];
	int nHt = 264; // 768 width in theory
	UIDeviceOrientation orientation = [[UIDevice currentDevice] orientation];
	if(UIDeviceOrientationIsLandscape(orientation))
		nHt = 352; // 1024 width in theory
	int nHtOffset = rect.size.height - nHt;
	CGRect frame = CGRectMake(rect.origin.x, rect.origin.y + nHtOffset, rect.size.width, nHt);
	
	self = [super initWithFrame:frame];
	
	if (self) {
        NSArray *nib = [[NSBundle mainBundle] loadNibNamed:@"PKCustomKeyboard" owner:self options:nil];
		[[nib objectAtIndex:0] setFrame:frame];
        self = [nib objectAtIndex:0];
    }
	
	NSMutableArray *buttons = [NSMutableArray arrayWithArray:self.characterKeys];
	[buttons addObjectsFromArray:self.altButtons];
	[buttons addObject:self.returnButton];
	[buttons addObject:self.dismissButton];
	[buttons addObject:self.deleteButton];
	
	for (UIButton *b in buttons) {
		[b setBackgroundImage:[PKCustomKeyboard imageFromColor:[UIColor colorWithWhite:0.5 alpha:0.5]] forState:UIControlStateHighlighted];
		b.layer.cornerRadius = 7.0;
		b.layer.masksToBounds = YES;
		b.layer.borderWidth = 0;
	}
	
	for (UIButton *b in self.altButtons)
		[b setTitle:kAltLabel forState:UIControlStateNormal];
	
	[self.returnButton setTitle:kReturnLabel forState:UIControlStateNormal];
	self.returnButton.titleLabel.adjustsFontSizeToFitWidth = YES;
	
	[self loadCharactersWithArray:kChar];
	
	return self;
}

// The magic here is to set this as an INPUT view...

-(void)setTextView:(id<UITextInput>)textView {
	
	if ([textView isKindOfClass:[UITextView class]])
        [(UITextView *)textView setInputView:self];
    else if ([textView isKindOfClass:[UITextField class]])
        [(UITextField *)textView setInputView:self];
    
    _textView = textView;
}

-(id<UITextInput>)textView {
	return _textView;
}

-(void)loadCharactersWithArray:(NSArray *)a {
	int i = 0;
	for (UIButton *b in self.characterKeys) {
		[b setTitle:[a objectAtIndex:i] forState:UIControlStateNormal];
		//setFont:kFont
		[b.titleLabel setFont:[UIFont systemFontOfSize:28]];
		i++;
	}
}

- (BOOL) enableInputClicksWhenVisible {
    return YES;
}

/* IBActions for Keyboard Buttons */

- (IBAction)returnPressed:(id)sender {
    [[UIDevice currentDevice] playInputClick];
	[self.textView insertText:@"\n"];
	if ([self.textView isKindOfClass:[UITextView class]])
		[[NSNotificationCenter defaultCenter] postNotificationName:UITextViewTextDidChangeNotification object:self.textView];
	else if ([self.textView isKindOfClass:[UITextField class]])
		[[NSNotificationCenter defaultCenter] postNotificationName:UITextFieldTextDidChangeNotification object:self.textView];
}

- (IBAction)shiftPressed:(id)sender {
	[[UIDevice currentDevice] playInputClick];
	if (!self.isShifted) {
		[self.keyboardBackground setImage:[UIImage imageNamed:@"Keyboard_Shift.png"]];
		[self loadCharactersWithArray:kChar_shift];
		for (UIButton *b in self.altButtons)
			[b setTitle:kAltLabel forState:UIControlStateNormal];
	}
}

- (IBAction)unShift {
	if (self.isShifted) {
		[self.keyboardBackground setImage:[UIImage imageNamed:@"Keyboard_Blank.png"]];
		[self loadCharactersWithArray:kChar];
	}
	if (!self.isShifted)
		self.shifted = YES;
	else
		self.shifted = NO;
}

- (IBAction)altPressed:(id)sender {
    [[UIDevice currentDevice] playInputClick];
	[self.keyboardBackground setImage:[UIImage imageNamed:@"Keyboard_Blank.png"]];
	self.shifted = NO;
	UIButton *button = (UIButton *)sender;
	
	if ([button.titleLabel.text isEqualToString:kAltLabel]) {
		[self loadCharactersWithArray:kChar_alt];
		for (UIButton *b in self.altButtons)
			[b setTitle:[kChar objectAtIndex:18] forState:UIControlStateNormal];
	}
	else {
		[self loadCharactersWithArray:kChar];
		for (UIButton *b in self.altButtons)
			[b setTitle:kAltLabel forState:UIControlStateNormal];
	}
}

- (IBAction)dismissPressed:(id)sender {
    [[UIDevice currentDevice] playInputClick];
	
	if ([self.textView isKindOfClass:[UITextView class]]) 
        [(UITextView *)self.textView resignFirstResponder];
	
    else if ([self.textView isKindOfClass:[UITextField class]])
        [(UITextField *)self.textView resignFirstResponder];
}

- (IBAction)deletePressed:(id)sender {
    [[UIDevice currentDevice] playInputClick];
	[self.textView deleteBackward];
	[[NSNotificationCenter defaultCenter] postNotificationName:UITextViewTextDidChangeNotification object:self.textView];
	
	if ([self.textView isKindOfClass:[UITextView class]])
		[[NSNotificationCenter defaultCenter] postNotificationName:UITextViewTextDidChangeNotification object:self.textView];
	else if ([self.textView isKindOfClass:[UITextField class]])
		[[NSNotificationCenter defaultCenter] postNotificationName:UITextFieldTextDidChangeNotification object:self.textView];
}

- (IBAction)characterPressed:(id)sender {
    [[UIDevice currentDevice] playInputClick];
	UIButton *button = (UIButton *)sender;
	NSString *character = [NSString stringWithString:button.titleLabel.text];
		
	[self.textView insertText:character];

	if (self.isShifted)
		[self unShift];
	
	if ([self.textView isKindOfClass:[UITextView class]])
		[[NSNotificationCenter defaultCenter] postNotificationName:UITextViewTextDidChangeNotification object:self.textView];
	else if ([self.textView isKindOfClass:[UITextField class]])
		[[NSNotificationCenter defaultCenter] postNotificationName:UITextFieldTextDidChangeNotification object:self.textView];
}

/* UI Utilities */

+ (UIImage *) imageFromColor:(UIColor *)color {
    CGRect rect = CGRectMake(0, 0, 1, 1);
    UIGraphicsBeginImageContext(rect.size);
    CGContextRef context = UIGraphicsGetCurrentContext();
    CGContextSetFillColorWithColor(context, [color CGColor]);
    CGContextFillRect(context, rect);
    UIImage *img = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    return img;
}

@end
