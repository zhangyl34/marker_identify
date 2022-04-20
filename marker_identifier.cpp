#include "marker_identifier.h"

bool find8Points(cv::Mat& image, std::vector<cv::Point2f>& result, float& pR) {
	
	// parameters:
	const float areaPortion = 2;

	cv::GaussianBlur(image, image, cv::Size(5, 5), 0);

	cv::Mat imgBin(image.rows, image.cols, CV_8UC1);
	cv::threshold(image, imgBin, 180, 255, cv::THRESH_BINARY);

	cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
	cv::morphologyEx(imgBin, imgBin, cv::MORPH_OPEN, element);  // eliminate while noise in black region.
	cv::morphologyEx(imgBin, imgBin, cv::MORPH_CLOSE, element);  // eliminate black noise in white region.

	cv::Mat imgEdge(image.rows, image.cols, CV_8UC1);
	cv::Canny(imgBin, imgEdge, 50.0, 50.0 * 2);

	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(imgEdge, contours, cv::RETR_LIST, cv::CHAIN_APPROX_NONE);
	imgEdge = cv::Scalar::all(0);

	// overlap contour erasing.
	// if two contours' area is similar, and distance is small, then erase one of them.
	std::vector<std::vector<cv::Point>> contoursUpdated0;
	for (int i = 0; i < contours.size(); i++) {
		cv::RotatedRect boxi = cv::fitEllipse(contours[i]);
		double areai = cv::contourArea(contours[i]);
		bool unique = 1;
		for (const auto& cUj : contoursUpdated0) {
			cv::RotatedRect boxj = cv::fitEllipse(cUj);
			if ((pow(boxi.center.x - boxj.center.x, 2) + pow(boxi.center.y - boxj.center.y, 2)) * CV_PI < areai &&
				abs(cv::contourArea(cUj) - areai) < areai * 0.1) {
				unique = 0;
			}
		}
		if (unique == 1) {
			contoursUpdated0.emplace_back(contours[i]);
		}
	}

	// contour area voting. too big or too small area is erased.
	int candidate = -1;
	for (int i = 0; i < contoursUpdated0.size(); i++) {
		double area_i = cv::contourArea(contoursUpdated0[i]);
		int voteNum = 0;

		for (int j = 0; j < contoursUpdated0.size(); j++) {
			double area_j = cv::contourArea(contoursUpdated0[j]);
			if (area_j / area_i < areaPortion && area_i / area_j < areaPortion) {
				voteNum++;
			}
		}
		
		if (voteNum >= 8) {
			candidate = i;
			break;
		}
	}
	if (candidate == -1) {
		return false;
	}

	// update contours1.
	std::vector<std::vector<cv::Point>> contoursUpdated1;
	float areaCan = cv::contourArea(contoursUpdated0[candidate]);
	for (int i = 0; i < contoursUpdated0.size(); i++) {
		float area_i = cv::contourArea(contoursUpdated0[i]);
		if (areaCan / area_i < areaPortion && area_i / areaCan < areaPortion) {
			contoursUpdated1.emplace_back(contoursUpdated0.at(i));
		}
	}

	// distance voting. too far away points are erased.
	candidate = -1;
	//float ppDistance = sqrt(areaCan / CV_PI) * 8;  // r*8
	const float ppRadius = sqrt(areaCan / CV_PI) * 10;  // radius
	for (int i = 0; i < contoursUpdated1.size(); i++) {
		cv::RotatedRect boxi = cv::fitEllipse(contoursUpdated1[i]);

		int supporters = 0;
		for (const auto& cj : contoursUpdated1) {
			cv::RotatedRect boxj = cv::fitEllipse(cj);
			if ((pow(boxi.center.x - boxj.center.x, 2) + pow(boxi.center.y - boxj.center.y, 2)) < ppRadius*ppRadius) {
				supporters++;
			}
		}
		if (supporters == 8) {
			candidate = i;
			break;
		}
	}
	if (candidate == -1) {
		return false;
	}

	// update contours2.
	std::vector<std::vector<cv::Point>> contoursUpdated2;
	cv::RotatedRect boxCan = cv::fitEllipse(contoursUpdated1[candidate]);
	for (int i = 0; i < contoursUpdated1.size(); i++) {
		cv::RotatedRect box_i = cv::fitEllipse(contoursUpdated1[candidate]);
		if ((pow(box_i.center.x - boxCan.center.x, 2) + pow(box_i.center.y - boxCan.center.y, 2)) < ppRadius * ppRadius) {
			contoursUpdated2.emplace_back(contoursUpdated1.at(i));
		}
	}

	//std::cout << contoursUpdated2.size() << std::endl;
	pR = sqrt(areaCan / CV_PI);
	// draw the results
	for (int i = 0; i < contoursUpdated2.size(); i++)
	{
		// at least 6 points.
		if (contoursUpdated2[i].size() < 6)
			return false;

		// fit ellipse
		//double area_i = cv::contourArea(contoursUpdated[i]);
		cv::RotatedRect box = cv::fitEllipse(contoursUpdated2[i]);
		//std::cout << box.center.x << " " << box.center.y << std::endl;
		//if (std::max<float>({ box.size.width, box.size.height }) > std::min<float>({ box.size.width, box.size.height }) * 10 ||
		//	box.size.width * box.size.height * CV_PI / 4)
		//	continue;

		// draw the ellipse
		cv::drawContours(imgEdge, contoursUpdated2, i, cv::Scalar::all(100));
		cv::ellipse(imgEdge, box, cv::Scalar::all(255));

		result.emplace_back(cv::Point2f(box.center.x, box.center.y));
	}

	cv::imshow("left Gaussian", image);
	cv::imshow("left Binary", imgBin);
	cv::imshow("left Edge", imgEdge);

	cv::waitKey(1);

	return true;
}

PatternContainer distinguish8Points(const std::vector<cv::Point2f>& pointsIn, const float pR) {
	PatternContainer pointsOut;

	if (pointsIn.size() != 8) {
		std::cout << "distinguish8Points: input size wrong!" << std::endl;
		return pointsOut;
	}

	// find X coordinate.
	bool findX = 0;
	for (const auto& a : pointsIn) {
		for (const auto& b : pointsIn) {
			if (b == a) {
				continue;
			}
			for (const auto& c : pointsIn) {
				if (c == a || c == b) {
					continue;
				}

				// criteria1: {Vec_ab}=={Vec_bc}
				if ( fabs(a.y + c.y - 2*b.y)>pR || fabs(a.x + c.x - 2*b.x)>pR ) {
					continue;
				}

				// criteria2: the remaining points are to the left of the {Vec_ac}
				// criteria3: max a angle theta1 == min c angle theta2
				bool cri2 = 1;
				float theta_a = 0, theta_c = 4;
				for (const auto& d : pointsIn) {
					if (d == a || d == b || d == c) {
						continue;
					}
					if (((c.x - a.x) * (d.y - a.y) - (c.y - a.y) * (d.x - a.x)) >= 0) {  // on the right side
						cri2 = 0;
						break;
					}

					float temp_a, temp_c;
					temp_a = acos( ((c.x - a.x) * (d.x - a.x) + (c.y - a.y) * (d.y - a.y)) /
						sqrt((c.x - a.x) * (c.x - a.x) + (c.y - a.y) * (c.y - a.y)) /
						sqrt((d.x - a.x) * (d.x - a.x) + (d.y - a.y) * (d.y - a.y)) );
					temp_c = acos(((c.x - a.x) * (d.x - c.x) + (c.y - a.y) * (d.y - c.y)) /
						sqrt((c.x - a.x) * (c.x - a.x) + (c.y - a.y) * (c.y - a.y)) /
						sqrt((d.x - c.x) * (d.x - c.x) + (d.y - c.y) * (d.y - c.y)));
					if (temp_a > theta_a) {  // max a
						theta_a = temp_a;
					}
					if (temp_c < theta_c) {  // min c
						theta_c = temp_c;
					}
				}
				if (!cri2) {
					continue;
				}
				if (fabs(theta_a-theta_c) > 5.0 / 180.0 * CV_PI) {
					continue;
				}
				float theta = (theta_a + theta_c) / 2;

				// criteria4: only one point can be found, at c's theta direction.
				int cCount = 0;
				cv::Point2f p8;
				for (const auto& d : pointsIn) {
					if (d == a || d == b || d == c) {
						continue;
					}
					float temp_c = acos(((c.x - a.x) * (d.x - c.x) + (c.y - a.y) * (d.y - c.y)) /
						sqrt((c.x - a.x) * (c.x - a.x) + (c.y - a.y) * (c.y - a.y)) /
						sqrt((d.x - c.x) * (d.x - c.x) + (d.y - c.y) * (d.y - c.y)));
					if (fabs(theta - temp_c) < (5.0 / 180.0 * CV_PI)) {
						cCount++;
						p8 = d;
					}
				}
				if (cCount != 1) {
					continue;
				}

				// criteria 5: distance between a and the first points at a's theta direction == 
				// distance between b and the first points at b's theta direction.
				float dis_a = pR * 100, dis_b = pR * 200;
				cv::Point2f p4, p5;
				for (const auto& d : pointsIn) {
					if (d == a || d == b || d == c) {
						continue;
					}
					float temp_a = acos(((c.x - a.x) * (d.x - a.x) + (c.y - a.y) * (d.y - a.y)) /
						sqrt((c.x - a.x) * (c.x - a.x) + (c.y - a.y) * (c.y - a.y)) /
						sqrt((d.x - a.x) * (d.x - a.x) + (d.y - a.y) * (d.y - a.y)));
					float temp_b = acos(((c.x - a.x) * (d.x - b.x) + (c.y - a.y) * (d.y - b.y)) /
						sqrt((c.x - a.x) * (c.x - a.x) + (c.y - a.y) * (c.y - a.y)) /
						sqrt((d.x - b.x) * (d.x - b.x) + (d.y - b.y) * (d.y - b.y)));
					if (fabs(theta - temp_a) < (5.0 / 180.0 * CV_PI)) {
						float temp_dis = sqrt((d.x - a.x) * (d.x - a.x) + (d.y - a.y) * (d.y - a.y));
						if (temp_dis < dis_a) {
							dis_a = temp_dis;
							p4 = d;
						}
					}
					if (fabs(theta - temp_b) < (5.0 / 180.0 * CV_PI)) {
						float temp_dis = sqrt((d.x - b.x) * (d.x - b.x) + (d.y - b.y) * (d.y - b.y));
						if (temp_dis < dis_b) {
							dis_b = temp_dis;
							p5 = d;
						}
					}
				}
				if (fabs(dis_a - dis_b) > pR) {
					continue;
				}

				// all criterias are met
				findX = 1;
				pointsOut.p1 = a;
				pointsOut.p2 = b;
				pointsOut.p3 = c;
				pointsOut.p4 = p4;
				pointsOut.p5 = p5;
				pointsOut.p8 = p8;
				std::vector<cv::Point2f> points2;
				for (const auto& d : pointsIn) {
					if (d == a || d == b || d == c ||
						norm(d - p4) < pR || norm(d - p5) < pR || norm(d - p8) < pR) {
						continue;
					}
					points2.emplace_back(d);  // only p6 and p7 are emplaced back.
				}
				float temp0 = acos(((c.x - a.x) * (points2[0].x - b.x) + (c.y - a.y) * (points2[0].y - b.y)) /
					sqrt((c.x - a.x) * (c.x - a.x) + (c.y - a.y) * (c.y - a.y)) /
					sqrt((points2[0].x - b.x) * (points2[0].x - b.x) + (points2[0].y - b.y) * (points2[0].y - b.y)));
				float temp1 = acos(((c.x - a.x) * (points2[1].x - b.x) + (c.y - a.y) * (points2[1].y - b.y)) /
					sqrt((c.x - a.x) * (c.x - a.x) + (c.y - a.y) * (c.y - a.y)) /
					sqrt((points2[1].x - b.x) * (points2[1].x - b.x) + (points2[1].y - b.y) * (points2[1].y - b.y)));
				if (temp0 < temp1) {
					pointsOut.p6 = points2[1];
					pointsOut.p7 = points2[0];
				}
				else {
					pointsOut.p6 = points2[0];
					pointsOut.p7 = points2[1];
				}
				return pointsOut;
			}
		}
	}

	// didn't find X coordinate
	std::cout << "distinguish8Points: can't find X coordinate..." << std::endl;
	return pointsOut;
}


