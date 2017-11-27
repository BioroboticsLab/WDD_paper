#include "stdafx.h"
#include "CLEyeCameraCapture.h"
#include "VideoFrameBuffer.h"
#include "WaggleDanceDetector.h"
#include "DotDetectorLayer.h"

namespace wdd{

	_MouseInteraction	_Mouse;
	CamConf _CC;

	
	WaggleDanceDetector * WDD_p = nullptr;

	void static findCornerPointNear(cv::Point2i p)
	{
		// we actually have a 4 point arena to work with
		if (_CC.arena.size() != 4)
			printf_s("WARNING! Unexpected number of corners for arena: %d - expect number = 4\n", _CC.arena.size());


		// check if there is already a corner marked as hovered
		if(_Mouse.cornerHovered > -1)
		{
			// revalidate distance
			if (cv::norm(_CC.arena[_Mouse.cornerHovered] - cv::Point2i(p)) < 5 ){
				// nothing to do, no GUI update needed
				return;
			}else
			{
				// unset corner marked as hovered
				_Mouse.cornerHovered = -1;				
				// done
				return;
			}
		}

		//find the corner the mouse points at
		// assertion Mouse.cornerHover = -1;
		for (std::size_t i = 0; i < _CC.arena.size(); i++)
		{
			if ( cv::norm(_CC.arena[i] - cv::Point2i(p)) < 5 )
			{
				_Mouse.cornerHovered = i;				
				break;
			}
		}
	}
	void static onMouseInput(int evnt, int x, int y, int flags, void* param)
	{
		cv::Point2i p(x, y);
		_Mouse.lastPosition = p;

		switch( evnt ){
		case CV_EVENT_MOUSEMOVE:
			// if a corner is selected, move the edge position
			if(_Mouse.cornerSelected > -1){
				_CC.arena[_Mouse.cornerSelected] = p;
			}else{
				// hover over corner point should trigger visual feedback
				findCornerPointNear(p);
			}
			break;

		case CV_EVENT_LBUTTONDOWN:
			// implement toggle: if corner was already selected, deselect it
			if(_Mouse.cornerSelected > -1)
				_Mouse.cornerSelected = -1;
			// else check if corner is hovered (=in range) and possibly select it
			else
				if (_Mouse.cornerHovered > -1)
					_Mouse.cornerSelected = _Mouse.cornerHovered;
			break;
		}

	}
	CLEyeCameraCapture::CLEyeCameraCapture(LPSTR windowName, GUID cameraGUID, CLEyeCameraColorMode mode, CLEyeCameraResolution resolution, float fps, CamConf CC, double dd_min_potential, int wdd_signal_min_cluster_size) :
		_cameraGUID(cameraGUID), _cam(NULL), _mode(mode), _resolution(resolution), _fps(fps), _running(false), _visual(true), aux_DD_MIN_POTENTIAL(dd_min_potential), aux_WDD_SIGNAL_MIN_CLUSTER_SIZE(wdd_signal_min_cluster_size)
	{
		strcpy_s(_windowName, windowName);

		// used for live monitoring (aka Run())
		if(resolution == CLEYE_QVGA)
		{
			_FRAME_WIDTH = 320;
			_FRAME_HEIGHT = 240;
		}
		// used for arena config (aka Setup())
		else if(resolution == CLEYE_VGA)
		{
			_FRAME_WIDTH = 640;
			_FRAME_HEIGHT = 480;
		}
		else
		{
			std::cerr<<"ERROR! Unknown resolution format!"<<std::endl;
			exit(-11);
		}
		_CC = CC;
		_setupModeOn = !_CC.configured;
	}
	bool CLEyeCameraCapture::StartCapture()
	{
		_running = true;

		cvNamedWindow(_windowName, CV_WINDOW_NORMAL | CV_GUI_EXPANDED);

		// Start CLEye image capture thread
		_hThread = CreateThread(NULL, 0, &CLEyeCameraCapture::CaptureThread, this, 0, 0);
		if(_hThread == NULL)
		{
			MessageBox(NULL,"Could not create capture thread","WaggleDanceDetector", MB_ICONEXCLAMATION);
			return false;
		}
		return true;
	}
	void CLEyeCameraCapture::StopCapture()
	{
		if(!_running)	return;
		_running = false;
		WaitForSingleObject(_hThread, 1000);
		cvDestroyWindow(_windowName);
	}
	void CLEyeCameraCapture::setVisual(bool visual)
	{
		_visual = visual;
	}
	void CLEyeCameraCapture::setSetupModeOn(bool setupMode)
	{
		_setupModeOn = setupMode;
	}
	const CamConf * CLEyeCameraCapture::getCamConfPtr()
	{
		return &_CC;
	}
	void CLEyeCameraCapture::IncrementCameraParameter(int param)
	{
		if(!_cam)	return;

	}
	void CLEyeCameraCapture::DecrementCameraParameter(int param)
	{
		if(!_cam)	return;
	}

	void CLEyeCameraCapture::Setup()
	{
		// listen for mouse interaction
		cv::setMouseCallback(_windowName, onMouseInput);
		// id der Ecke, die mit der Maus angehovert wurde
		_Mouse.cornerHovered = -1;
		// id der Ecke, die mit der Maus angeklickt wurde
		_Mouse.cornerSelected = -1;

		int w, h;
		IplImage *pCapImage;
		PBYTE pCapBuffer = NULL;
		// Create camera instance
		_cam = CLEyeCreateCamera(_cameraGUID, CLEYE_MONO_PROCESSED, CLEYE_VGA, 30);
		if(_cam == NULL)
		{
			std::cerr<<"ERROR! Could not create camera instance!"<<std::endl;
			return;
		}
		if(CLEyeSetCameraParameter(_cam, CLEYE_AUTO_GAIN, true) == false)
			std::cerr<<"WARNING! Could not set CLEYE_AUTO_GAIN = true!"<<std::endl;

		if(CLEyeSetCameraParameter(_cam, CLEYE_AUTO_EXPOSURE, true) == false)
			std::cerr<<"WARNING! Could not set CLEYE_AUTO_EXPOSURE = true!"<<std::endl;

		if(CLEyeSetCameraParameter(_cam, CLEYE_AUTO_WHITEBALANCE, true) == false)
			std::cerr<<"WARNING! Could not set CLEYE_AUTO_WHITEBALANCE = true!"<<std::endl;

		// Get camera frame dimensions
		CLEyeCameraGetFrameDimensions(_cam, w, h);
		// Depending on color mode chosen, create the appropriate OpenCV image
		if(_mode == CLEYE_COLOR_PROCESSED || _mode == CLEYE_COLOR_RAW)
			pCapImage = cvCreateImage(cvSize(w, h), IPL_DEPTH_8U, 4);
		else
			pCapImage = cvCreateImage(cvSize(w, h), IPL_DEPTH_8U, 1);

		// Start capturing
		CLEyeCameraStart(_cam);

		IplImage* image = cvCreateImage(cvSize(pCapImage->width, pCapImage->height), IPL_DEPTH_8U, 3);
		// image capturing loop
		while(_running)
		{
			cvGetImageRawData(pCapImage, &pCapBuffer);
			CLEyeCameraGetFrame(_cam, pCapBuffer);
			cvConvertImage(pCapImage, image);
			drawArena(cv::Mat(image));
			cvShowImage(_windowName, image);
		}
		cvReleaseImage(&image);

		// Stop camera capture
		CLEyeCameraStop(_cam);
		// Destroy camera object
		CLEyeDestroyCamera(_cam);
		// Destroy the allocated OpenCV image
		cvReleaseImage(&pCapImage);
		_cam = NULL;
		// set CamConf to configured
		_CC.configured = true;
	}
	void CLEyeCameraCapture::drawArena(cv::Mat &frame)
	{
		// we actually have a 4 point arena to work with
		if (_CC.arena.size() != 4){
			printf_s("WARNING! Unexpected number of corners for arena: %d - expect number = 4\n", _CC.arena.size());
		}
		// setupModeOn -> expect 640x480
		if(_setupModeOn)
		{
			for (std::size_t i = 0; i < _CC.arena.size(); i++)
			{
				cv::line(frame, _CC.arena[i], _CC.arena[(i+1) % _CC.arena.size()], CV_RGB(0, 255, 0));

				if (_Mouse.cornerHovered > -1)
					cv::circle(frame, _CC.arena[_Mouse.cornerHovered], 5, CV_RGB(0, 255, 0));

				if (_Mouse.cornerSelected > -1)
					cv::circle(frame, _CC.arena[_Mouse.cornerSelected], 5, CV_RGB(255, 255, 255));
			}
		}	
		// else from 640x480 -> 180x120
		else
		{
			for (std::size_t i = 0; i < _CC.arena_lowRes.size(); i++)
			{
				cv::line(frame, 
					_CC.arena_lowRes[i],  _CC.arena_lowRes[(i+1) % _CC.arena_lowRes.size()],
					CV_RGB(0, 0, 0));
			}
		}		
	}
	void CLEyeCameraCapture::drawPosDDs(cv::Mat &frame)
	{
		for(auto it=DotDetectorLayer::DD_SIGNALS_IDs.begin(); it!= DotDetectorLayer::DD_SIGNALS_IDs.end(); ++it)
			cv::circle(frame, DotDetectorLayer::DD_POSITIONS.at(*it), 1, CV_RGB(0, 255, 0));
	}

	char * hbf_extension = ".wtchdg";
	void CLEyeCameraCapture::makeHeartBeatFile()
	{
		char path[MAX_PATH];
		extern char _NAME_OF_EXE[MAX_PATH];
		extern char _FULL_PATH_EXE[MAX_PATH];

		strcpy_s(path, MAX_PATH, _FULL_PATH_EXE);
		strcat_s(path, MAX_PATH, "\\");
		strcat_s(path, MAX_PATH, _NAME_OF_EXE);
		strcat_s(path, MAX_PATH, hbf_extension);

		FILE * pFile;

		fopen_s (&pFile, path,"w");
		if (pFile!=NULL)
			fclose (pFile);
		else
			std::cerr<<"ERROR! Could not create heartbeat file: "<<path<<std::endl;		
	}

	//Adoption from stackoverflow
	//http://stackoverflow.com/questions/13080515/rotatedrect-roi-in-opencv
	//decide whether point p is in the ROI.
	//The ROI is a RotatableBox whose 4 corners are stored in pt[] 
	bool CLEyeCameraCapture::pointIsInArena(cv::Point p)
	{		
		double result[4];
		for(int i=0; i<4; i++)
		{
			result[i] = computeProduct(p, _CC.arena_lowRes[i], _CC.arena_lowRes[(i+1)%4]);
			// all have to be 1, exit on first negative encounter
			if(result[i] < 0)
				return false;
		}
		if(result[0]+result[1]+result[2]+result[3] == 4)
			return true;
		else
			return false;
	}
	//Adoption from stackoverflow
	//http://stackoverflow.com/questions/1560492/how-to-tell-whether-a-point-is-to-the-right-or-left-side-of-a-line
	//Use the sign of the determinant of vectors (AB,AM), where M(X,Y) is the query point:
	double CLEyeCameraCapture::computeProduct(const cv::Point p, const cv::Point2i a, const cv::Point2i b)
	{
		double x = ((b.x-a.x)*(p.y-a.y) - (b.y-a.y)*(p.x-a.x));

		return (x >= 0) ? 1 : -1;
	}
	void CLEyeCameraCapture::Run()
	{
		//
		//	Global: video configuration
		//
		int FRAME_RED_FAC = 1;

		//
		//	Layer 1: DotDetector Configuration
		//
		int DD_FREQ_MIN = 11;
		int DD_FREQ_MAX = 17;
		double DD_FREQ_STEP = 1;
		double DD_MIN_POTENTIAL = aux_DD_MIN_POTENTIAL;

		//
		//	Layer 2: Waggle SIGNAL Configuration
		//
		double WDD_SIGNAL_DD_MAXDISTANCE = 2.3;
		int		WDD_SIGNAL_MIN_CLUSTER_SIZE = aux_WDD_SIGNAL_MIN_CLUSTER_SIZE;

		//
		//	Layer 3: Waggle Dance Configuration
		//
		double	WDD_DANCE_MAX_POSITION_DISTANCEE = sqrt(2);
		int		WDD_DANCE_MAX_FRAME_GAP = 3;
		//TODO ORIGINAL 20
		int		WDD_DANCE_MIN_CONSFRAMES = 20;

		//
		//	Develop: Waggle Dance Configuration
		//
		bool visual = true;
		bool wdd_write_dance_file = false;
		bool wdd_write_signal_file = false;
		int wdd_verbose = 0;

		// prepare frame_counter
		unsigned long long frame_counter_global = 0;
		unsigned long long frame_counter_warmup = 0;

		// prepare videoFrameBuffer
		VideoFrameBuffer videoFrameBuffer(frame_counter_global, cv::Size(_FRAME_WIDTH, _FRAME_HEIGHT), cv::Size(30,30), _CC);

		// prepare buffer to hold mono chromized input frame
		cv::Mat frame_input_monochrome = cv::Mat(_FRAME_HEIGHT, _FRAME_WIDTH, CV_8UC1);

		// prepare buffer for possible output
		cv::Mat frame_visual; IplImage frame_visual_ipl = frame_visual;

		// prepare buffer to hold target frame
		double resize_factor =  pow(2.0, FRAME_RED_FAC);

		int frame_target_width = cvRound(_FRAME_WIDTH/resize_factor);
		int frame_target_height = cvRound(_FRAME_HEIGHT/resize_factor);
		cv::Mat frame_target = cv::Mat(frame_target_height, frame_target_width, CV_8UC1);


		std::cout<<"Printing WaggleDanceDetector frame parameter:"<<std::endl;
		printf("frame_height: %d\n", frame_target_height);
		printf("frame_width: %d\n", frame_target_width);
		printf("frame_rate: %d\n", WDD_FRAME_RATE);
		printf("frame_red_fac: %d\n", FRAME_RED_FAC);

		//
		// prepare DotDetectorLayer config vector
		//
		std::vector<double> ddl_config;
		ddl_config.push_back(DD_FREQ_MIN);
		ddl_config.push_back(DD_FREQ_MAX);
		ddl_config.push_back(DD_FREQ_STEP);
		ddl_config.push_back(WDD_FRAME_RATE);
		ddl_config.push_back(FRAME_RED_FAC);
		ddl_config.push_back(DD_MIN_POTENTIAL);

		//
		// select DotDetectors according to Arena
		//

		// assert: Setup() before Run() -> arena configured, 640x480 -> 180x120
		for(int i=0; i<4; i++)
		{
			_CC.arena_lowRes[i].x = static_cast<int>(std::floor(_CC.arena[i].x * 0.25));
			_CC.arena_lowRes[i].y = static_cast<int>(std::floor(_CC.arena[i].y * 0.25));
		}

		std::vector<cv::Point2i> dd_positions;
		for(int i=0; i<frame_target_width; i++)
		{
			for(int j=0; j<frame_target_height; j++)
			{
				// x (width), y(height)
				cv::Point2i tmp(i,j);
				if(pointIsInArena(tmp))
					dd_positions.push_back(tmp);
			}
		}
		printf("Initialize with %d DotDetectors (DD_MIN_POTENTIAL=%.1f) (WDD_SIGNAL_MIN_CLUSTER_SIZE=%d).\n", 
			dd_positions.size(), DD_MIN_POTENTIAL, WDD_SIGNAL_MIN_CLUSTER_SIZE);

		//
		// prepare WaggleDanceDetector config vector
		//
		std::vector<double> wdd_config;
		// Layer 2
		wdd_config.push_back(WDD_SIGNAL_DD_MAXDISTANCE);
		wdd_config.push_back(WDD_SIGNAL_MIN_CLUSTER_SIZE);
		// Layer 3
		wdd_config.push_back(WDD_DANCE_MAX_POSITION_DISTANCEE);
		wdd_config.push_back(WDD_DANCE_MAX_FRAME_GAP);
		wdd_config.push_back(WDD_DANCE_MIN_CONSFRAMES);

		WaggleDanceDetector wdd(
			dd_positions,
			&frame_target,
			ddl_config,
			wdd_config,
			&videoFrameBuffer,
			_CC,
			wdd_write_signal_file,
			wdd_write_dance_file,
			wdd_verbose
			);

		WDD_p = &wdd;

		// Create camera instance
		std::cerr << ((_resolution==CLEYE_QVGA) ? "QVGA" : "Not Qvga") << std::endl;
		_cam = CLEyeCreateCamera(_cameraGUID, _mode, _resolution, _fps);
		if(_cam == NULL)
		{
			std::cerr<<"ERROR! Could not create camera instance!"<<std::endl;
			return;
		}
		if(CLEyeSetCameraParameter(_cam, CLEYE_AUTO_GAIN, true) == false)
			std::cerr<<"WARNING! Could not set CLEYE_AUTO_GAIN = true!"<<std::endl;

		if(CLEyeSetCameraParameter(_cam, CLEYE_AUTO_EXPOSURE, true) == false)
			std::cerr<<"WARNING! Could not set CLEYE_AUTO_EXPOSURE = true!"<<std::endl;

		if(CLEyeSetCameraParameter(_cam, CLEYE_AUTO_WHITEBALANCE, true) == false)
			std::cerr<<"WARNING! Could not set CLEYE_AUTO_WHITEBALANCE = true!"<<std::endl;
		//CLEyeSetCameraParameter(_cam, CLEYE_EXPOSURE, 5);
		PBYTE pCapBuffer = NULL;
		// create the appropriate OpenCV image
		IplImage *pCapImage = cvCreateImage(cvSize(_FRAME_WIDTH, _FRAME_HEIGHT), IPL_DEPTH_8U, 1);
		IplImage frame_target_bc = frame_target;

		// Start capturing
		CLEyeCameraStart(_cam);

		//
		// WARMUP LOOP
		//
		printf("Start camera warmup..\n");
		bool WARMUP_DONE = false;
		unsigned int WARMUP_FPS_HIT = 0;
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
		while(_running)
		{
			cvGetImageRawData(pCapImage, &pCapBuffer);

			CLEyeCameraGetFrame(_cam, pCapBuffer);

			frame_input_monochrome = cv::Mat(pCapImage, true);

			// save to global Frame Buffer
			videoFrameBuffer.addFrame(&frame_input_monochrome);

			// subsample
			cv::resize(frame_input_monochrome, frame_target, frame_target.size(),
					0, 0, cv::INTER_AREA);

			// 
			if(_visual)
			{
				drawArena(cv::Mat(&frame_target_bc));
				cvShowImage(_windowName, &frame_target_bc);
			}

			if(!WARMUP_DONE)
			{
				wdd.copyInitialFrame(frame_counter_global, false);
			}
			else
			{
				// save number of frames needed for camera warmup
				frame_counter_warmup = frame_counter_global;
				wdd.copyInitialFrame(frame_counter_global, true);
				break;
			}
			// finally increase frame_input counter	
			frame_counter_global++;

			//test fps
			if((frame_counter_global % 100) == 0)
			{
				std::chrono::duration<double> sec = std::chrono::steady_clock::now() - start;
				double fps = 100/sec.count();
				printf("%1.f fps ..", fps);
				if(abs(WDD_FRAME_RATE - fps) < 1)
				{
					printf("\t [GOOD]\n");
					WARMUP_DONE = ++WARMUP_FPS_HIT >= 3 ? true : false;
				}
				else
				{
					printf("\t [BAD]\n");
				}

				start = std::chrono::steady_clock::now();
			}
		}		
		printf("Camera warmup done!\n\n\n");

		//
		// MAIN LOOP
		//
		std::vector<double> bench_res;
		while(_running)
		{

			cvGetImageRawData(pCapImage, &pCapBuffer);

			CLEyeCameraGetFrame(_cam, pCapBuffer);

			frame_input_monochrome = cv::Mat(pCapImage, true);

			// save to global Frame Buffer
			videoFrameBuffer.addFrame(&frame_input_monochrome);

			// subsample
			cv::resize(frame_input_monochrome, frame_target, frame_target.size(),
				0, 0, cv::INTER_AREA);

			// feed WDD with tar_frame
			wdd.copyFrame(frame_counter_global);

			if(_visual)
			{
				cv::cvtColor(cv::Mat(&frame_target_bc), frame_visual, CV_GRAY2BGR);
				drawArena(frame_visual);
				drawPosDDs(frame_visual);
				frame_visual_ipl = frame_visual;
				cvShowImage(_windowName, &frame_visual_ipl);
			}

#ifdef WDD_DDL_DEBUG_FULL
			if(frame_counter_global >= WDD_FBUFFER_SIZE-1)
				printf("Frame# %llu\t DD_SIGNALS_NUMBER: %d\n", WaggleDanceDetector::WDD_SIGNAL_FRAME_NR, DotDetectorLayer::DD_SIGNALS_NUMBER);

			// check exit condition
			if((frame_counter_global-frame_counter_warmup) >= WDD_DDL_DEBUG_FULL_MAX_FRAME-1)
			{
				std::cout<<"************** WDD_DDL_DEBUG_FULL DONE! **************"<<std::endl;
				printf("WDD_DDL_DEBUG_FULL captured %d frames.\n", WDD_DDL_DEBUG_FULL_MAX_FRAME);
				_running = false;				
			}
#endif
			// finally increase frame_input counter	
			frame_counter_global++;
			// benchmark output
			if((frame_counter_global % 100) == 0)
			{
				std::chrono::duration<double> sec = std::chrono::steady_clock::now() - start;
				bench_res.push_back(100/sec.count());
				start = std::chrono::steady_clock::now();
			}

			if((frame_counter_global % 500) == 0)
			{
				time_t     now = time(0);
				struct tm  tstruct;
				char       buf[80];
				localtime_s(&tstruct, &now);
				// Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
				// for more information about date/time format
				strftime(buf, sizeof(buf), "%Y%m%d %X", &tstruct);
				std::cout << "["<< buf << "] collected fps: ";
				double avg = 0;
				for(auto it=bench_res.begin(); it!=bench_res.end(); ++it)
				{
					printf("%.1f ", *it);
					avg += *it;
				}
				printf("(avg: %.1f)\n", avg/bench_res.size());
				bench_res.clear();

				makeHeartBeatFile();
			}
		}
		//
		// release objects
		//

		cvReleaseImage(&pCapImage);
		// Stop camera capture
		CLEyeCameraStop(_cam);
		// Destroy camera object
		CLEyeDestroyCamera(_cam);
		// Destroy the allocated OpenCV image
		cvReleaseImage(&pCapImage);
		_cam = NULL;
	}
	DWORD WINAPI CLEyeCameraCapture::CaptureThread(LPVOID instance)
	{
		// seed the rng with current tick count and thread id
		srand(GetTickCount() + GetCurrentThreadId());
		// forward thread to Capture function
		CLEyeCameraCapture *pThis = (CLEyeCameraCapture *)instance;

		if(pThis->_setupModeOn)
			pThis->Setup();
		else
			pThis->Run();

		return 0;
	}
}