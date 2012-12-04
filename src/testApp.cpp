#include "testApp.h"

using namespace ofxCv;

#define USE_NORMALIZED_OUTPUT 1


GLdouble modelviewMatrix[16], projectionMatrix[16];
GLint viewport[4];
void updateProjectionState() {
	glGetDoublev(GL_MODELVIEW_MATRIX, modelviewMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projectionMatrix);
	glGetIntegerv(GL_VIEWPORT, viewport);
}

ofVec3f ofWorldToScreen(ofVec3f world) {
	updateProjectionState();
	GLdouble x, y, z;
	gluProject(world.x, world.y, world.z, modelviewMatrix, projectionMatrix, viewport, &x, &y, &z);
	ofVec3f screen(x, y, z);
	screen.y = ofGetHeight() - screen.y;
	return screen;
}

ofMesh getProjectedMesh(const ofMesh& mesh) {
	ofMesh projected = mesh;
	for(int i = 0; i < mesh.getNumVertices(); i++) {
		ofVec3f cur = ofWorldToScreen(mesh.getVerticesPointer()[i]);
		cur.z = 0;
		projected.setVertex(i, cur);
	}
	return projected;
}

template <class T>
void addTexCoords(ofMesh& to, const vector<T>& from) {
	for(int i = 0; i < from.size(); i++) {
		to.addTexCoord(from[i]);
	}
}


#define MAKE_TRIANGLE_INDICES(m, a, b, c) m.addIndex(a); m.addIndex(b); m.addIndex(c);


testApp::testApp()
	: syphonFace(VIDEO_WIDTH, VIDEO_HEIGHT, "tet-input")
    , cameraInput(0)
	, useCamera(true)
    , setupComplete(false)
{
}

void testApp::clearBundle() {
	bundle.clear();
}

template <>
void testApp::addMessage(string address, ofVec3f data) {
	ofxOscMessage msg;
	msg.setAddress(address);
	msg.addFloatArg(data.x);
	msg.addFloatArg(data.y);
	msg.addFloatArg(data.z);
	bundle.addMessage(msg);
}

template <>
void testApp::addMessage(string address, ofVec2f data) {
	ofxOscMessage msg;
	msg.setAddress(address);
	msg.addFloatArg(data.x);
	msg.addFloatArg(data.y);
	bundle.addMessage(msg);
}

template <>
void testApp::addMessage(string address, float data) {
	ofxOscMessage msg;
	msg.setAddress(address);
	msg.addFloatArg(data);
	bundle.addMessage(msg);
}

template <>
void testApp::addMessage(string address, int data) {
	ofxOscMessage msg;
	msg.setAddress(address);
	msg.addIntArg(data);
	bundle.addMessage(msg);
}

void testApp::sendBundle() {
	osc.sendBundle(bundle);
}


void testApp::loadSettings()
{
	ofSetWindowShape(VIDEO_WIDTH / 2, VIDEO_HEIGHT / 2);
	ofSetVerticalSync(true);

	ofxXmlSettings xml;
	xml.loadFile("settings.xml");

	xml.pushTag("osc");
		// Send
		host = xml.getValue("host", "localhost");
		port = xml.getValue("port", 8338);
		osc.setup(host, port);
		
		// Receive
		receivePort = xml.getValue("receivePort", 8339);
		oscReceiver.setup(receivePort);
	xml.popTag();
	
	int camWidth = VIDEO_WIDTH;//xml.getValue("width", VIDEO_WIDTH);
	int camHeight = VIDEO_HEIGHT;//xml.getValue("height", VIDEO_HEIGHT);

	clone.setup(camWidth, camHeight);

	ofFbo::Settings settings;
	settings.width = camWidth;
	settings.height = camHeight;

	fboMask.allocate(settings);
	fboSrc.allocate(settings);

	fboSyphon.allocate(settings);
	fboCam.allocate(settings);
    fboCamDelayed.allocate(settings);
    fboWireframe.allocate(settings);
    
#if USE_NORMALIZED_OUTPUT
	fboCamNormalized.allocate(settings);
#endif

	syphonWireframe.setName("wireframe");
	syphonMask.setName("mask");
//    syphonMain.setName("main");
    
#if USE_NORMALIZED_OUTPUT
	syphonNormalized.setName("normalized");
#endif
    
	syphonInput.setup();
	syphonInput.setServerName("cam-input");
    // CanonOutput

	camTracker.setup();

	ofDirectory facesDir;
	facesDir.allowExt("jpg");
	facesDir.allowExt("jpeg");
	facesDir.allowExt("png");
	facesDir.allowExt("mov");
	
	facesDir.listDir("faces");

	int facesCount = facesDir.size();

	if(facesCount == 0)
	{
		ofLog() << "No face found, exiting.";
		ofExit(-1);
	}

	faces.resize(facesCount);
	
	for (int i = 0; i < facesCount; i++)
	{
		faces[i].load(facesDir.getPath(i));
	}

	ofLog(OF_LOG_VERBOSE) << facesCount << " images loaded.";
	updateCurrentFace(0);
    updateCameraInput();
}

void testApp::initCamera()
{
	if (!cameraInput)
	{
		ofLog(OF_LOG_VERBOSE) << "Initializing Camera Input";
		cameraInput = new ofVideoGrabber;

		int camWidth = VIDEO_WIDTH;//xml.getValue("width", VIDEO_WIDTH);
		int camHeight = VIDEO_HEIGHT;//xml.getValue("height", VIDEO_HEIGHT);
		
//		cameraInput->setDeviceID(xml.getValue("device", 0));
		
		int framerate = 30;//xml.getValue("framerate", -1); fps
        ofLog(OF_LOG_VERBOSE) << "Setting framerate to: "  << framerate;
        cameraInput->setDesiredFrameRate(framerate);
        ofSetFrameRate(framerate);
		
		cameraInput->initGrabber(camWidth, camHeight);	
	}	
}

void testApp::closeCamera()
{
	if (cameraInput)
	{
		ofLog(OF_LOG_VERBOSE) << "Closing Camera Input";
		delete cameraInput;
		cameraInput = NULL;
	}
}

void testApp::setup()
{
#ifdef TARGET_OSX
	ofSetDataPathRoot("../../../data/");
#endif

	wireframeEnabled = false;
	maskEnabled = true;
    orthoEnabled = true;
//    debugEnabled = false;
	filledMaskEnabled = false;
	shaderStrength = 16;
	currentFaceIndex = 0;
	
	loadSettings();
    setupComplete = true;
}

void testApp::sendOSC()
{
	bool found = camTracker.getFound();
	addMessage("/found", found ? 1 : 0);
	
	if (found)
	{
		ofVec2f position = camTracker.getPosition();
		addMessage("/pose/position", position);
		
		float scale = camTracker.getScale();
		addMessage("/pose/scale", scale);
		
		ofVec3f orientation = camTracker.getOrientation();
		addMessage("/pose/orientation", orientation);
		
		addMessage("/gesture/mouth/width", camTracker.getGesture(ofxFaceTracker::MOUTH_WIDTH));
		addMessage("/gesture/mouth/height", camTracker.getGesture(ofxFaceTracker::MOUTH_HEIGHT));
		
		addMessage("/gesture/eyebrow/left", camTracker.getGesture(ofxFaceTracker::LEFT_EYEBROW_HEIGHT));
		addMessage("/gesture/eyebrow/right", camTracker.getGesture(ofxFaceTracker::RIGHT_EYEBROW_HEIGHT));
		
		addMessage("/gesture/eye/left", camTracker.getGesture(ofxFaceTracker::LEFT_EYE_OPENNESS));
		addMessage("/gesture/eye/right", camTracker.getGesture(ofxFaceTracker::RIGHT_EYE_OPENNESS));
		addMessage("/gesture/jaw", camTracker.getGesture(ofxFaceTracker::JAW_OPENNESS));
		addMessage("/gesture/nostrils", camTracker.getGesture(ofxFaceTracker::NOSTRIL_FLARE));
		
		// Additional Tet messages:
		int size = camTracker.size();
		for (int i=0; i<size; i++)
		{
			addMessage("/point/image/" + ofToString(i), camTracker.getImagePoint(i));
			addMessage("/point/object/" + ofToString(i), camTracker.getObjectPoint(i));
		}
	}		
	
	if (bundle.getMessageCount() > 0)
	{
		sendBundle();
		clearBundle();
	}	
}

void testApp::receiveOSC()
{
	while (oscReceiver.hasWaitingMessages()) {
		ofxOscMessage msg;
		oscReceiver.getNextMessage(&msg);
		
        if (msg.getAddress() == "/reset") {
            camTracker.reset();
        } else if (msg.getAddress() == "/wireframe") {
			if (msg.getNumArgs() == 1)
                wireframeEnabled = msg.getArgAsInt32(0) == 0 ?
                false : true;			
		} else if (msg.getAddress() == "/mask") {
			if (msg.getNumArgs() == 1)
                maskEnabled = msg.getArgAsInt32(0) == 0 ?
                false : true;
        } else if (msg.getAddress() == "/ortho") {
			if (msg.getNumArgs() == 1)
                orthoEnabled = msg.getArgAsInt32(0) == 0 ?
                false : true;
		} else if (msg.getAddress() == "/map") {
			if (msg.getNumArgs() == 1)
                filledMaskEnabled = msg.getArgAsInt32(0) == 0 ?
                false : true;			
		} else if (msg.getAddress() == "/image") {
            if (msg.getNumArgs() == 1) {
				int index = msg.getArgAsInt32(0);
				updateCurrentFace(index);
			}
        } else if (msg.getAddress() == "/strength") {
            if (msg.getNumArgs() == 1) {
				float value = msg.getArgAsFloat(0);
				shaderStrength = (int) (value * 32.0);
				ofLog(OF_LOG_VERBOSE) << "Shader strength: " << shaderStrength;
			}
        } else if (msg.getAddress() == "/video_speed") {
            float speed = msg.getArgAsFloat(0);
            currentFace().setVideoSpeed(speed);
        } else if (msg.getAddress() == "/video_volume") {
            float volume = msg.getArgAsFloat(0);
            currentFace().setVideoVolume(volume);
        }
    }
}

void testApp::updateCameraInput()
{
    if (useCamera)
        initCamera();
    else
        closeCamera();
}

void testApp::updateInputFramebuffers()
{
	if (useCamera) {
		cameraInput->update();

		if (cameraInput->isFrameNew()) {
		    fboCam.begin();
		    {
		        cameraInput->draw(0, 0, fboCam.getWidth(), fboCam.getHeight());
		    }
		    fboCam.end();
		}
	} else {
	    
	    fboCam.begin();
	    {
            
            //  Draw camera image inverted
            //  inversion verticale
            //  ofPushMatrix();
            //  ofSetupScreenOrtho(fboCam.getWidth(), fboCam.getHeight(), OF_ORIENTATION_180, true);
            //  ofPopMatrix();
	        
            syphonInput.draw(0, 0, fboCam.getWidth(), fboCam.getHeight());
	    }
	    fboCam.end();
	}    
}



void testApp::update()
{
    if (cameraInput &&
        (cameraInput->isInitialized() == false)) {
        return;
    }

	receiveOSC();
    updateInputFramebuffers();
	currentFace().update();

	ofPixels pixels;
	fboCam.readToPixels(pixels);

    ofxTrackerScopedLock lock(camTracker);
    camTracker.update(toCv(pixels));			

    drawClone();
    sendOSC();

    fboCamDelayed.begin();
    {
        fboCam.draw(0, 0);
    }
    fboCamDelayed.end();
    
#if USE_NORMALIZED_OUTPUT
    drawNormalized();
#endif
    syphonWire();
}

void testApp::drawClone()
{
	int width = fboCam.getWidth();
	int height = fboCam.getHeight();
	
	bool found = camTracker.getFound();
	ofMesh camMesh = camTracker.getImageMesh();

	if (filledMaskEnabled)
		fillFaceMesh(camMesh);

	camMesh.clearTexCoords();
	camMesh.addTexCoords(currentFace().getPoints());

	fboMask.begin();
	{
		ofClear(0, 255);
		camMesh.draw();
	}
	fboMask.end();

	// cadre
/*	int baseIndex = 0;// camMesh.getNumVertices() + 1;
	camMesh = ofMesh();

	camMesh.addVertex(ofVec3f(0, 0, 0));						// bi
	camMesh.addVertex(ofVec3f(VIDEO_WIDTH, 0, 0));				// bi + 1
	camMesh.addVertex(ofVec3f(VIDEO_WIDTH, VIDEO_HEIGHT, 0));	// bi + 2
	camMesh.addVertex(ofVec3f(0, VIDEO_HEIGHT, 0));				// bi + 3

	MAKE_TRIANGLE_INDICES(camMesh, baseIndex, baseIndex + 1, baseIndex + 2);
	MAKE_TRIANGLE_INDICES(camMesh, baseIndex, baseIndex + 2, baseIndex + 3);

*/

	fboSrc.begin();
	{
		ofClear(0, 255);
		currentFace().bind();
		camMesh.draw();
		currentFace().unbind();
	}
	fboSrc.end();			

	clone.setStrength(shaderStrength);
	clone.update(maskEnabled && found,
				 fboSrc.getTextureReference(), 
				 fboCamDelayed.getTextureReference(),
				 fboMask.getTextureReference());

    fboSyphon.begin();
    {
		clone.draw(0, 0, width, height);
        
    }
    fboSyphon.end();
    
	ofTexture& texture = fboSyphon.getTextureReference();
	syphonMask.publishTexture(&texture);
    
//    fboWireframe.begin();
//    {
//		clone.draw(0, 0, width, height);
////        drawWireframe();
//        
//    }
//    fboWireframe.end();
//    
//	ofTexture& texture2 = fboWireframe.getTextureReference();
//	syphonWireframe.publishTexture(&texture2);
    
    
}

// this is important for avoiding slightl discrepencies when the mesh is
// projected, or processed by GL transforms vs OF transforms


void testApp::normalizeMesh(ofMesh& mesh)
{
	vector<ofVec3f>& vertices = mesh.getVertices();
	for(int i = 0; i < vertices.size(); i++) {
		vertices[i] *= normalizedMeshScale / normalizedWidth;
		vertices[i] += ofVec2f(normalizedWidth, normalizedHeight) / 2.;
		vertices[i].z = 0;
	}
}

void testApp::fillFaceMesh(ofMesh& mesh)
{
	//LEFT EYE
	MAKE_TRIANGLE_INDICES(mesh, 36, 41, 37);
	MAKE_TRIANGLE_INDICES(mesh, 41, 38, 37);
	MAKE_TRIANGLE_INDICES(mesh, 38, 41, 40);
	MAKE_TRIANGLE_INDICES(mesh, 38, 39, 40);
	
	// RIGHT EYE
	MAKE_TRIANGLE_INDICES(mesh, 42, 47, 43);
	MAKE_TRIANGLE_INDICES(mesh, 47, 44, 43);
	MAKE_TRIANGLE_INDICES(mesh, 44, 47, 46);
	MAKE_TRIANGLE_INDICES(mesh, 44, 45, 46);
	
	// MOUTH
	MAKE_TRIANGLE_INDICES(mesh, 48, 60, 65);
	MAKE_TRIANGLE_INDICES(mesh, 60, 61, 65);
	MAKE_TRIANGLE_INDICES(mesh, 61, 64, 65);
	MAKE_TRIANGLE_INDICES(mesh, 61, 62, 64);
	MAKE_TRIANGLE_INDICES(mesh, 64, 62, 63);
	MAKE_TRIANGLE_INDICES(mesh, 62, 54, 63);	
}

void testApp::drawNormalized()
{
	fboCamNormalized.begin();
	fboCamDelayed.getTextureReference().bind();
	{
		ofClear(0, 0);
		
		ofMesh mesh = camTracker.getMeanObjectMesh();
		fillFaceMesh(mesh);
		normalizeMesh(mesh);
		mesh.draw();
	}
	fboCamDelayed.getTextureReference().unbind();		
	fboCamNormalized.end();

	ofTexture& texture = fboCamNormalized.getTextureReference();
	syphonNormalized.publishTexture(&texture);
}


void testApp::drawWireframe()
{
//    fboCam.draw(0, 0);
        
    ofxTrackerScopedLock lock(camTracker);

    if(camTracker.getFound())
    {
        ofSetLineWidth(1.0);
        
        ofMesh mesh = camTracker.getImageMesh();
/*        
		int i = 66;
		int j = 67;
		int k = 68;
		int l = 69;
		
		mesh.addVertex(ofVec3f(0, 0, 0));						// i
		mesh.addVertex(ofVec3f(VIDEO_WIDTH, 0, 0));				// i + 1
		mesh.addVertex(ofVec3f(VIDEO_WIDTH, VIDEO_HEIGHT, 0));	// i + 2
		mesh.addVertex(ofVec3f(0, VIDEO_HEIGHT, 0));			// i + 3
		
		MAKE_TRIANGLE_INDICES(mesh, i,  0, 17);
		MAKE_TRIANGLE_INDICES(mesh, i, 17, 18);
		MAKE_TRIANGLE_INDICES(mesh, i, 18, 19);
		MAKE_TRIANGLE_INDICES(mesh, i, 19, 20);
		MAKE_TRIANGLE_INDICES(mesh, i, 20, 21);
//		MAKE_TRIANGLE_INDICES(mesh, baseIndex, baseIndex + 2, baseIndex + 3);
*/
		
        ofSetColor(0, 0, 0);
        mesh.drawWireframe();

        ofSetColor(0, 0, 0);
        camTracker.getImageFeature(ofxFaceTracker::LEFT_EYEBROW).draw();
        camTracker.getImageFeature(ofxFaceTracker::RIGHT_EYEBROW).draw();
        camTracker.getImageFeature(ofxFaceTracker::LEFT_JAW).draw();
        camTracker.getImageFeature(ofxFaceTracker::RIGHT_JAW).draw();
        camTracker.getImageFeature(ofxFaceTracker::JAW).draw();
        
        ofPolyline poly;
        poly = camTracker.getImageFeature(ofxFaceTracker::LEFT_EYE);
        poly.setClosed(true);
        poly.draw();
        
        poly = camTracker.getImageFeature(ofxFaceTracker::RIGHT_EYE);
        poly.setClosed(true);
        poly.draw();
        
        poly = camTracker.getImageFeature(ofxFaceTracker::OUTER_MOUTH);
        poly.setClosed(true);
        poly.draw();
        
        poly = camTracker.getImageFeature(ofxFaceTracker::INNER_MOUTH);
        poly.setClosed(true);
        poly.draw();
        
        ofSetColor(255, 255, 255);
        
        glPointSize(2);
        mesh.drawVertices();

        // Draw numbers
        int n = camTracker.size();
        for(int i = 0; i < n; i++){
            if(camTracker.getVisibility(i)) {
                ofDrawBitmapString(ofToString(i), camTracker.getImagePoint(i));
                
                
            }
        }
    }
}


void testApp::draw()
{
    int width = ofGetWidth();
    int height = ofGetHeight();
    
    
    if(orthoEnabled)
    {
    
	ofSetupScreenOrtho(0, 0, OF_ORIENTATION_180, false);
    }
    clone.draw(0, 0, width, height);

    if(wireframeEnabled)
    {
    ofScale(width / fboCam.getWidth(), height / fboCam.getHeight(), 1.0);
    drawWireframe();
    fboCamNormalized.draw (450,0,200,200);
    }
    
 }

void testApp::syphonWire()
{
    fboWireframe.begin();
    {
        
        clone.draw(0,0,640,480);
		drawWireframe();
        
    }
    fboWireframe.end();
    
	ofTexture& texture = fboWireframe.getTextureReference();
	syphonWireframe.publishTexture(&texture);
    
}



DynamicFace& testApp::currentFace()
{
	if (currentFaceIndex == -1)
		return syphonFace;
	else if (currentFaceIndex >= 0)
		return faces[currentFaceIndex];
}

void testApp::updateCurrentFace(int index)
{
    if (index == currentFaceIndex) {
        return;
    }
    
	currentFace().stop();

	if (index >= -1)
	{
		ofLog(OF_LOG_VERBOSE) << "Loading Face Index " << index;
		currentFaceIndex = index;
	}
	else
	{
		ofLog(OF_LOG_VERBOSE) << "Can't load index " << index << " (out of bounds)";
	}

	currentFace().start();
}

void testApp::keyPressed(int key)
{
	int index = currentFaceIndex;

	switch(key)
	{
		case OF_KEY_UP:
			index++;
			break;

		case OF_KEY_DOWN:
			index--;
			break;

		case 'i':
			useCamera = !useCamera;
            updateCameraInput();

			ofLog(OF_LOG_VERBOSE) << "Switched to " << (useCamera ? "camera" : "syphon") << " input.";
			break;
			
		case 'v':
			if (cameraInput)
				cameraInput->videoSettings();
			break;

		case 'f':
			filledMaskEnabled = !filledMaskEnabled;
			break;
			
		case ' ':
			camTracker.reset();
			break;

		case 'w':
			wireframeEnabled = !wireframeEnabled;
			ofLog(OF_LOG_VERBOSE) << "Wireframe " << (wireframeEnabled ? "ENABLED" : "DISABLED");
			break;

		case 'm':
			maskEnabled = !maskEnabled;
			ofLog(OF_LOG_VERBOSE) << "Mask " << (maskEnabled ? "ENABLED" : "DISABLED");
			break;
            
        case 'o':
			orthoEnabled = !orthoEnabled;
			ofLog(OF_LOG_VERBOSE) << "Ortho " << (orthoEnabled ? "ENABLED" : "DISABLED");
			break;
            
        case '@':
                ofToggleFullscreen();
            break;
           
            
            //        case 'd':
            //            debugEnabled = !debugEnabled;
            //            break;
            
	}
	
	updateCurrentFace(index);
}
