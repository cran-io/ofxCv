#include "testApp.h"

void testApp::setup() {
	ofSetVerticalSync(true);
	
	movie.loadMovie("video.mov");
	movie.play();
	
	contourFinder.setMinAreaRadius(1);
	contourFinder.setMaxAreaRadius(100);
	contourFinder.setThreshold(15);
}

void testApp::update() {
	movie.update();
	if(movie.isFrameNew()) {
		blur(movie, 10);
		contourFinder.findContours(movie);
		
		// this should be happening inside the contour finder
		vector<cv::Rect> boundingBoxes;
		for(int i = 0; i < contourFinder.size(); i++) {
			boundingBoxes.push_back(contourFinder.getBoundingRect(i));
		}
		rectTracker.track(boundingBoxes);
	}
}

void testApp::draw() {
	ofSetColor(255);
	movie.draw(0, 0);
	contourFinder.draw();
	for(int i = 0; i < contourFinder.size(); i++) {
		ofPoint center = toOf(contourFinder.getCenter(i));
		ofDrawBitmapString(ofToString(rectTracker.getLabels()[i]), center);
	}
}