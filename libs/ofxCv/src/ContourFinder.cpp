#include "ofxCv/ContourFinder.h"
#include "ofxCv/Wrappers.h"

namespace ofxCv {

	struct CompareContourArea
	{
		CompareContourArea(const std::vector<double>& areaVec)
		: mAreaVec(areaVec) {}

		// Sort contour indices into decreasing order, based on a vector of
		// contour areas.  Later, we will use these indices to order the
		// contours (which are stored in a separate vector).
		bool operator()(size_t a, size_t b) const
		{
			return mAreaVec[a] > mAreaVec[b];
		}

		const std::vector<double>& mAreaVec;
	};

	using namespace cv;
	
	ContourFinder::ContourFinder()
	:invert(false)
	,simplify(true)
	,contourFindingMode(CV_RETR_EXTERNAL)
	,sortBySize(false) {
		resetMinArea();
		resetMaxArea();
	}
	
	void ContourFinder::findContours(Mat img) {
        // cv::findContours modifies the original image, so we work on a copy        
        img.copyTo(srcCopy); // copyTo handles allocation (only if necessary)

		// run the contour finder
		vector<vector<cv::Point> > allContours;
		int simplifyMode = simplify ? CV_CHAIN_APPROX_SIMPLE : CV_CHAIN_APPROX_NONE;
		cv::findContours(srcCopy, allContours, contourFindingMode, simplifyMode);
		
		// filter the contours
		bool needMinFilter = (minArea > 0);
		bool needMaxFilter = maxAreaNorm ? (maxArea < 1) : (maxArea < numeric_limits<float>::infinity());
		vector<size_t> allIndices;
		vector<double> allAreas;
		if(needMinFilter || needMaxFilter) {
			double imgArea = srcCopy.rows * srcCopy.cols;
			double imgMinArea = minAreaNorm ? (minArea * imgArea) : minArea;
			double imgMaxArea = maxAreaNorm ? (maxArea * imgArea) : maxArea;
			for(size_t i = 0; i < allContours.size(); i++) {
				double curArea = contourArea(Mat(allContours[i]));
				allAreas.push_back(curArea);
				if((!needMinFilter || curArea >= imgMinArea) &&
					 (!needMaxFilter || curArea <= imgMaxArea)) {
					allIndices.push_back(i);
				}
			}
		} else {
			for(size_t i = 0; i < allContours.size(); i++) {
				if (sortBySize) {
					allAreas.push_back(contourArea(allContours[i]));
				}
				allIndices.push_back(i);
			}
		}

		if (allIndices.size() > 1 && sortBySize) {
			// Sort contour indices, based on a separate vector of areas.
			std::sort(allIndices.begin(), allIndices.end(), CompareContourArea(allAreas));
		}

		// generate polylines and bounding boxes from the contours
		contours.clear();
		polylines.clear();
		boundingRects.clear();
		
		blobs.clear();
		for(size_t i = 0; i < allIndices.size(); i++) {
			contours.push_back(allContours[allIndices[i]]);
			polylines.push_back(toOf(contours[i]));
			boundingRects.push_back(boundingRect(contours[i]));
			
			ofxCvBlob blob;
			
            blob.area = std::abs(toOf(contours[i]).getArea()); // Depending on whether opencv considers a region a hole or not, the area might be negative (which we don't care about)
            blob.boundingRect = toOf(boundingRect(contours[i]));

            Moments m = moments(contours[i]);
            blob.centroid.set(m.m10 / m.m00, m.m01 / m.m00);
            
            blob.hole = false;
            
            for (vector<cv::Point>::iterator p_it = allContours[allIndices[i]].begin(); p_it != allContours[allIndices[i]].end(); ++p_it) {
                ofVec3f new_pt = ofVec3f(p_it->x, p_it->y);
                blob.pts.push_back(new_pt);
            }
			blob.nPts = blob.pts.size();
			
			blob.length = arcLength(contours[i], true);
			
			blobs.push_back(blob);
		}
	}
	

	void ContourFinder::setFindHoles(bool findHoles){
		if(findHoles){
			contourFindingMode = CV_RETR_LIST;
		}else{
			contourFindingMode = CV_RETR_EXTERNAL;
		}
	}

	void ContourFinder::setSortBySize(bool sizeSort) {
		sortBySize = sizeSort;
	}

	const vector<vector<cv::Point> >& ContourFinder::getContours() const {
		return contours;
	}
	
	const vector<ofPolyline>& ContourFinder::getPolylines() const {
		return polylines;
	}
	
	const vector<cv::Rect>& ContourFinder::getBoundingRects() const {
		return boundingRects;
	}
	
	unsigned int ContourFinder::size() const {
		return contours.size();
	}
	
	vector<cv::Point>& ContourFinder::getContour(unsigned int i) {
		return contours[i];
	}
	
	ofPolyline& ContourFinder::getPolyline(unsigned int i) {
		return polylines[i];
	}
	
	cv::Rect ContourFinder::getBoundingRect(unsigned int i) const {
		return boundingRects[i];
	}
	
	cv::Point2f ContourFinder::getCenter(unsigned int i) const {
		cv::Rect box = getBoundingRect(i);
		return cv::Point2f(box.x + box.width / 2, box.y + box.height / 2);
	}
	
	cv::Point2f ContourFinder::getCentroid(unsigned int i) const {
		Moments m = moments(contours[i]);
		return cv::Point2f(m.m10 / m.m00, m.m01 / m.m00);
	}
	
	cv::Point2f ContourFinder::getAverage(unsigned int i) const {
		Scalar average = mean(contours[i]);
		return cv::Point2f(average[0], average[1]);
	}
	
	cv::Vec2f ContourFinder::getBalance(unsigned int i) const {
		return cv::Vec2f(getCentroid(i) - getCenter(i));
	}
	
	double ContourFinder::getContourArea(unsigned int i) const {
		return contourArea(contours[i]);
	}
	
	double ContourFinder::getArcLength(unsigned int i) const {
		return arcLength(contours[i], true);
	}
	
	vector<cv::Point> ContourFinder::getConvexHull(unsigned int i) const {
		vector<cv::Point> hull;
		convexHull(contours[i], hull);
		return hull;
	}
	
	vector<cv::Vec4i> ContourFinder::getConvexityDefects(unsigned int i) const {
		return convexityDefects(contours[i]);
	}
	
	cv::RotatedRect ContourFinder::getMinAreaRect(unsigned int i) const {
		return minAreaRect(contours[i]);
	}
	
	cv::Point2f ContourFinder::getMinEnclosingCircle(unsigned int i, float& radius) const {
		cv::Point2f center;
		minEnclosingCircle(contours[i], center, radius);
		return center;
	}
	
	cv::RotatedRect ContourFinder::getFitEllipse(unsigned int i) const {
		if(contours[i].size() < 5) {
			return getMinAreaRect(i);
		}
		return fitEllipse(contours[i]);
	}
	
	vector<cv::Point> ContourFinder::getFitQuad(unsigned int i) const {
		vector<cv::Point> convexHull = getConvexHull(i);		
		vector<cv::Point> quad = convexHull;
		
		static const unsigned int targetPoints = 4;
		static const unsigned int maxIterations = 16;
		static const double infinity = numeric_limits<double>::infinity();
		double minEpsilon = 0;
		double maxEpsilon = infinity;
		double curEpsilon = 16; // good initial guess
		
		// unbounded binary search to simplify the convex hull until it's 4 points
		if(quad.size() > 4) {
			for(int i = 0; i <(int) maxIterations; i++) {
				approxPolyDP(Mat(convexHull), quad, curEpsilon, true);
				if(quad.size() == targetPoints) {
					break;
				}
				if(quad.size() > targetPoints) {
					minEpsilon = curEpsilon;
					if(maxEpsilon == infinity) {
						curEpsilon = curEpsilon *  2;
					} else {
						curEpsilon = (maxEpsilon + minEpsilon) / 2;
					}
				}
				if(quad.size() < targetPoints) {
					maxEpsilon = curEpsilon;
					curEpsilon = (maxEpsilon + minEpsilon) / 2;
				}
			}
		}
		
		return quad;
	}
		
	void ContourFinder::setInvert(bool invert) {
		this->invert = invert;
	}
    
	void ContourFinder::setSimplify(bool simplify) {
		this->simplify = simplify;
	}
	
	void ContourFinder::draw() {
		ofPushStyle();
		ofNoFill();
		for(int i = 0; i < (int)polylines.size(); i++) {
			polylines[i].draw();
			ofRect(toOf(getBoundingRect(i)));
		}
		ofPopStyle();
	}
	
	void ContourFinder::resetMinArea() {
		setMinArea(0);
	}
	
	void ContourFinder::resetMaxArea() {
		setMaxArea(numeric_limits<float>::infinity());
	}
	
	void ContourFinder::setMinArea(float minArea) {
		this->minArea = minArea;
		minAreaNorm = false;
	}
	
	void ContourFinder::setMaxArea(float maxArea) {
		this->maxArea = maxArea;
		maxAreaNorm = false;
	}
	
	void ContourFinder::setMinAreaRadius(float minAreaRadius) {
		minArea = PI * minAreaRadius * minAreaRadius;
		minAreaNorm = false;
	}
	
	void ContourFinder::setMaxAreaRadius(float maxAreaRadius) {
		maxArea = PI * maxAreaRadius * maxAreaRadius;
		maxAreaNorm = false;
	}
	
	void ContourFinder::setMinAreaNorm(float minAreaNorm) {
		minArea = minAreaNorm;
		this->minAreaNorm = true;
	}
	
	void ContourFinder::setMaxAreaNorm(float maxAreaNorm) {
		maxArea = maxAreaNorm;
		this->maxAreaNorm = true;
	}
	
}
