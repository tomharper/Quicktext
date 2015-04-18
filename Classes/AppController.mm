/*
     File: AppController.m
 Abstract: The UIApplication delegate class.
  Version: 1.12
 Copyright (C) 2013 Apple Inc. All Rights Reserved.
*/

#import "AppController.h"
#import "PaintingViewController.h"

@implementation AppController

@synthesize window;

- (void)applicationDidFinishLaunching:(UIApplication*)application
{
	/*
#define TESTING 1
#ifdef TESTING
	
	// disabling for ios7
	if ([[UIDevice currentDevice] respondsToSelector:@selector(uniqueIdentifier)]) {
		NSString *identifier = [[UIDevice currentDevice] performSelector:@selector(uniqueIdentifier)];
		[TestFlight setDeviceIdentifier:identifier];
	}
	
#endif
	
	[TestFlight takeOff:kCredTestflightAppToken];
	*/
	
	// and do this only now that logging is setup to output to testflight ...
	//LogInfo("Client build version %@ (%@)\0",
	//			[[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleShortVersionString"],
	//			[[[NSBundle mainBundle] infoDictionary] objectForKey:(NSString *)kCFBundleVersionKey]);
	//Logging::logBuildInfo();
	
	
	// THIS IS THE OTHER WAY TO DO THIS SAME THING
	// This is the same as the AppDelegate
	self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];
	self.window.backgroundColor = [UIColor whiteColor];
	self.window.opaque = NO;
	[self.window makeKeyAndVisible];
	// Override point for customization after application launch.
	PaintingViewController *controller = [[PaintingViewController alloc] init];
	// we don't want to load PaintingView here, so we make up a "fake" view
	// and load it into this view controller- I am not sure why it loads PaintingView
	// by default, as I have removed storyboards, etc that would link the two
	controller.view = [[UIView alloc] init];
	[controller viewDidLoad];
	[self.window setRootViewController:controller];
	self.viewController = controller;
	[controller release];

    
    // Look in the Info.plist file and you'll see the status bar is hidden
	// Set the style to black so it matches the background of the application
	//[application setStatusBarStyle:UIStatusBarStyleBlackTranslucent animated:NO];
	// Now show the status bar, but animate to the style.
	[application setStatusBarHidden:YES];//  withAnimation:YES
	
	/*
	 // we aren't ready for this, but when we are, it goes here, and there is 
	 // more stuff to do to handle it
	[application registerForRemoteNotificationTypes:
	 UIRemoteNotificationTypeBadge |
	 UIRemoteNotificationTypeAlert |
	 UIRemoteNotificationTypeSound];
	 */
}

- (void)applicationWillTerminate:(UIApplication *)application
{
	// Saves changes in the application's managed object context before the application terminates.
	//
	// make sure this is killed before static destruction starts, as if its static destruction occurs after logging, then its attempt
	// to unregister itself from being a logging receiver won't end happily
	//

	//Logging_flush();
}



#pragma mark - Application's Documents directory

// Returns the URL to the application's Documents directory.
- (NSURL *)applicationDocumentsDirectory
{
	return [[[NSFileManager defaultManager] URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask] lastObject];
}


// Release resources when they are no longer needed,
- (void)dealloc
{
	[window release];
	[super dealloc];
}

@end
