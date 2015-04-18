 /*
     File: PaintingView.m
 Abstract: The class responsible for the finger painting. The class wraps the 
 CAEAGLLayer from CoreAnimation into a convenient UIView subclass. The view 
 content is basically an EAGL surface you render your OpenGL scene into.
  Version: 1.12
 Copyright (C) 2013 Apple Inc. All Rights Reserved.
*/

#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>
#import <GLKit/GLKit.h>

#import "PaintingView.h"
#import "shaderUtil.h"
#import "debug.h"

#include <map>
#include <vector>
#include "TFile.h"
#include "TUtils.h"
#include "TLogging.h"

using namespace std;

// how transparent the brush is
#define kBrushOpacity		(1.0)
// our snap to grid size (i.e. we quantize coordinates to this
#define kBrushPixelStep		3
// controls brush point size
#define kBrushScale			4


// Shaders
enum {
    PROGRAM_POINT,
    NUM_PROGRAMS
};

enum {
	UNIFORM_MVP,
    UNIFORM_POINT_SIZE,
    UNIFORM_VERTEX_COLOR,
    UNIFORM_TEXTURE,
	NUM_UNIFORMS
};

enum {
	ATTRIB_VERTEX,
	NUM_ATTRIBS
};

typedef struct {
	char *vert, *frag;
	GLint uniform[NUM_UNIFORMS];
	GLuint id;
} programInfo_t;

programInfo_t program[NUM_PROGRAMS] = {
    { "point.vsh",   "point.fsh" },     // PROGRAM_POINT
};


// Texture
typedef struct {
    GLuint id;
    GLsizei width, height;
} textureInfo_t;


@interface PaintingView()
{
	// The pixel dimensions of the backbuffer
	GLint backingWidth;
	GLint backingHeight;
	
	EAGLContext *context;
	
	// OpenGL names for the renderbuffer and framebuffers used to render to this view
	GLuint viewRenderbuffer, viewFramebuffer;
	
	// OpenGL name for the depth buffer that is attached to viewFramebuffer, if it exists (0 if it does not exist)
	GLuint depthRenderbuffer;
	
	textureInfo_t brushTexture;     // brush texture
	CGFloat brushColor[4];          // brush color
   
	Boolean needsErase;
	
	// Shader objects
	GLuint vertexShader;
	GLuint fragmentShader;
	GLuint shaderProgram;
	
	// Buffer Objects
	GLuint vboId;
	
	std::vector<SwipePoint> arrSwipePoints;
	std::map<std::string, Vocab> r3l;
	std::map<std::string, Vocab> r2l;
	
	BOOL initialized;

}

@end

@implementation PaintingView

@synthesize  sLoc;
@synthesize  sLocPrev;
@synthesize nIndexLast; // used to keep the color index of the brush

// Implement this to override the default layer class (which is [CALayer class]).
// We do this so that our view will be backed by a layer that is capable of OpenGL ES rendering.
+ (Class)layerClass
{
	return [CAEAGLLayer class];
}

// The GL view is stored in the nib file. When it's unarchived it's sent -initWithCoder:
- (id)initWithCoder:(NSCoder*)coder {
	
	//fix
	//676 entries, none with > .0023
	TFileReader fr2l(TUtils::pathForResource("count_2l.txt"));
	r2l = fr2l.readVocab();
	
	//Test r2l
	//std::map<std::string, Vocab>::iterator iter;
	//for (iter = r2l.begin(); iter != r2l.end(); ++iter) {
	//	Vocab* pData = (Vocab*)&iter->second;
	//	ZLogDebug("str:%s prob:%f pint:%d",pData->d.c_str(), pData->p, pData->pint);
	//}
	
	TFileReader fr3l(TUtils::pathForResource("count_3l.txt"));
	r3l = fr3l.readVocab();
	
    if ((self = [super initWithCoder:coder])) {
		 CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;
		 eaglLayer.opaque = YES;
		 self.backgroundColor = [UIColor clearColor];
		 CGColorSpaceRef rgb = CGColorSpaceCreateDeviceRGB();
		 const CGFloat myColor[] = {0.0, 0.0, 0.0, 0.0};
		 eaglLayer.backgroundColor = CGColorCreate(rgb, myColor);
		 CGColorSpaceRelease(rgb);
		 
		// In this application, we want to retain the EAGLDrawable contents after a call to presentRenderbuffer.
		eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
										[NSNumber numberWithBool:YES], kEAGLDrawablePropertyRetainedBacking, kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat, nil];
		
		context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
		
		if (!context || ![EAGLContext setCurrentContext:context]) {
			[self release];
			return nil;
		}
        
        // Set the view's scale factor as you wish
        self.contentScaleFactor = [[UIScreen mainScreen] scale];
        
		// Make sure to start with a cleared buffer
		needsErase = YES;
	}
	
	return self;
}

// If our view is resized, we'll be asked to layout subviews.
// This is the perfect opportunity to also update the framebuffer so that it is
// the same size as our display area.
-(void)layoutSubviews
{
	[EAGLContext setCurrentContext:context];
	
	if (!initialized) {
		initialized = [self initGL];
	}
	else {
		[self resizeFromLayer:(CAEAGLLayer*)self.layer];
	}
	
	// Clear the framebuffer the first time it is allocated
	if (needsErase) {
		[self eraseMe];
		needsErase = NO;
	}
}

- (void)setupShaders
{
	for (int i = 0; i < NUM_PROGRAMS; i++)
	{
        // FIXME: test
        const char* nm = TUtils::pathForResource(program[i].vert);
		char *vsrc = TUtils::readFile(nm);
        nm = TUtils::pathForResource(program[i].frag);
		char *fsrc = TUtils::readFile(nm);
		GLsizei attribCt = 0;
		GLchar *attribUsed[NUM_ATTRIBS];
		GLint attrib[NUM_ATTRIBS];
		GLchar *attribName[NUM_ATTRIBS] = { "inVertex", };
		const GLchar *uniformName[NUM_UNIFORMS] = {
			"MVP", "pointSize", "vertexColor", "texture",
		};
		
		// auto-assign known attribs
		for (int j = 0; j < NUM_ATTRIBS; j++)
		{
			if (strstr(vsrc, attribName[j]))
			{
				attrib[attribCt] = j;
				attribUsed[attribCt++] = attribName[j];
			}
		}
		
		glueCreateProgram(vsrc, fsrc,
								attribCt, (const GLchar **)&attribUsed[0], attrib,
								NUM_UNIFORMS, &uniformName[0], program[i].uniform,
								&program[i].id);
		
		free(vsrc);
		free(fsrc);
		
		// Set constant/initalize uniforms
		if (i == PROGRAM_POINT)
		{
			glUseProgram(program[PROGRAM_POINT].id);
			
			// the brush texture will be bound to texture unit 0
			glUniform1i(program[PROGRAM_POINT].uniform[UNIFORM_TEXTURE], 0);
			
			// viewing matrices
			GLKMatrix4 projectionMatrix = GLKMatrix4MakeOrtho(0, backingWidth, 0, backingHeight, -1, 1);
			GLKMatrix4 modelViewMatrix = GLKMatrix4Identity; // this sample uses a constant identity modelView matrix
			GLKMatrix4 MVPMatrix = GLKMatrix4Multiply(projectionMatrix, modelViewMatrix);
			
			glUniformMatrix4fv(program[PROGRAM_POINT].uniform[UNIFORM_MVP], 1, GL_FALSE, MVPMatrix.m);
			
			// point size
			glUniform1f(program[PROGRAM_POINT].uniform[UNIFORM_POINT_SIZE], brushTexture.width / kBrushScale);
			
			// initialize brush color
			glUniform4fv(program[PROGRAM_POINT].uniform[UNIFORM_VERTEX_COLOR], 1, brushColor);
		}
	}
	
	glError();
}

// Create a texture from an image
- (textureInfo_t)textureFromName:(NSString *)name
{
	CGImageRef		brushImage;
	CGContextRef	brushContext;
	GLubyte			*brushData;
	size_t			width, height;
	GLuint          texId;
	textureInfo_t   texture;
	
	// First create a UIImage object from the data in a image file, and then extract the Core Graphics image
	brushImage = [UIImage imageNamed:name].CGImage;
	
	// Get the width and height of the image
	width = CGImageGetWidth(brushImage);
	height = CGImageGetHeight(brushImage);
	
	// Make sure the image exists
	if(brushImage) {
		// Allocate  memory needed for the bitmap context
		brushData = (GLubyte *) calloc(width * height * 4, sizeof(GLubyte));
		// Use  the bitmatp creation function provided by the Core Graphics framework.
		brushContext = CGBitmapContextCreate(brushData, width, height, 8, width * 4, CGImageGetColorSpace(brushImage), kCGImageAlphaPremultipliedLast);
		// After you create the context, you can draw the  image to the context.
		CGContextDrawImage(brushContext, CGRectMake(0.0, 0.0, (CGFloat)width, (CGFloat)height), brushImage);
		// You don't need the context at this point, so you need to release it to avoid memory leaks.
		CGContextRelease(brushContext);
		// Use OpenGL ES to generate a name for the texture.
		glGenTextures(1, &texId);
		// Bind the texture name.
		glBindTexture(GL_TEXTURE_2D, texId);
		// Set the texture parameters to use a minifying filter and a linear filer (weighted average)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		// Specify a 2D texture image, providing the a pointer to the image data in memory
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, brushData);
		// Release  the image data; it's no longer needed
		free(brushData);
		
		texture.id = texId;
		texture.width = width;
		texture.height = height;
	}
	
	return texture;
}

- (BOOL)initGL
{
    // Generate IDs for a framebuffer object and a color renderbuffer
	glGenFramebuffers(1, &viewFramebuffer);
	glGenRenderbuffers(1, &viewRenderbuffer);
	
	glBindFramebuffer(GL_FRAMEBUFFER, viewFramebuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, viewRenderbuffer);
	// This call associates the storage for the current render buffer with the EAGLDrawable (our CAEAGLLayer)
	// allowing us to draw into a buffer that will later be rendered to screen wherever the layer is (which corresponds with our view).
	[context renderbufferStorage:GL_RENDERBUFFER fromDrawable:(id<EAGLDrawable>)self.layer];
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, viewRenderbuffer);
	
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &backingWidth);
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &backingHeight);
	
	/*
	// For this sample, we do not need a depth buffer. If you do, this is how you can create one and attach it to the framebuffer:
	glGenRenderbuffers(1, &depthRenderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, backingWidth, backingHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
	*/
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		NSLog(@"failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
		return NO;
	}
    
    // Setup the view port in Pixels
    glViewport(0, 0, backingWidth, backingHeight);
    
    // Create a Vertex Buffer Object to hold our data
    glGenBuffers(1, &vboId);
    
    // Load the brush texture
    brushTexture = [self textureFromName:@"Particle.png"];
    
    // Load shaders
    [self setupShaders];
    
    // Enable blending and set a blending function appropriate for premultiplied alpha pixel data
    glEnable(GL_BLEND);
	// GL_SRC_ALPHA is another option for GL_ONE
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    return YES;
}

- (BOOL)resizeFromLayer:(CAEAGLLayer *)layer
{
	// Allocate color buffer backing based on the current layer size
    glBindRenderbuffer(GL_RENDERBUFFER, viewRenderbuffer);
    [context renderbufferStorage:GL_RENDERBUFFER fromDrawable:layer];
	glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &backingWidth);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &backingHeight);
	
	/*
    // For this sample, we do not need a depth buffer. If you do, this is how you can allocate depth buffer backing:
	glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, backingWidth, backingHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);
	 */
	
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		NSLog(@"Failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
		return NO;
    }
    
    // Update projection matrix
    GLKMatrix4 projectionMatrix = GLKMatrix4MakeOrtho(0, backingWidth, 0, backingHeight, -1, 1);
    GLKMatrix4 modelViewMatrix = GLKMatrix4Identity; // this sample uses a constant identity modelView matrix
    GLKMatrix4 MVPMatrix = GLKMatrix4Multiply(projectionMatrix, modelViewMatrix);
    
    glUseProgram(program[PROGRAM_POINT].id);
    glUniformMatrix4fv(program[PROGRAM_POINT].uniform[UNIFORM_MVP], 1, GL_FALSE, MVPMatrix.m);
    
    // Update viewport
    glViewport(0, 0, backingWidth, backingHeight);
	
    return YES;
}

// Releases resources when they are not longer needed.
- (void)dealloc
{
	arrSwipePoints.clear();

	// Destroy framebuffers and renderbuffers
	if (viewFramebuffer) {
        glDeleteFramebuffers(1, &viewFramebuffer);
        viewFramebuffer = 0;
    }
    if (viewRenderbuffer) {
        glDeleteRenderbuffers(1, &viewRenderbuffer);
        viewRenderbuffer = 0;
    }
	if (depthRenderbuffer)
	{
		glDeleteRenderbuffers(1, &depthRenderbuffer);
		depthRenderbuffer = 0;
	}
    // texture
    if (brushTexture.id) {
		glDeleteTextures(1, &brushTexture.id);
		brushTexture.id = 0;
	}
    // vbo
    if (vboId) {
        glDeleteBuffers(1, &vboId);
        vboId = 0;
    }
    
    // tear down context
	if ([EAGLContext currentContext] == context)
        [EAGLContext setCurrentContext:nil];
	
	[context release];
    
	[super dealloc];
}

// Erases the screen
- (void)eraseMe
{
	[EAGLContext setCurrentContext:context];
	
	// Clear the buffer
	glBindFramebuffer(GL_FRAMEBUFFER, viewFramebuffer);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(GL_COLOR_BUFFER_BIT);
	
	// Display the buffer
	glBindRenderbuffer(GL_RENDERBUFFER, viewRenderbuffer);
	[context presentRenderbuffer:GL_RENDERBUFFER];
    
    // clear here?
    arrSwipePoints.clear();
}

// Drawings a line onscreen based on where the user touches
- (void)renderLineFromPoint:(CGPoint)start toPoint:(CGPoint)end
{
	static GLfloat*		vertexBuffer = NULL;
	static NSUInteger	vertexMax = 64;
	NSUInteger			vertexCount = 0,
						count,
						i;
	
	[EAGLContext setCurrentContext:context];
	glBindFramebuffer(GL_FRAMEBUFFER, viewFramebuffer);
	
	// Convert locations from Points to Pixels
	CGFloat scale = self.contentScaleFactor;
	start.x *= scale;
	start.y *= scale;
	end.x *= scale;
	end.y *= scale;
	
	// Allocate vertex array buffer
	if(vertexBuffer == NULL)
		vertexBuffer = (GLfloat*)malloc(vertexMax * 2 * sizeof(GLfloat));
	
	// Add points to the buffer so there are drawing points every X pixels
	count = MAX(ceilf(sqrtf((end.x - start.x) * (end.x - start.x) + (end.y - start.y) * (end.y - start.y)) / kBrushPixelStep), 1);
	for(i = 0; i < count; ++i) {
		if(vertexCount == vertexMax) {
			vertexMax = 2 * vertexMax;
			vertexBuffer = (GLfloat*)realloc(vertexBuffer, vertexMax * 2 * sizeof(GLfloat));
		}
		
		vertexBuffer[2 * vertexCount + 0] = start.x + (end.x - start.x) * ((GLfloat)i / (GLfloat)count);
		vertexBuffer[2 * vertexCount + 1] = start.y + (end.y - start.y) * ((GLfloat)i / (GLfloat)count);
		vertexCount += 1;
	}
    
	// Load data to the Vertex Buffer Object
	glBindBuffer(GL_ARRAY_BUFFER, vboId);
	// GL_DYNAMIC_DRAW or GL_STATIC_DRAW?
	glBufferData(GL_ARRAY_BUFFER, vertexCount*2*sizeof(GLfloat), vertexBuffer, GL_DYNAMIC_DRAW);
	
	glEnableVertexAttribArray(ATTRIB_VERTEX);
	glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, GL_FALSE, 0, 0);
	
	// Draw
	glUseProgram(program[PROGRAM_POINT].id);
	glDrawArrays(GL_POINTS, 0, vertexCount);
	
	// Display the buffer
	glBindRenderbuffer(GL_RENDERBUFFER, viewRenderbuffer);
	[context presentRenderbuffer:GL_RENDERBUFFER];
}

- (void)playback
{
	for (int i=1; i<arrSwipePoints.size(); i++) {
		CGPoint pt1 = arrSwipePoints[i].pPoint;
		CGPoint pt2 = arrSwipePoints[i-1].pPoint;
		[self renderLineFromPoint:pt1 toPoint:pt2];
	}
}

-(NSString*)getSwypedWord
{
    std::vector<SwipePoint> arrText;
    
	if (arrSwipePoints.size()==0)
		return nullptr;
	
	if (arrSwipePoints.size() == 1) {
		if (arrSwipePoints[0].key>=0)
			arrText.push_back(arrSwipePoints[0]);

		return nullptr;
	}
	
TLogDebug("CAPTURE LETTERS");
	
	vector<SwipePoint> arrTemp;
	
	int i;
	// setup the first key info, based on what we know
	int nKey = arrSwipePoints[0].key;
	int nTLast = arrSwipePoints[0].pTime.asMs();
	int nMs = 17;
	int nCnt = 1;
	int nFirstKey = -1;
	
	int nAngleMin, nAngleMax;
	int nAngleMinLast, nAngleMaxLast;
	float nVelMin, nVelMax;
	float nVelMinLast, nVelMaxLast;
	float fDist = 0;
	nAngleMin = nAngleMax = -1000;
	nAngleMinLast = nAngleMaxLast = 0;
	nVelMin = nVelMax = arrSwipePoints[0].fVelocity;
	nVelMinLast = nVelMaxLast = 0;
	int nSize = arrSwipePoints.size();
	// not so smart algorithm
	for (i=1; i<nSize; i++) {
		if (nKey>=0)
		{
		// if we have the same key, then measure how long we have stayed hovering over it
			if (arrSwipePoints[i].key == nKey) {
				nMs += (arrSwipePoints[i].pTime.asMs() - nTLast);
				if (arrSwipePoints[i].fVelocity<nVelMin)
					nVelMin = arrSwipePoints[i].fVelocity;
				if (arrSwipePoints[i].fVelocity>nVelMax)
					nVelMax = arrSwipePoints[i].fVelocity;
				if (abs(arrSwipePoints[i].nAngle)<abs(nAngleMin))
					nAngleMin = arrSwipePoints[i].nAngle;
				if (abs(arrSwipePoints[i].nAngle)>abs(nAngleMax))
					nAngleMax = arrSwipePoints[i].nAngle;
				fDist += arrSwipePoints[i].fDist;
				nCnt++;
			}
			else {
				
				// this algorithm is wrong for predicting what should
				// be, but it shows us the p of what we captured
				

				std::string _bi;
				std::string _tri;
				float bi = 1;
				float tri = bi;
				if (i>1)
				{
					char c1 = static_cast<char>(arrSwipePoints[i-2].key);
					char c2 = static_cast<char>(arrSwipePoints[i-1].key);
					char c3 = static_cast<char>(arrSwipePoints[i].key);
				
					_bi += c1;
					_bi += c2;
					_tri = _bi;
					_tri += c3;
					tri = r3l[_tri].p;
					bi = r2l[_bi].p;
				}
				
				int nAbsAngleMinDiff = abs(nAngleMin - nAngleMinLast);
				int nAbsAngleMaxDiff = abs(nAngleMax - nAngleMaxLast);
				if (
					 (nFirstKey==-1) ||
					 ((tri>0.000017) && (nCnt>1)) ||
					 (nMs>100) ||
					 ((tri>0.000017) && (nMs>37) && ((nAbsAngleMaxDiff>45) && (nAbsAngleMaxDiff<360)))
					 )
				{
					nFirstKey = nKey;
					// OK so we are grabbing this swipe minus 1
					SwipePoint pt = arrSwipePoints[i-1];
					pt.fVelocity = nVelMaxLast;
					// angle was originally determined as the angle of the line from last to current
					// there is probably a lot more we could do with this analysis of angle to make
					// this metric more useful - right now we are looking for angle least not angle max
					// for example
					pt.nAngle = nAbsAngleMaxDiff;
					pt.nCnt = nCnt;
					pt.nMs = nMs;
					pt.key = nKey;
					pt.fDist = fDist;
					arrTemp.push_back(pt);
					
TLogDebug("char:%c #:%d ms:%d >:%d bi:%f:%s tri:%f:%s ", static_cast<char>(pt.key), pt.nCnt, pt.nMs, pt.nAngle, bi, _bi.c_str(), tri, _tri.c_str());
TLogDebug("vel:%d dist:%d", (int)pt.fVelocity, (int)pt.fDist);
				}
				nAngleMaxLast = nAngleMax;
				nVelMaxLast = nVelMax;
				nAngleMinLast = nAngleMin;
				nVelMinLast = nVelMin;
				nAngleMin = nAngleMax = arrSwipePoints[i].nAngle;
				nVelMin = nVelMax = arrSwipePoints[i].fVelocity;
				nMs = 0;
				nCnt = 0;
				fDist = 0;
			}
		}
		nKey = arrSwipePoints[i].key;
		nTLast = arrSwipePoints[i].pTime.asMs();
 	}
	// get last key
	if (nCnt && nMs)
	{
		SwipePoint pt = arrSwipePoints[i];
		pt.fVelocity = nVelMin;
		pt.nAngle = nAngleMin;
		pt.nCnt = nCnt;
		pt.nMs = nMs;
		pt.key = nKey;
		arrTemp.push_back(pt);
	}

	
	
TLogDebug("--NEW WORD--")
	for(auto pt: arrTemp) {
TLogDebug("c:%c #:%d ms:%d >:%d vel:%f dist:%f", static_cast<char>(pt.key), pt.nCnt, pt.nMs, pt.nAngle, pt.fVelocity, pt.fDist);
		arrText.push_back(pt);
	}
    
    NSMutableString *outstring = [[NSMutableString alloc] init];
    
    if (arrText.size()) {
        for (int i=0; i<arrText.size(); i++)
        {
            NSString * temp = [NSString stringWithFormat:@"%c", arrText[i].key];
            [outstring appendString: temp];
        }
    }
	
	// TODO: lambda analyze FINAL
	// for now we instead just clear the history
	//arrSwipePoints.clear();
    
    return outstring;
}


- (bool)isVowel:(int)mychar
{
//	if (mychar)
	return false;
}

// Handles the start of a touch
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{   
	CGRect				bounds = [self bounds];
	CGPoint curLoc = [[touches anyObject] locationInView:self];
	
	// START OF FIRST FINGER DOWN?
	// Convert touch point from UIView referential to OpenGL one (upside-down flip)
	sLoc.pPoint = curLoc;
	sLoc.pPoint.y = bounds.size.height - sLoc.pPoint.y;
	sLoc.pTime.setToNow();
	sLoc.key = _nKeyLast;
	
	// more verbose
	//ZLogInfo("BEGAN: %fx%f %fx%f",sLocPrev.pPoint.x, sLocPrev.pPoint.y,sLoc.pPoint.x, sLoc.pPoint.y);
	
	// we store the swipe for analysis here
	arrSwipePoints.push_back(sLoc);
}

// bounds contains the size of the screen palette
// location contains the touch point

// the openGL coordinate system is apparently more 
// Handles the continuation of a touch.
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
    [self.superview touchesMoved:touches withEvent:event];

    [super touchesMoved:touches withEvent:event];
	CGRect				bounds = [self bounds];
	CGPoint prevLoc = [[touches anyObject] previousLocationInView:self];
	CGPoint curLoc = [[touches anyObject] locationInView:self];

	// APPLE NICELY STORES LAST AND CUR FOR US? 
	sLoc.pPoint = curLoc;
	sLoc.pPoint.y = bounds.size.height - sLoc.pPoint.y;
	
	sLocPrev.pPoint = prevLoc;
	sLocPrev.pPoint.y = bounds.size.height - sLocPrev.pPoint.y;
		
	// swap times
	sLocPrev.pTime = sLoc.pTime;
	sLoc.pTime.setToNow();
	sLoc.key = _nKeyLast;

	float fVelocity = 0;
	int nDistance = 0;
	int nTime = 0;
	sLoc.ComparePointVsLast(sLocPrev, fVelocity, nDistance, nTime);
	if (fVelocity<=0) fVelocity = 1;
	CGFloat nBrush = (kPaletteSize/fVelocity);

	[self setBrushColorWithIndex:nBrush];

	// we store the swipe for analysis here
	arrSwipePoints.push_back(sLoc);

	// Render the stroke
	[self renderLineFromPoint:sLocPrev.pPoint toPoint:sLoc.pPoint];
	
	// TODO: lambda analyze FINAL
	//ZLogInfo("MOVING: %fx%f ms:%d dist:%d vel:%f brush:%d", sLoc.pPoint.x, sLoc.pPoint.y, nTime, nDistance, fVelocity, nBrush);
}

// Handles the end of a touch event when the touch is a tap.
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    [self.superview touchesEnded:touches withEvent:event];
    
	CGRect				bounds = [self bounds];
	CGPoint prevLoc = [[touches anyObject] previousLocationInView:self];
	CGPoint curLoc = [[touches anyObject] locationInView:self];

	sLoc.pPoint = curLoc;
	sLoc.pPoint.y = bounds.size.height - sLoc.pPoint.y;
	
	sLocPrev.pPoint = prevLoc;
	sLocPrev.pPoint.y = bounds.size.height - sLocPrev.pPoint.y;
	
	// swap times
	sLocPrev.pTime = sLoc.pTime;
	sLoc.pTime.setToNow();
	sLoc.key = _nKeyLast;
	
	float fVelocity = 0;
	int nDistance = 0;
	int nTime = 0;
	sLoc.ComparePointVsLast(sLocPrev, fVelocity, nDistance, nTime);
	if (fVelocity<=0) fVelocity = 1;
	CGFloat nBrush = (1/fVelocity);
	
	[self setBrushColorWithIndex:nBrush];
	
	// we store the swipe for analysis here
	arrSwipePoints.push_back(sLoc);
	
	[self renderLineFromPoint:sLocPrev.pPoint toPoint:sLoc.pPoint];
	
	// REMOVE ME- this is for debugging
	//[self playback];
	
	[self eraseMe];
	
	// very verbose
	//ZLogInfo("END: %fx%f ms:%d dist:%d vel:%f brush:%d", sLoc.pPoint.x, sLoc.pPoint.y, nTime, nDistance, fVelocity, nBrush);
}

// Handles the end of a touch event.
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
    [self.superview touchesCancelled:touches withEvent:event];
	// If appropriate, add code necessary to save the state of the application.
	// This application is not saving state.
	
}

- (void)setBrushColorWithIndex:(NSInteger)nBrush{

	if ((nBrush < kPaletteSize) && (nBrush>=0) && (nBrush != nIndexLast))
	{
		// Define a new brush color
		CGColorRef color = [UIColor colorWithHue:(CGFloat)nBrush / (CGFloat)kPaletteSize
												saturation:kSaturation
												brightness:kBrightness
													  alpha:1.0].CGColor;
		
		const CGFloat *components = CGColorGetComponents(color);
		nIndexLast = nBrush;
		
		// Defer to the OpenGL view to set the brush color
		[self setBrushColor:components[0] green:components[1] blue:components[2]];
	}
}

- (void)setBrushColor:(CGFloat)red green:(CGFloat)green blue:(CGFloat)blue
{
	// Update the brush color
    brushColor[0] = red * kBrushOpacity;
    brushColor[1] = green * kBrushOpacity;
    brushColor[2] = blue * kBrushOpacity;
    brushColor[3] = kBrushOpacity;
    
    if (initialized) {
        glUseProgram(program[PROGRAM_POINT].id);
        glUniform4fv(program[PROGRAM_POINT].uniform[UNIFORM_VERTEX_COLOR], 1, brushColor);
    }
}

@end
