#pragma once

#include "ofxFaceTracker.h"


class ofxFaceTrackerThreaded : public ofThread
{
public:
	void setup()
	{
		resetTracking = false;
		startThread(false, false);
	}

	void update(cv::Mat mat)
	{
		ofxCv::copy(mat, buffer);
		newFrame = true;
	}

	void draw()
	{
		tracker->draw();
	}

	int size()
	{
		return tracker->size();
	}
	
	bool getVisibility(int i)
	{
		return tracker->getVisibility(i);
	}
	
	bool getFound()
	{
		return tracker->getFound();
	}

	ofMesh getImageMesh()
	{
		return tracker->getImageMesh();
	}
	
	ofPolyline getImageFeature(ofxFaceTracker::Feature feature)
	{
		return tracker->getImageFeature(feature);
	}

	ofVec2f getPosition()
	{
		return tracker->getPosition();
	}

	float getScale()
	{
		return tracker->getScale();
	}

	ofVec3f getOrientation()
	{
		return tracker->getOrientation();
	}
    
    ofMatrix4x4 getRotationMatrix() const
    {
        return tracker->getRotationMatrix();
    }
	
	float getGesture(ofxFaceTracker::Gesture gesture)
	{
		return tracker->getGesture(gesture);
	}
	
	ofVec2f getImagePoint(int i)
	{
		return tracker->getImagePoint(i);
	}

	ofVec3f getObjectPoint(int i)
	{
		return tracker->getObjectPoint(i);
	}

	ofMesh getMeanObjectMesh()
	{
		return tracker->getMeanObjectMesh();
	}
	
	void reset ()
	{
		if (lock())
		{
			resetTracking = true;
			unlock();
		}
	}
	
protected:
	void threadedFunction() {
		newFrame = false;
		tracker = new ofxFaceTracker();
		tracker->setup();

        while(isThreadRunning())
        {
			if(newFrame)
            {
				if(lock())
                {
					newFrame = false;
					if (resetTracking)
					{
						tracker->reset();
						resetTracking = false;
					}
					else
					{
						tracker->update(buffer);
                        ofSleepMillis(30); // give the tracker a moment
					}

					unlock();
				}
			}

			ofSleepMillis(1);
		}
	}
	
	ofxFaceTracker* tracker;
	cv::Mat buffer;
	volatile bool newFrame;
	volatile bool resetTracking;
};

//à changer pour le threaded


class ofxTrackerScopedLock
{
public:
	ofxTrackerScopedLock(ofxFaceTrackerThreaded& object)
		: tracker(object)
	{
		tracker.lock();
	}
	
	~ofxTrackerScopedLock()
	{
		tracker.unlock();
	}
	
private:
	ofxFaceTrackerThreaded& tracker;
};

//class ofxTrackerScopedLock
//{
//public:
//	ofxTrackerScopedLock(ofxFaceTrackerThreaded& object)
//	{
//	}
//	
//	~ofxTrackerScopedLock()
//	{
//	}
//};
