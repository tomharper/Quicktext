/*
     File: PaintingView.h
 Abstract: The class responsible for the finger painting. The class wraps the 
 CAEAGLLayer from CoreAnimation into a convenient UIView subclass. The view 
 content is basically an EAGL surface you render your OpenGL scene into.
  Version: 1.12
 Copyright (C) 2013 Apple Inc. All Rights Reserved.
 
*/

#import <UIKit/UIKit.h>
#import <OpenGLES/EAGL.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import "TDateTime.h"
#include "Math.h"
#include <vector>


#define kFont [UIFont fontWithName:@"Courier-Bold" size:20]
#define kAltLabel @"123"
#define kReturnLabel @"return"
#define kSpaceLabel @"space"
#define kChar @[ @"q", @"w", @"e", @"r", @"t", @"y", @"u", @"i", @"o", @"p", @"@", @"?", @"a", @"s", @"d", @"f", @"g", @"h",	@"j", @"k", @"l", @".", @"z", @"x", @"c", @"v", @"b", @"n", @"m", @"!", @"ਯ" ]
#define kChar_shift @[ @"Q", @"W", @"E", @"R", @"T", @"Y", @"U", @"I", @"O", @"P", @"@", @"?", @"A", @"S", @"D", @"F", @"G", @"H", @"J", @"K", @"L", @".", @"Z", @"X", @"C", @"V", @"B", @"N", @"M", @"!", @"ਞ" ]
#define kChar_alt @[ @"1", @"2", @"3", @"4", @"5", @"6", @"7", @"8", @"9", @"0", @"*", @"[", @"]", @"/", @":", @"(", @")", @"#", @"$", @"%", @"'", @"\"", @"{", @"}", @"-", @"+", @"=", @";", @"<", @">" ]


//CONSTANTS:
// DEFINES FOR BRUSH COLORS
#define kBrightness             1.0
//0.45
#define kSaturation             1.0

// # of colors that we want to use- this is pretty much hard coded, as we
// currently don't generate the colors, we use pngs
#define kPaletteSize			17
#define kMinEraseInterval		0.5

/*
class ProximityMap {
}
class ProximityKey {
	int nKeyN;
	int n
	int nKeyS;
	int nKeyWest;
}
 */
#define PI 3.14159265
// TODO: probably there is a better representation for the letters/characters than int
class SwipePoint {
public:
	SwipePoint() {
		//pTime.setToNow();
		nMs = 0;
		pPoint.x = 0;
		pPoint.y = 0;
		key = -1;
		nCnt = 0;
		fVelocity = 0.0F;
		nAngle = 0;
	}
	~SwipePoint() {};
	CGPoint pPoint;
	TDateTime pTime;
	int nMs;
	int nCnt;
	int key;
	float fVelocity;
	float fDist;
	int nAngle;
	
	void ComparePointVsLast(SwipePoint prev, float& nVelocity, int& nDist, int& nTime)
	{
		//nTime = pTime.asMs() - prev.pTime.asMs();
	
		int nDistX = pPoint.x - prev.pPoint.x;
		int nDistY = pPoint.y - prev.pPoint.y;
		if (nTime<=0) nTime = 1;
		
		fDist = sqrt(nDistX*nDistX + nDistY*nDistY);
		fVelocity = fDist/(float)nTime;
		nAngle = atan2(nDistY, nDistX) * 180.0 / PI;
		
		nVelocity = fVelocity;
		nDist = fDist;
	}
};


@interface PaintingView : UIView <UIInputViewAudioFeedback>

@property(nonatomic, readwrite) SwipePoint sLoc;
@property(nonatomic, readwrite) SwipePoint sLocPrev;
@property(nonatomic, readwrite) NSInteger nIndexLast;
@property(nonatomic, readwrite) NSInteger nKeyLast;


- (void)setBrushColor:(CGFloat)red green:(CGFloat)green blue:(CGFloat)blue;
- (void)setBrushColorWithIndex:(NSInteger)nIndex;
- (NSString*)getSwypedWord;

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event;
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event;
@end
