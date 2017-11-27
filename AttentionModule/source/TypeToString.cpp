#include "stdafx.h"
#include "TypeToString.h"

namespace wdd
{

	TypeToString::TypeToString()
	{
		// TODO Auto-generated constructor stub

	}

	TypeToString::~TypeToString()
	{
		// TODO Auto-generated destructor stub
	}
	/* kudos "http://stackoverflow.com/questions/10167534/
	how-to-find-out-what-type-of-a-mat-object-is-with-mattype-in-opencv */
	std::string TypeToString::typeToString(int type)
	{
		std::string r;

		uchar depth = type & CV_MAT_DEPTH_MASK;
		uchar chans = 1 + (type >> CV_CN_SHIFT);

		switch ( depth )
		{
		case CV_8U:
			r = "8U";
			break;
		case CV_8S:
			r = "8S";
			break;
		case CV_16U:
			r = "16U";
			break;
		case CV_16S:
			r = "16S";
			break;
		case CV_32S:
			r = "32S";
			break;
		case CV_32F:
			r = "32F";
			break;
		case CV_64F:
			r = "64F";
			break;
		default:
			r = "User";
			break;
		}

		r += "C";
		r += (chans+'0');

		return r;
	}
	void TypeToString::printType(int type)
	{


		printf("%s\n",TypeToString::typeToString(type).c_str());

	}
	void TypeToString::printType(cv::Mat mat)
	{
		printf("Matrix: %s %dx%d \n",TypeToString::typeToString(mat.type()).c_str(),
			mat.cols, mat.rows );
	}
} /* namespace WaggleDanceDetector */