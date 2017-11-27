#include "stdafx.h"
#include "WaggleDanceOrientator.h"

#include "cvblob.h"

namespace wdd
{

	int WaggleDanceOrientator::picID = 0;
	int WaggleDanceOrientator::blobID = 0;
	double WaggleDanceOrientator::WDO_IMAGE_RED_FAC = 1;
	// WDO_IMAGE_RED_SCALE = 1/WDO_IMAGE_RED_FAC
	double WaggleDanceOrientator::WDO_IMAGE_RED_SCALE = 1/WDO_IMAGE_RED_FAC;

	bool WaggleDanceOrientator::WDO_VERBOSE = false;

	// create a 5x5 double-precision kernel matrix adopted from MATLAB
	cv::Mat WaggleDanceOrientator::gaussKernel = 
		(cv::Mat_<double>(5,5) <<	0.0002, 0.0033, 0.0081, 0.0033, 0.0002,
		0.0033, 0.0479, 0.1164, 0.0479, 0.0033,
		0.0081, 0.1164, 0.2831, 0.1164, 0.0081,
		0.0033, 0.0479, 0.1164, 0.0479, 0.0033,
		0.0002, 0.0033, 0.0081, 0.0033, 0.0002);

	char * WaggleDanceOrientator::path_out = "\\verbose";
	char * WaggleDanceOrientator::path_out_root = "\\verbose\\wdo";
	char * WaggleDanceOrientator::blobDirName = "\\blobs";
	char * WaggleDanceOrientator::file_blobs_detail = "\\blobs_detail.txt";
	char WaggleDanceOrientator::path_out_root_img[MAX_PATH];
	char WaggleDanceOrientator::path_out_root_img_blob[MAX_PATH];

	/*
	* static functions
	*/
	cv::Point2d WaggleDanceOrientator::extractOrientationFromImageSequence(std::vector<cv::Mat> seq_in, std::size_t unique_id)
	{
		WaggleDanceOrientator::picID = 0;
		WaggleDanceOrientator::blobID = 0;

		// TODO externalize single init of folder structure
		if(WDO_VERBOSE)
		{
			// link to help functionin main.cpp
			extern bool dirExists(const char * dirPath);
			// link full path from main.cpp
			extern char _FULL_PATH_EXE[MAX_PATH];

			char BUFF_PATH[MAX_PATH];
			char BUFF_UID[MAX_PATH];

			// create path to .\verbose
			strcpy_s(BUFF_PATH ,MAX_PATH, _FULL_PATH_EXE);
			strcat_s(BUFF_PATH, MAX_PATH, path_out);

			// check for path_out folder
			if(!dirExists(BUFF_PATH))
			{
				if(!CreateDirectory(BUFF_PATH, NULL))
					printf("ERROR! Couldn't create %s directory.\n", BUFF_PATH);
			}

			// create path to .\verbose\wdo
			strcpy_s(BUFF_PATH ,MAX_PATH, _FULL_PATH_EXE);
			strcat_s(BUFF_PATH, MAX_PATH, path_out_root);

			// check for path_out folder
			if(!dirExists(BUFF_PATH))
			{
				if(!CreateDirectory(BUFF_PATH, NULL))
					printf("\nCouldn't create %s directory.\n", BUFF_PATH);
			}

			// create dynamic path_out string
			strcat_s(BUFF_PATH, MAX_PATH, "\\");

			// convert unique id
			_itoa_s(unique_id, BUFF_UID, MAX_PATH, 10);

			// append 
			strcat_s(BUFF_PATH, MAX_PATH, BUFF_UID);

			if(!dirExists(BUFF_PATH))
			{
				// Create a new dynamic directory
				if(!CreateDirectory(BUFF_PATH, NULL))
					printf("\nCouldn't create %s directory.\n", BUFF_PATH);
			}
			// save path_out_root_img
			strcpy_s(path_out_root_img, MAX_PATH, BUFF_PATH);

			// create dynamic path 
			strcat_s(BUFF_PATH, MAX_PATH, blobDirName);

			if(!dirExists(BUFF_PATH))
			{
				// Create a new dynamic directory
				if(!CreateDirectory(BUFF_PATH, NULL))
					printf("\nCouldn't create %s directory.\n", BUFF_PATH);
			}
			// save path_out_root_img_blob
			strcpy_s(path_out_root_img_blob, MAX_PATH, BUFF_PATH);
		}

		std::vector<cv::Vec2d> unityOrientations;

		// *_u8 hold the subsampled 8-bit grayscale frames
		cv::Mat i1_u8;
		cv::Mat i2_u8;		

		//CV_16S - 16-bit signed integers ( -32768..32767 )
		// *_f hold the smoothed, subsamples
		cv::Mat i1_f;
		cv::Mat i2_f;

		// td holdes values of the difference i2_f-i1_f: values [-255;255]	
		cv::Mat td;
		td.convertTo(td, CV_16S);

		// td_bin holdes the binarized td image
		cv::Mat td_bin;

		//START foreach temporal difference 'td'
		for(auto it = seq_in.begin(); (it+1)!=seq_in.end(); ++it, picID++)
		{

			// if first loop, init both 
			if(it == seq_in.begin())
			{	
				// get subsamples
				cv::resize(*it, i1_u8, cv::Size(), WDO_IMAGE_RED_SCALE, WDO_IMAGE_RED_SCALE, cv::INTER_AREA);
				cv::resize(*(it+1), i2_u8, cv::Size(), WDO_IMAGE_RED_SCALE, WDO_IMAGE_RED_SCALE, cv::INTER_AREA);

				// convert uint8 to float
				i1_u8.convertTo(i1_f, CV_16SC1);
				i2_u8.convertTo(i2_f, CV_16SC1);

				// smooth subsamples
				cv::filter2D(i1_f, i1_f, -1, gaussKernel);
				cv::filter2D(i2_f, i2_f, -1, gaussKernel);
			}
			// else, swap i1 = i2, load new i2
			else
			{	
				// save 50% work 
				i1_f = i2_f.clone();

				// get new subsample
				cv::resize(*(it+1), i2_u8, cv::Size(), WDO_IMAGE_RED_SCALE, WDO_IMAGE_RED_SCALE, cv::INTER_AREA);

				// convert uint8 to float
				i2_u8.convertTo(i2_f, CV_16SC1);

				// smooth subsamples
				cv::filter2D(i2_f, i2_f, -1, gaussKernel);
			}

			// get temproal difference
			td = i2_f-i1_f;

			// smooth td
			cv::filter2D(td, td, -1, gaussKernel);

			// init binary image
			td_bin = cv::Mat::zeros(td.size(), CV_8UC1);

			// get binary image from td
			WaggleDanceOrientator::extractBinaryImageFromTD(&td, &td_bin);

			// get angles as vector from td
			WaggleDanceOrientator::extractUnityOrientationsFromBinaryImage(&td_bin, &unityOrientations);

		}//END for each temporal difference 'td'

		cv::Point2d orient = WaggleDanceOrientator::getMeanOrientationFromUnityOrientations(&unityOrientations);

		if(WDO_VERBOSE)
			WaggleDanceOrientator::saveDetectedOrientationImages(&seq_in, &orient);


		return orient;
	}
	cv::Point2d WaggleDanceOrientator::extractOrientationFromPositions(std::vector<cv::Point2d> positions, cv::Point2d position_last)
	{
		printf("extracting orientation\n");
		printf("size of positions: %d\n", (int)positions.size());
		cv::Vec4f direction;
		std::vector<cv::Point2f> positionsF;
		cv::Mat(positions).copyTo(positionsF);
		//cv::Mat X(positions);

		//cv::Mat X(positions.size(), 2, CV_32F);
		//cv::Mat X2 = cv::Mat(positions, true).reshape(1);
		//X2.convertTo(X, CV_32F);
		//cv::Mat X (positions.size(), 2, CV_32FC1);
		//int i = 0;
		//for (auto &p : positions) {
		//	X.row(i) = cv::Mat();
		//	++i;
		//}
		cv::fitLine(positionsF, direction, CV_DIST_L2, 0, 0.01, 0.01);
		//std::cout << X << std::endl;
		cv::Point2d orient_raw = cv::Point2d(direction[0], direction[1]);
		printf("orientation by fitline: %f\n", atan2(orient_raw.y, orient_raw.x) * 180. / 3.14259);

		cv::Point2d orient_raw_naive = position_last - positions[0];

		printf("orientation by naive: %f\n", atan2(orient_raw_naive.y, orient_raw_naive.x) * 180. / 3.14259);
		if(cv::norm(orient_raw) > 0)
			return (orient_raw * (1.0 / cv::norm(orient_raw)));
		else
			return cv::Point2d(1,1);
	}


	cv::Point2d WaggleDanceOrientator::extractNaiveness(std::vector<cv::Point2d> positions, cv::Point2d position_last)
	{

		cv::Point2d orient_raw = position_last - positions[0];
		if (cv::norm(orient_raw) > 0)
			return (orient_raw * (1.0 / cv::norm(orient_raw)));
		else
			return cv::Point2d(1, 1);
	}


	/*
	handles the problem that orientations are unaware of head/tail 
	*/
	cv::Vec2d WaggleDanceOrientator::getMeanOrientationFromUnityOrientations(std::vector<cv::Vec2d> * unityOrientations_ptr)
	{
		std::vector<cv::Vec2d *> class_head_ptr_list;
		cv::Vec2d class_head_mean;
		double dist_head;
		std::vector<cv::Vec2d *> class_tail_ptr_list;
		cv::Vec2d class_tail_mean;
		double dist_tail;

		// assing each orientation one of two classes
		for(auto it=(*unityOrientations_ptr).begin(); it!=(*unityOrientations_ptr).end(); ++it)
		{
			//if it is the first orientation, init classes
			if(class_head_ptr_list.empty())
			{
				// init head class to its orientation
				class_head_mean = *it;
				// and tail class to its counter orientation
				class_tail_mean = - *it;
				// add orientation to head class pointer list
				class_head_ptr_list.push_back(&(*it));
				continue;
			}
			// check nearest distance, assign to appropriate class
			dist_head = cv::norm(class_head_mean, *it);
			dist_tail = cv::norm(class_tail_mean, *it);

			// assign to head class
			if(dist_head <= dist_tail)
			{
				// add orientation to head class pointer list
				class_head_ptr_list.push_back(&(*it));
				// recalculate head class mean
				WaggleDanceOrientator::getMeanUnityOrientation(&class_head_mean, &class_head_ptr_list);
			}
			// assign to tail class
			else
			{
				// add orientation to tail class pointer list
				class_tail_ptr_list.push_back(&(*it));
				// recalculate tail class mean
				WaggleDanceOrientator::getMeanUnityOrientation(&class_tail_mean, &class_tail_ptr_list);
			}
		}

		// flip all tail orientations to head
		for(auto it=class_tail_ptr_list.begin(); it!=class_tail_ptr_list.end(); ++it)
		{
			**it *= -1;
		}

		// merge flipped tail ptr list into head ptr list
		class_head_ptr_list.insert( class_head_ptr_list.end(), class_tail_ptr_list.begin(), class_tail_ptr_list.end());

		// finally, recalculate head class
		WaggleDanceOrientator::getMeanUnityOrientation(&class_head_mean, &class_head_ptr_list);

		return class_head_mean;
	}
	void WaggleDanceOrientator::getMeanUnityOrientation(cv::Vec2d *class_mean_ptr, std::vector<cv::Vec2d *> *class_ptr_list_ptr)
	{
		// reset mean to (0,0);
		(*class_mean_ptr)[0] = 0; (*class_mean_ptr)[1] = 0;

		// recalculate class mean
		for(auto it=(*class_ptr_list_ptr).begin(); it!=(*class_ptr_list_ptr).end(); ++it)
		{
			(*class_mean_ptr) += **it;
		}

		// it is guaranteed:(*class_ptr_list_ptr).size() > 0
		(*class_mean_ptr) *= 1.0 / (*class_ptr_list_ptr).size();

		// remap to unity circle
		(*class_mean_ptr) *= 1.0 / cv::norm((*class_mean_ptr));
	}
	void WaggleDanceOrientator::extractBinaryImageFromTD(cv::Mat *td_ptr, cv::Mat *td_bin_ptr)
	{
		double _stddevFactor = 2;

		cv::Mat td_norm;
		WaggleDanceOrientator::stretch(td_ptr, &td_norm);

		cv::Scalar mean,stddev;

		cv::meanStdDev (td_norm, mean, stddev);
		double limit = _stddevFactor * stddev[0];
		//std::cout<<mean<<std::endl;
		//std::cout<<stddev<<std::endl;
		//std::cout<<td_norm<<std::endl;

		cv::Size td_size((*td_ptr).size());

		for(int i=0; i<td_size.height; i++)
		{
			for(int j=0; j<td_size.width; j++)
			{
				if(abs(td_norm.at<float>(i,j) - mean[0]) > limit)
				{
					(*td_bin_ptr).at<unsigned char>(i,j) = 255;
				}
			}
		}

		// finally, apply SSA filter
		WaggleDanceOrientator::statisticalSmoothingFilter(td_bin_ptr);
	}
	void WaggleDanceOrientator::extractUnityOrientationsFromBinaryImage(cv::Mat * td_bw_mat_ptr, std::vector<cv::Vec2d> * unityOrientations_ptr)
	{
		// get TD image area size
		double _TD_AREA_LIMIT = (*td_bw_mat_ptr).rows * (*td_bw_mat_ptr).cols * 0.02;

		//convert Mat* to Iplimage* 
		IplImage *td_bw_ipl_ptr = &(*td_bw_mat_ptr).operator IplImage(); 

		IplImage *labelImg_ptr = cvCreateImage(cvGetSize(td_bw_ipl_ptr), IPL_DEPTH_LABEL, 1);
		cvb::CvBlobs blobs;

		// run blob extraction
		cvb::cvLabel(td_bw_ipl_ptr, labelImg_ptr, blobs);

		cv::Mat labels(labelImg_ptr);

		// outer loop allocation
		double angle;
		std::vector<double> majMinAxisLenghts(2);
		cv::Mat majMinAxisMoments;

		// for each blob..
		blobID=0;
		for (cvb::CvBlobs::const_iterator it=blobs.begin(); it!=blobs.end(); ++it)
		{
			// check that blob area is bigger then 2% of the image
			if((*it).second->area <= _TD_AREA_LIMIT)
				continue;

			/* http://en.wikipedia.org/wiki/Image_moment#Examples_2 
			M =	[ u20, u11; u11, u02 ]	
			*/
			majMinAxisMoments = (cv::Mat_<double>(2,2) << 
				(*it).second->u20, (*it).second->u11,
				(*it).second->u11, (*it).second->u02);

			// get length of Major and Minor Axis as eigen values of majMinAxisMoments
			cv::eigen(majMinAxisMoments, majMinAxisLenghts);

			// check that blob has: height > 3x width
			if( (majMinAxisLenghts[1] > 0) & ((majMinAxisLenghts[0]/majMinAxisLenghts[1]) >= 10))
			{
				// add angle of that blob (SI in radian)
				angle = cvAngle((*it).second);

				(*unityOrientations_ptr).push_back(cv::Vec2d(cos(angle),sin(angle)));

				if(WDO_VERBOSE)
				{
					WaggleDanceOrientator::saveDetectedBlobImage(&(*it->second), &labels, &majMinAxisLenghts);
					blobID++;
				}
			}
		}
	}
	void WaggleDanceOrientator::saveDetectedOrientationImages(const std::vector<cv::Mat> *seq_in_ptr, const cv::Point2d *orient_ptr)
	{
		// get image dimensions
		cv::Size size = (*seq_in_ptr)[0].size();

		// get direction length
		double length = MIN(size.width/2, size.height/2) * 0.8;

		// get image center point
		cv::Point2d CENTER(size.width/2., (size.height/2.));

		// get image orientation point
		cv::Point2d HEADIN(CENTER);
		HEADIN += (*orient_ptr)*length;

		// preallocate 3 channel image output buffer
		cv::Mat image_out(size, CV_8UC3);

		// set image file name buffer
		char BUFF_PATH[MAX_PATH];
		char BUFF_UID[MAX_PATH];

		std::size_t i=0;
		for (auto it=seq_in_ptr->begin(); it!=seq_in_ptr->end(); ++it)
		{
			// convert & copy input image into BGR
			cv::cvtColor(*it, image_out, CV_GRAY2BGR);

			// create dynamic path_out string
			strcpy_s(BUFF_PATH, MAX_PATH, path_out_root_img);
			strcat_s(BUFF_PATH, MAX_PATH, "\\image_");

			// convert picID=[0;seq_in_ptr->size()-1]
			sprintf_s(BUFF_UID, MAX_PATH, "%03d", i);

			// append 
			strcat_s(BUFF_PATH, MAX_PATH, BUFF_UID);
			strcat_s(BUFF_PATH, MAX_PATH, ".png");

			//cv::resize(image_out,image_out,cv::Size(), 10.0,10.0, cv::INTER_AREA);

			// write image to disk
			WaggleDanceOrientator::saveImage(&image_out, BUFF_PATH);
			i++;

			//clear buffers
			memset(BUFF_PATH,0,MAX_PATH*sizeof(char));
			memset(BUFF_UID,0,MAX_PATH*sizeof(char));
		}

		//finally draw detected orientation
		//black background
		image_out.setTo(0);

		// draw the orientation line
		cv::line(image_out, CENTER, HEADIN, CV_RGB(0.,255.,0.), 2, CV_AA);

		// create dynamic path_out string
		strcpy_s(BUFF_PATH, MAX_PATH, path_out_root_img);
		strcat_s(BUFF_PATH, MAX_PATH, "\\orient.png");

		WaggleDanceOrientator::saveImage(&image_out, BUFF_PATH);
	}
	void WaggleDanceOrientator::saveDetectedBlobImage(cvb::CvBlob * blob_ptr, cv::Mat * labels_ptr, std::vector<double> * majMinAxisLenghts_ptr)
	{	
		double _zoomFactor = 4;
		std::stringstream ss;

		// copy blob pixels
		cv::Mat blob_visual((*labels_ptr).size(), CV_8UC1, cv::Scalar(0));
		for (unsigned int r=blob_ptr->miny; r < blob_ptr->maxy; r++)
		{
			for (unsigned int c=blob_ptr->minx; c  < blob_ptr->maxx; c++)
			{
				if((*labels_ptr).at<unsigned int>(r,c) == blob_ptr->label)
					blob_visual.at<unsigned char>(r,c) = 255;
			}
		}

		// zoom to enhance visability
		cv::resize(blob_visual, blob_visual, cv::Size(), _zoomFactor, _zoomFactor, cv::INTER_AREA);

		// convert to 3channel color type
		cv::cvtColor(blob_visual, blob_visual, CV_GRAY2BGR);

		// TODO: pass angle over function
		double angle = cvAngle(blob_ptr);

		// define Major axis
		double lengthLine = MAX(blob_ptr->maxx-blob_ptr->minx, blob_ptr->maxy-blob_ptr->miny);
		cv::Point2d HEAD(blob_ptr->centroid.x+lengthLine*cos(angle), blob_ptr->centroid.y+lengthLine*sin(angle));
		cv::Point2d CENT(blob_ptr->centroid.x, blob_ptr->centroid.y);

		// define Minor axis, + 90° to Major, relative line length according to axis lengths
		double angle_minor = angle + 0.5 * CV_PI;
		lengthLine *= (*majMinAxisLenghts_ptr)[1] / (*majMinAxisLenghts_ptr)[0];
		cv::Point2d BOTT(blob_ptr->centroid.x+lengthLine*cos(angle_minor), blob_ptr->centroid.y+lengthLine*sin(angle_minor));

		// call cv line funcs
		cv::line(blob_visual, CENT*_zoomFactor, HEAD*_zoomFactor, CV_RGB(0.,255.,0.),1,CV_AA);
		cv::line(blob_visual, CENT*_zoomFactor, BOTT*_zoomFactor, CV_RGB(255.,0.,0.),1,CV_AA);


		// set blob file name
		char BUFF_PATH[MAX_PATH];
		char BUFF_UID[MAX_PATH];
		// create dynamic path_out string
		strcpy_s(BUFF_PATH, MAX_PATH, path_out_root_img_blob);
		strcat_s(BUFF_PATH, MAX_PATH, "\\image_");

		// convert picID
		sprintf_s(BUFF_UID, MAX_PATH, "%03d", picID);
		//append
		strcat_s(BUFF_PATH, MAX_PATH, BUFF_UID);

		//append
		strcat_s(BUFF_PATH, MAX_PATH, "_blob_");

		// convert unique id
		_itoa_s(blobID, BUFF_UID, MAX_PATH, 10);
		//append
		strcat_s(BUFF_PATH, MAX_PATH, BUFF_UID);

		// append 
		strcat_s(BUFF_PATH, MAX_PATH, ".png");


		WaggleDanceOrientator::saveImage(&blob_visual, BUFF_PATH);
		WaggleDanceOrientator::writeBlobsDetailLine(angle, majMinAxisLenghts_ptr);
	}
	void WaggleDanceOrientator::writeBlobsDetailLine(double angle, std::vector<double> * majMinAxisLenghts_ptr)
	{
		FILE * file_blobs_detail_ptr;

		char BUFF_PATH[MAX_PATH];
		strcpy_s(BUFF_PATH, MAX_PATH, path_out_root_img);
		strcat_s(BUFF_PATH, MAX_PATH, file_blobs_detail);


		fopen_s (&file_blobs_detail_ptr, BUFF_PATH, "a+");

		if(file_blobs_detail_ptr != NULL)
		{
			fprintf(file_blobs_detail_ptr, "%d\t %d\t %.2f\t %.1f\t %.1f\t %.1f\n", picID, blobID, angle, 
				(*majMinAxisLenghts_ptr)[0],(*majMinAxisLenghts_ptr)[1], 
				(*majMinAxisLenghts_ptr)[0]/(*majMinAxisLenghts_ptr)[1]);

			fclose(file_blobs_detail_ptr);
		}
		else
		{
			std::cerr <<"Warning! Could not write blob detail file!"<<std::endl;
		}
	}

	/* stretches the values of a td image to be inbetween [0;1] 
	TODO: check if conversion really is neccessary for algorithm, too?
	*/
	void WaggleDanceOrientator::stretch(cv::Mat * in_ptr, cv::Mat * out_ptr)
	{
		(*in_ptr).convertTo(*out_ptr, CV_32F);

		double minVal, maxVal;
		cv::minMaxIdx(*out_ptr, &minVal, &maxVal);

		if(minVal < 0)
		{
			minVal = abs(minVal);
			*out_ptr += minVal;
			maxVal += minVal;
		}
		else
		{
			*out_ptr -= minVal;
			maxVal -= minVal;
			std::cerr <<"Warning! Unexpected minVal >= 0 in WaggleDanceOrientator::stretch encountered!"<<std::endl;
		}

		*out_ptr *= 1/maxVal;
	}
	/*The Statistical Smoothing Filter (SSF) algorithm reduces the noise present in a binary
	image by eliminating small areas and filling small holes. This simple but efficient
	method uses a 3x3 operator or window, and is based on statistical decision criteria.*/
	//TODO: use instruction list rather then full comparison
	void WaggleDanceOrientator::statisticalSmoothingFilter(cv::Mat *img_ptr)
	{
		cv::Mat img_filtered = (*img_ptr).clone();

		std::vector<cv::Point2i> _1_list;
		std::vector<cv::Point2i> _0_list;
		while(true)
		{
			// define initial location of 3x3 sub matrices in image, operate on input image
			// currently discard edges where kernel does not fit completly


			// attention! +/-1 row,col boundary
			for(int row=1; row < (*img_ptr).rows -1; row++)
			{
				// init position on left border
				cv::Rect ROI_Rect(0, row-1, 3, 3);
				cv::Mat ROI_SubMat( (*img_ptr), ROI_Rect);

				for(int col=1; col < (*img_ptr).cols -1; col++)
				{				
					// count how many pixel are set (==255)
					int positive = cv::countNonZero(ROI_SubMat);

					// different decision whether pixel is set
					if(ROI_SubMat.at<unsigned char>(1,1) > 0)
					{
						if (positive <= 2)
							_0_list.push_back(cv::Point2i(row,col));
						//img_filtered.at<unsigned char>(row,col) = 255;
						//else

						//img_filtered.at<unsigned char>(row,col) = 0;
					}   
					else
					{
						if (positive > 5)
							_1_list.push_back(cv::Point2i(row,col));
						//img_filtered.at<unsigned char>(row,col) = 255;
						//else
						//img_filtered.at<unsigned char>(row,col) = 0;
					}

					// move one position to the right
					ROI_SubMat.adjustROI(0,0,-1,1);
				}
			}
			// check for exit condition: both lists have zero elements
			if(_0_list.empty() & _1_list.empty())
				break;

			cv::Point2i position;
			while(!_0_list.empty())
			{
				position = _0_list.back();
				_0_list.pop_back();

				(*img_ptr).at<unsigned char>(position.x, position.y) = 0;
			}
			while(!_1_list.empty())
			{
				position = _1_list.back();
				_1_list.pop_back();

				(*img_ptr).at<unsigned char>(position.x, position.y) = 255;
			}
			// Guarantee: if no new filter instructions exit condition is true
		}
	}

	std::vector<cv::Mat> WaggleDanceOrientator::loadImagesFromFolder(const std::string dirInNameFormat)
	{
		std::vector<cv::Mat> out;

		cv::VideoCapture sequence(dirInNameFormat);
		if (!sequence.isOpened())
		{
			std::cerr << "Error! Failed to open Image Sequence!\n" << std::endl;
			return std::vector<cv::Mat>();
		}

		cv::Mat image;
		//cv::namedWindow("Image | q or esc to quit", CV_WINDOW_AUTOSIZE | CV_WINDOW_KEEPRATIO | CV_GUI_NORMAL);

		for(;;)
		{
			sequence >> image;

			if(image.empty())
				break;
			else
				out.push_back(cv::Mat(image.clone()));
		}

		return out;
	}

	void WaggleDanceOrientator::showImagesFromFolder(const std::string dirInNameFormat)
	{
		cv::VideoCapture sequence(dirInNameFormat);
		if (!sequence.isOpened())
		{
			std::cerr << "Error! Failed to open Image Sequence!\n" << std::endl;
			return;
		}

		cv::Mat image;
		cv::namedWindow("Image | q or esc to quit", CV_WINDOW_AUTOSIZE | CV_WINDOW_KEEPRATIO | CV_GUI_NORMAL);

		for(;;)
		{
			sequence >> image;
			if(image.empty())
				break;

			imshow("Image | q or esc to quit", image);

			char key = (char)cv::waitKey(500);
			if(key == 'q' || key == 'Q' || key == 27)
				break;
		}

	}

	void WaggleDanceOrientator::showImage(const cv::Mat *img_ptr)
	{
		if((*img_ptr).empty())
		{
			std::cerr << "Error! WaggleDanceOrientator::showImage - image empty!\n" << std::endl;
			return;
		}
		std::string winName = "Image | press any key to quit";
		cv::namedWindow(winName, CV_WINDOW_AUTOSIZE | CV_WINDOW_KEEPRATIO | CV_GUI_NORMAL);
		imshow(winName, *img_ptr);
		cv::waitKey();
	}

	void WaggleDanceOrientator::saveImage(const cv::Mat *img_ptr, const char * path_ptr)
	{
		bool result;

		try {

			result = cv::imwrite(path_ptr, *img_ptr);
		}
		catch (std::runtime_error& ex) {
			std::cerr << "Exception saving image '"<<path_ptr
				<<"': "<<ex.what() << std::endl;
		}
		if(!result)
			std::cerr << "Error saving image '"<<path_ptr<<"'!"<< std::endl;
	}
	/*
	Export function to interface with MATLAB " M = dlmread(filename) "
	*/
	void WaggleDanceOrientator::exportImage(const cv::Mat *img_ptr, const std::string path_ptr)
	{
		FILE * fid;
		fopen_s(&fid, path_ptr.c_str(), "w");

		if (!fid)
		{	
			std::cerr << "Error! Can not open output file: "<<path_ptr<<std::endl;
			return;
		}

		std::stringstream ss;
		for(int i=0; i<(*img_ptr).rows; i++)
		{
			ss.str(std::string());
			for(int j=0; j<(*img_ptr).cols; j++)
			{
				ss << static_cast<int>((*img_ptr).at<short>(i,j))<<" ";
			}
			//std::cout<<ss.str()<<std::endl;
			fprintf(fid, "%s\n", ss.str().c_str());
		}

		fclose(fid);
	}
} /* namespace WaggleDanceDetector */