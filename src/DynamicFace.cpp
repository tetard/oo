//
//  DynamicFace.cpp
//  Ö
//
//  Created by Camille Troillard on 20/11/12.
//
//

#include "DynamicFace.h"
#include "ofxFaceTracker.h"

DynamicFace::DynamicFace()
: kind(DynamicFace::INVALID)
, syphonClient(0)
{
}

DynamicFace::DynamicFace(int width, int height, string syphonClientName)
: kind(DynamicFace::SYPHON)
, syphonClient(new ofxSyphonClient())
{
	syphonClient->setup();
	syphonClient->setServerName(syphonClientName);
	loadPoints("faces/" + syphonClientName + ".points");

	ofFbo::Settings settings;
	settings.width = width;
	settings.height = height;

	fboSyphon.allocate(settings);
}

DynamicFace::~DynamicFace()
{
	if (syphonClient)
		delete syphonClient;
}

void DynamicFace::load(string filename)
{
	ofLog(OF_LOG_VERBOSE) << "Loading face: " << filename;

	if (ofFilePath::getFileExt(filename) == "mov")
	{
		kind = DynamicFace::VIDEO;
		video.loadMovie(filename);
	}
	else
	{
		kind = image.loadImage(filename) ?
        DynamicFace::IMAGE :
        DynamicFace::INVALID;
	}

	string pointsFilename = ofFilePath::removeExt(filename) + ".points";
	ofFile pointsFile(pointsFilename);

	if (pointsFile.exists())
	{
		loadPoints(pointsFilename);
	}
	else
	{
		if (kind == DynamicFace::IMAGE)
		{
			ofxFaceTracker tracker;
			tracker.setup();
			tracker.setIterations(25);
			tracker.setAttempts(4);
			tracker.reset();
			tracker.update(ofxCv::toCv(image));

			points = tracker.getImagePoints();
			savePoints(pointsFilename);
		}
	}
}

void DynamicFace::savePoints(string filename) const
{
	cv::FileStorage fs(ofToDataPath(filename), cv::FileStorage::WRITE);

	fs << "points" << "[";
	{
		for(int i = 0; i < points.size(); i++)
		{
			fs << points[i].x;
			fs << points[i].y;
		}
	}
	fs << "]";
}

void DynamicFace::loadPoints(string filename)
{
	ofLog(OF_LOG_VERBOSE) << "   Loading points: " << filename;

	string fullPath = ofToDataPath(filename);
	ofFile file(fullPath);

	if (file.exists())
	{
		cv::FileStorage fs(fullPath, cv::FileStorage::READ);
		cv::FileNode pointsNode = fs["points"];

		int n = pointsNode.size() / 2;
		PointVector thePoints(n);

		for(int i = 0; i < n; i++)
		{
			float x;
			float y;
			pointsNode[i*2]   >> x;
			pointsNode[i*2+1] >> y;
			thePoints[i] = ofVec2f(x, y);
		}

		points = thePoints;
	}
	else
		throw runtime_error("Points file not found: " + fullPath);
}

PointVector const& DynamicFace::getPoints() const
{
	return points;
}

void DynamicFace::update()
{
	switch (kind)
	{
		case DynamicFace::VIDEO:
			video.update();
			break;

		case DynamicFace::SYPHON:
			fboSyphon.begin();
        {
            ofPushMatrix();
            ofSetupScreenOrtho(fboSyphon.getWidth(), fboSyphon.getHeight(), OF_ORIENTATION_180, true);
            syphonClient->draw(0, 0, fboSyphon.getWidth(), fboSyphon.getHeight());
            ofPopMatrix();
        }
			fboSyphon.end();
			break;

		default:
			break;
	}
}

void DynamicFace::start()
{
	if (kind == DynamicFace::VIDEO) {
        video.setPosition(0);
        video.setSpeed(1.0);
		video.play();
    }
}

void DynamicFace::stop()
{
	if (kind == DynamicFace::VIDEO) {
		video.stop();
    }
}

void DynamicFace::setVideoVolume(float volume)
{
	if (kind == DynamicFace::VIDEO) {
		video.setVolume(volume * 100.0);
    }
}

void DynamicFace::setVideoSpeed(float speed)
{
	if (kind == DynamicFace::VIDEO) {
		video.setSpeed(speed);
    }
}

void DynamicFace::bind()
{
	switch (kind)
	{
		case DynamicFace::IMAGE:
			image.bind();
			break;

		case DynamicFace::VIDEO:
			video.getTextureReference().bind();
			break;

		case DynamicFace::SYPHON:
			fboSyphon.getTextureReference().bind();
			break;

		default:
			break;
	}
}

void DynamicFace::unbind()
{
	switch (kind)
	{
		case DynamicFace::IMAGE:
			image.unbind();
			break;

		case DynamicFace::VIDEO:
			video.getTextureReference().unbind();
			break;

		case DynamicFace::SYPHON:
			fboSyphon.getTextureReference().unbind();
			break;
			
		default:
			break;
	}
}
