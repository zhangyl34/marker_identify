#pragma once
#include <opencv2/opencv.hpp>
#include <math.h>

struct PatternContainer {
	PatternContainer()
		: p1(-1,-1),
		p2(-1, -1),
		p3(-1, -1),
		p4(-1, -1),
		p5(-1, -1),
		p6(-1, -1),
		p7(-1, -1),
		p8(-1, -1)
	{
	}

	int getId() {
		int id = -1;
		if (p1.x < 0 || p2.x < 0 || p3.x < 0 || p4.x < 0 ||
			p5.x < 0 || p6.x < 0 || p7.x < 0 || p8.x < 0) {
			std::cout << "PatternContainer is not initialized..." << std::endl;
			return -1;
		}

		cv::Point2f temp[4];
		// 6
		int pos6;
		temp[0] = 2.0 * p4 - p1;
		temp[1] = temp[0] + (p2 - p1) * 0.5;
		temp[2] = 1.5 * p4 - 0.5 * p1;
		temp[3] = temp[2] + (p2 - p1) * 0.5;
		float distance = norm(p2 - p1);
		for (int i = 0; i < 4; i++) {
			if (norm(p6 - temp[i]) < distance) {
				distance = norm(p6 - temp[i]);
				pos6 = i;
			}
		}

		// 7
		int pos7;
		temp[0] = 2.0 * p5 - p2;
		temp[1] = temp[0] + (p3 - p2) * 0.5;
		temp[2] = 1.5 * p5 - 0.5 * p2;
		temp[3] = temp[2] + (p3 - p2) * 0.5;
		distance = norm(p2 - p1);
		for (int i = 0; i < 4; i++) {
			if (norm(p7 - temp[i]) < distance) {
				distance = norm(p7 - temp[i]);
				pos7 = i;
			}
		}

		// 8
		int pos8;
		temp[0] = p3 + 2.0 * p4 - 2.0 * p1;
		temp[1] = p3 + 1.5 * p4 - 1.5 * p1;
		temp[2] = p3 + 1.0 * p4 - 1.0 * p1;
		temp[3] = p3 + 0.5 * p4 - 0.5 * p1;
		distance = norm(p2 - p1);
		for (int i = 0; i < 4; i++) {
			if (norm(p8 - temp[i]) < distance) {
				distance = norm(p8 - temp[i]);
				pos8 = i;
			}
		}
		id = pos6 * 100 + pos7 * 10 + pos8;
		return id;
	}

	cv::Point2f p1;
	cv::Point2f p2;
	cv::Point2f p3;
	cv::Point2f p4;
	cv::Point2f p5; 
	cv::Point2f p6;
	cv::Point2f p7;
	cv::Point2f p8;
};

void findSquares(const cv::Mat& image, std::vector<std::vector<cv::Point>>& squares);

bool find8Points(const cv::Mat& image, std::vector<std::vector<cv::Point2f>>& result, std::vector<float>& pR,
	std::vector<int>& minx, std::vector<int>& miny, std::vector<int>& maxx, std::vector<int>& maxy);

PatternContainer distinguish8Points(const std::vector<cv::Point2f>& pointsIn, const float pR);

std::vector<int> crossCheck(std::vector<PatternContainer>& leftPoints, std::vector<PatternContainer>& rightPoints);

std::vector<cv::Point3f> uv2xyz(const std::vector<cv::Point2f>& lPts, const std::vector<cv::Point2f>& rPts,
	const cv::Mat& cameraMatrixl, const cv::Mat& distCoeffsl,
	const cv::Mat& cameraMatrixr, const cv::Mat& distCoeffsr, const cv::Mat& T2);

bool markerIdentify(const cv::Mat& leftSrc, const cv::Mat& rightSrc, std::vector<int>& markerId, std::vector<cv::Mat>& Tcam_marker);
