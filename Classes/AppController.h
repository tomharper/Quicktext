/*
     File: AppController.h
 Abstract: The UIApplication delegate class.
  Version: 1.12
 Copyright (C) 2013 Apple Inc. All Rights Reserved.
*/

//CLASS INTERFACES:
@class PaintingViewController;

@interface AppController : NSObject <UIApplicationDelegate>

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (strong, nonatomic) PaintingViewController *viewController;

@end
