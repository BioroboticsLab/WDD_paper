#pragma once
#include "WaggleDanceDetector.h"
namespace wdd
{
	class WaggleDanceExport
	{
	public:

		static void write(const std::vector<cv::Mat> seq, const DANCE * d_ptr, std::size_t camID);
		static double uvecToRad(cv::Point2d in);
		static void execRootExistChk();
		static void setArena(std::array<cv::Point2i,4> auxArena);
		static void createYYYYMMDDFolder();
		static void createGenericFolder(char dir[]); 
		static int countDirectories(char dir[]);
	};

}