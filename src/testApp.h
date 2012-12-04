#pragma once

#include "ofMain.h"
#include "DynamicFace.h"
#include "ofxCv.h"
#include "Clone.h"
#include "ofxFaceTracker.h"
#include "ofxFaceTrackerThreaded.h"
#include "ofxOsc.h"
#include "ofxXmlSettings.h"
#include "ofxSyphonServer.h"


#define VIDEO_WIDTH  640
#define VIDEO_HEIGHT 480

#define TRACK_WIDTH  320
#define TRACK_HEIGHT 240

//#define VIDEO_WIDTH  640 // 853 is for a 16/9 camera
//#define VIDEO_HEIGHT 480

// HD settings
//#define VIDEO_WIDTH  1280
//#define VIDEO_HEIGHT 720

class testApp : public ofBaseApp
{
public:
	testApp();
	
	void clearBundle();
	template <class T>
	void addMessage(string address, T data);
	void sendBundle();
	
	void loadSettings();
	void setup();
    void updateCameraInput();
    void updateInputFramebuffers();
	void updateStaticFace();
	void update();
    void draw();
	void drawCam();
	void drawClone();
	void drawWireframe();
    void syphonWire();

	DynamicFace& currentFace();
	void updateCurrentFace(int index);

	void fillFaceMesh(ofMesh& mesh);
	void normalizeMesh(ofMesh& mesh);
	void drawNormalized();
	
	void keyPressed(int key);
	
    void sendOSC();
	void receiveOSC();

private:
	static const int normalizedWidth = VIDEO_WIDTH;
	static const int normalizedHeight = VIDEO_HEIGHT;
	
	// this (approximately) makes the mesh hit the edges of the fbos
	/*static const*/ float normalizedMeshScale = VIDEO_WIDTH * 7.8125;
	
	void initCamera();
	void closeCamera();

//  tetVideoDelay videoDelay;
	ofFbo fboCamDelayed;    
	ofFbo fboCam;    
	ofFbo fboCamNormalized;
	ofFbo fboSyphon;
	ofxSyphonServer syphonMask;
    ofxSyphonServer syphonWireframe;
//  ofxSyphonServer syphonMain;
	ofxSyphonServer syphonNormalized;


	ofxSyphonClient syphonInput;
	ofVideoGrabber* cameraInput;
	bool useCamera;

	ofxFaceTrackerThreaded camTracker;

	int currentFaceIndex;
	DynamicFace syphonFace;
	vector<DynamicFace> faces;

	Clone clone;
	ofFbo fboSrc, fboMask, fboWireframe ;
	
	bool wireframeEnabled;
	bool maskEnabled;
    bool orthoEnabled;
    bool filledMaskEnabled;
    bool debugEnabled;
	int shaderStrength;

	string host;
	int port;
	int receivePort;
	ofxOscBundle bundle;
	ofxOscSender osc;
    ofxOscReceiver oscReceiver;
    bool setupComplete;
    

    
};
