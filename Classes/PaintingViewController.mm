/*
     File: PaintingViewController.m
 Abstract: The central controller of the application.
  Version: 1.12
 Copyright (C) 2013 Apple Inc. All Rights Reserved.
 */

#import "PaintingViewController.h"
//#import "PaintingView.h"
#import "Keyboards/iPad//PKCustomKeyboard.h"
#import "Keyboards/iPhone:iPod/PMCustomKeyboard.h"

//CLASS IMPLEMENTATIONS:

@interface PaintingViewController()
{
//	PaintingView *pPaintView;
}
@end

@implementation PaintingViewController

- (void)viewDidLoad
{
	[super viewDidLoad];
	
	self.title = @"PaintingViewController";
	self.view = [[UIView alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
	self.view.backgroundColor = [UIColor clearColor];
	
	CGRect rect = [[UIScreen mainScreen] bounds];
	int kPaletteHeight = rect.size.height - 216;
	CGRect frame = CGRectMake(rect.origin.x+10.0, rect.origin.y+20.0, rect.size.width-20.0, kPaletteHeight-20.0);
	UITextView *textView = [[UITextView alloc] initWithFrame:frame];
	//textView.translucent = YES; // not avail here
	textView.opaque = NO; // no effect for these kind of controls
	//textView.tintColor = [UIColor clearColor ]; // not settable ios 6
	textView.backgroundColor = [UIColor clearColor]; // background color
	textView.alpha = 1.0; // allows us to see the underlay on ios 6.0
	//self.view = textView;
	textView.layer.zPosition = 1;
	// add control, then release it
	[self.view addSubview:textView];
	
	if ( UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone )
	{
		// init is called in this constructor here
		PMCustomKeyboard *customKeyboard = [[PMCustomKeyboard alloc] init];
		customKeyboard.layer.zPosition = 1;
		[customKeyboard setTextView:textView];
        customKeyboard.alpha = 1.0f;
		
		// comment me out...?
		[self.view addSubview:customKeyboard];
	}
	else {
		//352-1024
		PKCustomKeyboard *customKeyboard = [[PKCustomKeyboard alloc] init];
		customKeyboard.layer.zPosition = 1;
		[customKeyboard setTextView:textView];
		customKeyboard.alpha = 1.0f;

		// comment me out
		[self.view addSubview:customKeyboard];
	}
	[textView release];

	Class cl = [self.view class];
	NSString *classDescription = [cl description];
	while ([cl superclass])
	{
		cl = [cl superclass];
		classDescription = [classDescription stringByAppendingFormat:@":%@", [cl description]];
	}
	NSLog(@"%@ %@", classDescription, NSStringFromCGRect(self.view.frame));
	//for (NSUInteger i = 0; i < [self.view.subviews count]; i++) {
	//	UIView *subView = [self.view.subviews objectAtIndex:i];
	//}
}

// Release resources when they are no longer needed,
- (void)dealloc
{
	
	[super dealloc];
}


- (BOOL)shouldAutorotate
{
    return NO;
}

// On pre-6 iOS:
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return (interfaceOrientation == UIInterfaceOrientationPortrait);
}

@end
