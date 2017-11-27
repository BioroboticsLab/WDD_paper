#pragma once
#include "cvblob.h"

namespace wdd
{
	class WaggleDanceOrientator
	{	
	public:
		WaggleDanceOrientator(void);
		~WaggleDanceOrientator(void);

		static cv::Point2d extractOrientationFromImageSequence(const std::vector<cv::Mat> seq_in, std::size_t unique_id);
		static cv::Point2d extractOrientationFromPositions(std::vector<cv::Point2d> positions, cv::Point2d position_last);
		static cv::Point2d extractNaiveness(std::vector<cv::Point2d> positions, cv::Point2d position_last);
		static void extractBinaryImageFromTD(cv::Mat *td_ptr, cv::Mat *td_bin_ptr);
		static void extractUnityOrientationsFromBinaryImage(cv::Mat * td_bin_mat_ptr, std::vector<cv::Vec2d> * unityOrientations_ptr);
		static cv::Vec2d getMeanOrientationFromUnityOrientations(std::vector<cv::Vec2d> * unityOrientations_ptr);
		static void getMeanUnityOrientation(cv::Vec2d *class_mean_ptr, std::vector<cv::Vec2d *> *class_ptr_list_ptr);
		static void saveDetectedOrientationImages(const std::vector<cv::Mat> *seq_in_ptr, const cv::Point2d *orient_ptr);
		static void saveDetectedBlobImage(cvb::CvBlob * blob_ptr, cv::Mat * labels_ptr, std::vector<double> * majMinAxisLenghts_ptr);
		static void writeBlobsDetailLine(double angle, std::vector<double> * majMinAxisLenghts_ptr);
		static void stretch(cv::Mat * in_ptr, cv::Mat * out_ptr);
		static void showImagesFromFolder(const std::string dirInNameFormat);
		static void showImage(const cv::Mat * img_ptr);
		static void WaggleDanceOrientator::saveImage(const cv::Mat *img_ptr, const char * path_ptr);
		static std::vector<cv::Mat> loadImagesFromFolder(const std::string dirInNameFormat);
		static void exportImage(const cv::Mat *img_ptr, const std::string path_ptr);
		static void statisticalSmoothingFilter(cv::Mat *img_ptr);

		static bool WDO_VERBOSE;
		static int picID;
		static int blobID;
		static double WDO_IMAGE_RED_FAC;
		static double WDO_IMAGE_RED_SCALE;

		static cv::Mat gaussKernel;

		static char * path_out;
		static char * path_out_root;
		static char * blobDirName;
		static char * file_blobs_detail;
		static char   path_out_root_img[];
		static char   path_out_root_img_blob[];
	};
} /* namespace WaggleDanceDetector */
