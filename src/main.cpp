#include "testApp.h"
#include "ofAppGlutWindow.h"

int main()
{
	ofSetLogLevel(OF_LOG_VERBOSE);
	ofAppGlutWindow window;
	ofSetupOpenGL(&window, 640, 480, OF_WINDOW);
	ofRunApp(new testApp());
}
