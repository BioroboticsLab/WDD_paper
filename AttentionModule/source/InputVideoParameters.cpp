#include "stdafx.h"
#include "InputVideoParameters.h"
#include "TypeToString.h"

namespace wdd
{

	InputVideoParameters::InputVideoParameters(cv::VideoCapture * vc)
	{
		_vc = vc;

		frame_width = getFrameWidthOf(vc);
		frame_height = getFrameHeightOf(vc);
		frame_rate = getFrameRateOf(vc);
		frame_format = getFrameFormatOf(vc);
	}

	InputVideoParameters::~InputVideoParameters()
	{
		// TODO Auto-generated destructor stub
	}

	/*
	* member functions
	*/
	int InputVideoParameters::getFrameWidth()
	{
		return frame_width;
	}
	int InputVideoParameters::getFrameHeight()
	{
		return frame_height;
	}
	double InputVideoParameters::getFrameRate()
	{
		return frame_rate;
	}
	std::string InputVideoParameters::getFrameFormat()
	{
		return frame_format;
	}

	void InputVideoParameters::printProperties()
	{
		InputVideoParameters::printPropertiesOf(_vc);
	}
	/*
	* static functions
	*/
	// get functions
	int InputVideoParameters::getFrameWidthOf(cv::VideoCapture * vc)
	{
		return (int) vc->get(CV_CAP_PROP_FRAME_WIDTH);
	}
	int InputVideoParameters::getFrameHeightOf(cv::VideoCapture * vc)
	{
		return (int) vc->get(CV_CAP_PROP_FRAME_HEIGHT);
	}
	double InputVideoParameters::getFrameRateOf(cv::VideoCapture * vc)
	{
		return vc->get(CV_CAP_PROP_FPS);
	}
	std::string InputVideoParameters::getFrameFormatOf(cv::VideoCapture * vc)
	{
		return TypeToString::typeToString(static_cast<int>(vc->get(CV_CAP_PROP_FORMAT)));
	}
	// print function(s)
	void InputVideoParameters::printPropertiesOf(cv::VideoCapture * vc)
	{
		printf("Printing InputVideoParameters:\n");
		printf("frame_width: %d\n", InputVideoParameters::getFrameWidthOf(vc));
		printf("frame_height: %d\n", InputVideoParameters::getFrameHeightOf(vc));
		printf("frame_rate: %.1f\n", InputVideoParameters::getFrameRateOf(vc));
		printf("frame_format: %s\n",
			InputVideoParameters::getFrameFormatOf(vc).c_str());
	}
}