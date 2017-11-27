#pragma once

namespace wdd
{

	class TypeToString
	{
	public:
		TypeToString();
		~TypeToString();
		static std::string typeToString(int type);
		static void printType(int type);
		static void printType(cv::Mat mat);
	};

} /* namespace WaggleDanceDetector */
