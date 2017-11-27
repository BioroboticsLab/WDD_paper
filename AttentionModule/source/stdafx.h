#pragma once
#define _CRT_SECURE_NO_WARNINGS

#define TEST_MODE_ON

#define WDD_LAYER2_MAX_POS_DDS 100
#define WDD_FBUFFER_SIZE 32
#define WDD_FREQ_NUMBER 7

#if defined(TEST_MODE_ON)
	#define WDD_FRAME_RATE 102
#else
	#define WDD_FRAME_RATE 102
#endif

#define VFB_MAX_FRAME_HISTORY 600

#if !defined(ARMA_NO_DEBUG)
  #define ARMA_NO_DEBUG
#endif

#if !defined(WDD_EXTRACT_ORIENT)
	#define WDD_EXTRACT_ORIENT
#endif

/*
#if !defined(WDD_DDL_DEBUG_FULL)
	#define WDD_DDL_DEBUG_FULL
	#define WDD_DDL_DEBUG_FULL_MAX_FRAME 40
	#define WDD_DDL_DEBUG_FULL_MAX_DDS 10
	#define WDD_DDL_DEBUG_FULL_MAX_FIRING_DDS 10
#endif
	*/

#include "targetver.h"
#include <atomic>
#include <chrono>
#include <iostream>
#include <stdio.h>
#include <stdexcept>
#include <cmath>
#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>
#define NOMINMAX
//#include <tchar.h>
#include "atlstr.h"
#include <windows.h>
#include "Shlwapi.h"

#include <array>
#include <map>
#include <limits>
#include <list>
#include <vector>

#include <armadillo>
#include "CLEyeMulticam.h"
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <opencv2/opencv.hpp>

enum RUN_MODE {TEST, LIVE};

struct CamConf {
	std::size_t camId;
	char guid_str[64];
	std::array<cv::Point2i,4> arena;
	std::array<cv::Point2i,4> arena_lowRes;
	bool configured;
};

struct _MouseInteraction
{
	// id der Ecke, die mit der Maus angehovert wurde
	int cornerHovered;
	// id der Ecke, die mit der Maus angeklickt wurde
	int cornerSelected;

	cv::Point lastPosition;
}; 