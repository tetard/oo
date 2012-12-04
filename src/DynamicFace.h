//
//  DynamicFace.h
//  TetGrimace
//
//  Created by Camille Troillard on 20/11/12.
//
//

#ifndef __TetGrimace__DynamicFace__
#define __TetGrimace__DynamicFace__

#include "ofMain.h"
#include "ofxSyphonClient.h"

typedef vector<ofVec2f> PointVector;

class DynamicFace
{
public:
	enum Kind
	{
		INVALID,
		IMAGE,
		VIDEO,
		SYPHON
	};

	DynamicFace();
	DynamicFace(int width, int height, string syphonClientName);

	~DynamicFace();

	void update();
	void load(string filename);

	PointVector const& getPoints() const;

	void start();
	void stop();

	void bind();
	void unbind();

    void setVideoVolume(float volume);
    void setVideoSpeed(float speed);

protected:
	void loadPoints(string filename);
	void savePoints(string filename) const;

private:
	enum Kind kind;
	PointVector points;
	ofImage image;
	ofVideoPlayer video;

	ofxSyphonClient* syphonClient;
	ofFbo fboSyphon;
};


#endif /* defined(__TetGrimace__DynamicFace__) */
