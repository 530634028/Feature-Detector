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

void SIFT::buildDoGPyramid(const vector<Mat>& gpyr, vector<Mat>& dogpyr) const
{
	int nOctaves = (int)gpyr.size() / (nOctaveLayers + 3);
	dogpyr.resize(nOctaves*(nOctaveLayers + 2));

	for (int o = 0; o < nOctaves; o++)
	{
		for (int i = 0; i < nOctaveLayers + 2; i++)
		{
			// ��o���i��ͼ��Ϊ��˹�������е�o���i+1��i��ͼ������õ�  
			const Mat& src1 = gpyr[o*(nOctaveLayers + 3) + i];
			const Mat& src2 = gpyr[o*(nOctaveLayers + 3) + i + 1];
			Mat& dst = dogpyr[o*(nOctaveLayers + 2) + i];
			subtract(src2, src1, dst, noArray(), CV_16S);
		}
	}
}

// Detects features at extrema in DoG scale space.  Bad features are discarded  
// based on contrast and ratio of principal curvatures.  
// ��DoG�߶ȿռ�Ѱ�����㣨��ֵ�㣩  
void SIFT::findScaleSpaceExtrema(const vector<Mat>& gauss_pyr, const vector<Mat>& dog_pyr,
	vector<KeyPoint>& keypoints) const
{
	int nOctaves = (int)gauss_pyr.size() / (nOctaveLayers + 3);

	// The contrast threshold used to filter out weak features in semi-uniform  
	// (low-contrast) regions. The larger the threshold, the less features are produced by the detector.  
	// ���˵�����������ֵ contrastThresholdĬ��Ϊ0.04  
	int threshold = cvFloor(0.5 * contrastThreshold / nOctaveLayers * 255 * SIFT_FIXPT_SCALE);
	const int n = SIFT_ORI_HIST_BINS; //36  
	float hist[n];
	KeyPoint kpt;

	keypoints.clear();

	for (int o = 0; o < nOctaves; o++)
		for (int i = 1; i <= nOctaveLayers; i++)
		{
			int idx = o*(nOctaveLayers + 2) + i;
			const Mat& img = dog_pyr[idx];
			const Mat& prev = dog_pyr[idx - 1];
			const Mat& next = dog_pyr[idx + 1];
			int step = (int)img.step1();
			int rows = img.rows, cols = img.cols;

			for (int r = SIFT_IMG_BORDER; r < rows - SIFT_IMG_BORDER; r++)
			{
				const short* currptr = img.ptr<short>(r);
				const short* prevptr = prev.ptr<short>(r);
				const short* nextptr = next.ptr<short>(r);

				for (int c = SIFT_IMG_BORDER; c < cols - SIFT_IMG_BORDER; c++)
				{
					int val = currptr[c];

					// find local extrema with pixel accuracy  
					// Ѱ�Ҿֲ���ֵ�㣬DoG��ÿ�����������ڵ���������Χ��26����Ƚ�  
					// if ��val�����ж��� ���� val�����ж�С��  
					if (std::abs(val) > threshold &&
						((val > 0 && val >= currptr[c - 1] && val >= currptr[c + 1] &&
						val >= currptr[c - step - 1] && val >= currptr[c - step] &&
						val >= currptr[c - step + 1] && val >= currptr[c + step - 1] &&
						val >= currptr[c + step] && val >= currptr[c + step + 1] &&
						val >= nextptr[c] && val >= nextptr[c - 1] &&
						val >= nextptr[c + 1] && val >= nextptr[c - step - 1] &&
						val >= nextptr[c - step] && val >= nextptr[c - step + 1] &&
						val >= nextptr[c + step - 1] && val >= nextptr[c + step] &&
						val >= nextptr[c + step + 1] && val >= prevptr[c] &&
						val >= prevptr[c - 1] && val >= prevptr[c + 1] &&
						val >= prevptr[c - step - 1] && val >= prevptr[c - step] &&
						val >= prevptr[c - step + 1] && val >= prevptr[c + step - 1] &&
						val >= prevptr[c + step] && val >= prevptr[c + step + 1]) ||
						(val < 0 && val <= currptr[c - 1] && val <= currptr[c + 1] &&
						val <= currptr[c - step - 1] && val <= currptr[c - step] &&
						val <= currptr[c - step + 1] && val <= currptr[c + step - 1] &&
						val <= currptr[c + step] && val <= currptr[c + step + 1] &&
						val <= nextptr[c] && val <= nextptr[c - 1] &&
						val <= nextptr[c + 1] && val <= nextptr[c - step - 1] &&
						val <= nextptr[c - step] && val <= nextptr[c - step + 1] &&
						val <= nextptr[c + step - 1] && val <= nextptr[c + step] &&
						val <= nextptr[c + step + 1] && val <= prevptr[c] &&
						val <= prevptr[c - 1] && val <= prevptr[c + 1] &&
						val <= prevptr[c - step - 1] && val <= prevptr[c - step] &&
						val <= prevptr[c - step + 1] && val <= prevptr[c + step - 1] &&
						val <= prevptr[c + step] && val <= prevptr[c + step + 1])))
					{
						int r1 = r, c1 = c, layer = i;

						// �ؼ��㾫ȷ��λ  
						if (!adjustLocalExtrema(dog_pyr, kpt, o, layer, r1, c1,
							nOctaveLayers, (float)contrastThreshold,
							(float)edgeThreshold, (float)sigma))
							continue;

						float scl_octv = kpt.size*0.5f / (1 << o);
						// �����ݶ�ֱ��ͼ  
						float omax = calcOrientationHist(
							gauss_pyr[o*(nOctaveLayers + 3) + layer],
							Point(c1, r1),
							cvRound(SIFT_ORI_RADIUS * scl_octv),
							SIFT_ORI_SIG_FCTR * scl_octv,
							hist, n);
						float mag_thr = (float)(omax * SIFT_ORI_PEAK_RATIO);
						for (int j = 0; j < n; j++)
						{
							int l = j > 0 ? j - 1 : n - 1;
							int r2 = j < n - 1 ? j + 1 : 0;

							if (hist[j] > hist[l] && hist[j] > hist[r2] && hist[j] >= mag_thr)
							{
								float bin = j + 0.5f * (hist[l] - hist[r2]) /
									(hist[l] - 2 * hist[j] + hist[r2]);
								bin = bin < 0 ? n + bin : bin >= n ? bin - n : bin;
								kpt.angle = (float)((360.f / n) * bin);
								keypoints.push_back(kpt);
							}
						}
					}
				}
			}
		}
}



#endif