#pragma once
#include "CLEyeCameraCapture.h"

namespace wdd
{
	class VideoFrameBuffer
	{
		unsigned int _BUFFER_POS;
		unsigned long long _CURRENT_FRAME_NR;

		cv::Size _cachedFrameSize;
		cv::Size _extractFrameSize;
		cv::Point _sequenceFramePointOffset;

		cv::Mat _FRAME[VFB_MAX_FRAME_HISTORY];
		SYSTEMTIME st;
		int _last_hour;
		CamConf _CC;
	public:
		VideoFrameBuffer::VideoFrameBuffer(unsigned long long current_frame_nr, cv::Size cachedFrameSize, cv::Size extractFrameSize, CamConf _CC);
		~VideoFrameBuffer(void);

		void setSequecenFrameSize(cv::Size size);
		void addFrame(cv:: Mat * frame_ptr);
		void saveFullFrame();
		void drawArena(cv::Mat &frame);
		cv::Mat * getFrameByNumber(unsigned long long frame_nr);
		std::vector<cv::Mat> loadCroppedFrameSequenc(unsigned long long startFrame, unsigned long long endFrame, cv::Point2i center, double FRAME_REDFAC);
	};
} /* namespace WaggleDanceDetector */

