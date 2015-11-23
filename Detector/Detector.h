/*
 *
 *
 *
 *
 *
*/

#ifndef DETECTOR_H
#define DETECTOR_H

#include <iostream>
#include <vector>
#include <string>
#include <opencv2/opencv.hpp>
#include "cv.h"
#include "highgui.h"
#include "cxcore.h"

const int nOctaveLayers = 2;
const double sigma = 0.1;

using namespace std;
using namespace cv;

class Detector
{
public:
	Detector(Mat &I) : image(I) {};
	~Detector() { };
	void Lapls(const Mat &, vector<Point2d> &);
	void Harris(const Mat &, vector<Point2d> &);
	void OrientativeFliter(const Mat &, vector<Point2d> &);
	Mat &ReadImage(const string &);
	void Display(const Mat &, vector<Point2d> &);
	void WriteImage(const Mat &, string &);

private:
	vector<int> points;
	Mat image;
};

void Detector::Lapls(const Mat &image, vector<Point2d> &points)
{
	Mat laplsCal = Mat::zeros(3, 3, CV_64F);

}


// ����nOctaves�飨ÿ��nOctaves+3�㣩��˹������  
void buildGaussianPyramid(const Mat& base, vector<Mat>& pyr, int nOctaves)
{
	vector<double> sig(nOctaveLayers + 3);
	pyr.resize(nOctaves*(nOctaveLayers + 3));

	// precompute Gaussian sigmas using the following formula:  
	//  \sigma_{total}^2 = \sigma_{i}^2 + \sigma_{i-1}^2��  
	// �����ͼ������ͬ�߶ȸ�˹ģ���ĳ߶�����  
	sig[0] = sigma;
	double k = pow(2., 1. / nOctaveLayers);
	for (int i = 1; i < nOctaveLayers + 3; i++)
	{
		double sig_prev = pow(k, (double)(i - 1))*sigma;
		double sig_total = sig_prev*k;
		sig[i] = std::sqrt(sig_total*sig_total - sig_prev*sig_prev);
	}

	for (int o = 0; o < nOctaves; o++)
	{
		// DoG��������ҪnOctaveLayers+2��ͼ�������nOctaves��߶�  
		// ���Ը�˹��������ҪnOctaveLayers+3��ͼ��õ�nOctaveLayers+2��DoG������  
		for (int i = 0; i < nOctaveLayers + 3; i++)
		{
			// dstΪ��o�飨Octave��������  
			Mat& dst = pyr[o*(nOctaveLayers + 3) + i];
			// ��0���0��Ϊԭʼͼ��  
			if (o == 0 && i == 0)
				dst = base;

			// base of new octave is halved image from end of previous octave  
			// ÿһ���0��ͼ��ʱ��һ�鵹��������ͼ���������õ�  
			else if (i == 0)
			{
				const Mat& src = pyr[(o - 1)*(nOctaveLayers + 3) + nOctaveLayers];
				resize(src, dst, Size(src.cols / 2, src.rows / 2),
					0, 0, INTER_NEAREST);
			}
			// ÿһ���i��ͼ�����ɵ�i-1��ͼ�����sig[i]�ĸ�˹ģ���õ�  
			// Ҳ���Ǳ���ͼ����sig[i]�ĳ߶ȿռ��µ�ͼ��  
			else
			{
				const Mat& src = pyr[o*(nOctaveLayers + 3) + i - 1];
				GaussianBlur(src, dst, Size(), sig[i], sig[i]);
			}
		}
	}
}


#endif