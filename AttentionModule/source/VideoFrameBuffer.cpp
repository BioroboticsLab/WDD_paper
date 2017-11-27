#include "stdafx.h"
#include "VideoFrameBuffer.h"
#include "WaggleDanceOrientator.h"

namespace wdd
{
	char vfb_root_path[] = "\\fullframes";
	char vfb_root_fullpath[MAX_PATH];

	VideoFrameBuffer::VideoFrameBuffer(unsigned long long current_frame_nr, cv::Size cachedFrameSize, cv::Size extractFrameSize, CamConf _CC):
		_BUFFER_POS(0),
		_CURRENT_FRAME_NR(current_frame_nr),
		_cachedFrameSize(cachedFrameSize),
		_extractFrameSize(extractFrameSize),
		_CC(_CC),
		_last_hour(-1)
	{		
		if( (extractFrameSize.width > cachedFrameSize.width) || (extractFrameSize.height > cachedFrameSize.height))
		{
			std::cerr<< "Error! VideoFrameBuffer():: extractFrameSize can not be bigger then cachedFrameSize in any dimension!"<<std::endl;
		}
		_sequenceFramePointOffset = 
			cv::Point2i(extractFrameSize.width/2,extractFrameSize.height/2);

		//init memory
		for(std::size_t i=0; i<VFB_MAX_FRAME_HISTORY; i++)
		{
			_FRAME[i] = cv::Mat(cachedFrameSize, CV_8UC1);
		}
	}

	VideoFrameBuffer::~VideoFrameBuffer(void)
	{
	}	

	void VideoFrameBuffer::addFrame(cv:: Mat * frame_ptr)
	{		
		//std::cout<<"VideoFrameBuffer::addFrame::_BUFFER_POS: "<<_BUFFER_POS<<std::endl;
		_FRAME[_BUFFER_POS]= frame_ptr->clone();

		if(_CURRENT_FRAME_NR > 1000)
		{
			GetLocalTime(&st);

			int curent_hour = st.wHour;
			if((curent_hour > _last_hour) ||
				((curent_hour == 0) && (_last_hour == 23)))

			{
				_last_hour = curent_hour;
				saveFullFrame();
			}
		}
		_BUFFER_POS = (++_BUFFER_POS) < VFB_MAX_FRAME_HISTORY ? _BUFFER_POS : 0;

		_CURRENT_FRAME_NR++;

		// check for hourly save image


	}
	void VideoFrameBuffer::saveFullFrame()
	{
		// link to help functionin main.cpp
		extern bool dirExists(const char * dirPath);
		// link full path from main.cpp
		extern char _FULL_PATH_EXE[MAX_PATH];

		char BUFF_PATH[MAX_PATH];

		// create path to .
		strcpy_s(BUFF_PATH, _FULL_PATH_EXE);
		strcat_s(BUFF_PATH, vfb_root_path);

		// check for path_out folder
		if(!dirExists(BUFF_PATH))
		{
			if(!CreateDirectory(BUFF_PATH, NULL))
			{
				printf("ERROR! Couldn't create %s directory.\n", BUFF_PATH);
				exit(-19);
			}	
		}
		// save static full path
		strcpy_s(vfb_root_fullpath,BUFF_PATH);

		// create dynamic path_out with cam_id
		// convert camID to string
		char buf_camID[10];
		_itoa_s(_CC.camId, buf_camID, sizeof(buf_camID), 10);

		strcat_s(BUFF_PATH, MAX_PATH, "\\");
		strcat_s(BUFF_PATH, MAX_PATH, buf_camID);

		// check for path_out//id folder
		if(!dirExists(BUFF_PATH))
		{
			if(!CreateDirectory(BUFF_PATH, NULL))
			{
				printf("ERROR! Couldn't create %s directory.\n", BUFF_PATH);
				exit(-19);
			}	
		}

		// create dynamic path_out string
		char TIMESTMP[64];		
		sprintf_s(TIMESTMP, 64, "\\%04d%02d%02d_%02d%02d_", 
			st.wYear, st.wMonth, st.wDay,st.wHour, st.wMinute);

		strcat_s(BUFF_PATH, MAX_PATH, TIMESTMP);		
		strcat_s(BUFF_PATH, MAX_PATH, buf_camID);
		strcat_s(BUFF_PATH, MAX_PATH, ".png");

		cv::Mat _tmp = _FRAME[_BUFFER_POS].clone();

		cv::cvtColor(_tmp, _tmp, CV_GRAY2BGR);
		drawArena(_tmp);

		WaggleDanceOrientator::saveImage(&_tmp, BUFF_PATH);
	}
	void VideoFrameBuffer::drawArena(cv::Mat &frame)
	{
		float fac_red = 1.0/2.0;
		for (std::size_t i = 0; i < _CC.arena.size(); i++)
		{
			cv::line(frame, 
				cv::Point2f(_CC.arena[i].x *fac_red, _CC.arena[i].y *fac_red), 
				cv::Point2f(_CC.arena[(i+1) % _CC.arena.size()].x * fac_red, _CC.arena[(i+1) % _CC.arena.size()].y * fac_red), CV_RGB(0, 255, 0));
		}
	}
	cv::Mat * VideoFrameBuffer::getFrameByNumber(unsigned long long req_frame_nr)
	{
		int frameJump = static_cast<int>(_CURRENT_FRAME_NR - req_frame_nr);

		//std::cout<<"VideoFrameBuffer::getFrameByNumber::_CURRENT_FRAME_NR, req_frame_nr, frameJump: "<<_CURRENT_FRAME_NR<<", "<<req_frame_nr<<", "<<frameJump<<std::endl;

		// if req_frame_nr > CURRENT_FRAME_NR --> framesJump < 0
		if(frameJump <= 0)
		{
			std::cerr<< "Error! VideoFrameBuffer::getFrameByNumber can not retrieve frame from future "<<req_frame_nr<<" (CURRENT_FRAME_NR < req_frame_nr)!"<<std::endl;
			return NULL;
		}

		// assert: CURRENT_FRAME_NR >= req_frame_nr --> framesJump >= 0
		if( frameJump <= VFB_MAX_FRAME_HISTORY)
		{
			int framePosition = _BUFFER_POS - frameJump;

			framePosition = framePosition >= 0 ? framePosition : VFB_MAX_FRAME_HISTORY + framePosition;

			//std::cout<<"VideoFrameBuffer::getFrameByNumber::_BUFFER_POS: "<<framePosition<<std::endl;

			return &_FRAME[framePosition];
		}
		else
		{
			std::cerr<< "Error! VideoFrameBuffer::getFrameByNumber can not retrieve frame "<<req_frame_nr<<" - history too small!"<<std::endl;
			return NULL;
		}


	}

	std::vector<cv::Mat> VideoFrameBuffer::loadCroppedFrameSequenc(unsigned long long startFrame, unsigned long long endFrame, cv::Point2i center, double FRAME_REDFAC)
	{
		// fatal: if this is true, return empty vector
		if( startFrame >= endFrame)
			return std::vector<cv::Mat>();

		//std::cout<<"VideoFrameBuffer::loadFrameSequenc::startFrame, endFrame: "<<startFrame<<", "<<endFrame<<std::endl;
		//std::cout<<"VideoFrameBuffer::_cachedFrameSize.width, _cachedFrameSize.height: "<<_cachedFrameSize.width<<", "<<_cachedFrameSize.height<<std::endl;
		//std::cout<<"VideoFrameBuffer::_extractFrameSize.width, _extractFrameSize.height: "<<_extractFrameSize.width<<", "<<_extractFrameSize.height<<std::endl;

		const unsigned short _center_x = static_cast<int>(center.x*pow(2, FRAME_REDFAC));
		const unsigned short _center_y = static_cast<int>(center.y*pow(2, FRAME_REDFAC));

		int roi_rec_x = static_cast<int>(_center_x - _sequenceFramePointOffset.x);
		int roi_rec_y = static_cast<int>(_center_y - _sequenceFramePointOffset.y);

		// safe roi edge - lower bounds check
		roi_rec_x = roi_rec_x < 0 ? 0 : roi_rec_x;
		roi_rec_y = roi_rec_y < 0 ? 0 : roi_rec_y;

		// safe roi edge - upper bounds check
		roi_rec_x = (roi_rec_x + _extractFrameSize.width) <= _cachedFrameSize.width ? roi_rec_x : _cachedFrameSize.width - _extractFrameSize.width;
		roi_rec_y = (roi_rec_y + _extractFrameSize.height) <= _cachedFrameSize.height ? roi_rec_y : _cachedFrameSize.height - _extractFrameSize.height;


		// set roi
		cv::Rect roi_rec(cv::Point2i(roi_rec_x,roi_rec_y), _extractFrameSize);

		std::vector<cv::Mat> out;

		cv::Mat * frame_ptr;

		while(startFrame <=  endFrame)
		{
			frame_ptr = getFrameByNumber(startFrame);

			// check the pointer is not null and cv::Mat not empty
			if(frame_ptr && !frame_ptr->empty())
			{
				cv::Mat subframe_monochrome(*frame_ptr, roi_rec);
				out.push_back(subframe_monochrome.clone());
			}
			else
			{
				std::cerr << "Error! VideoFrameBuffer::loadFrameSequenc frame "<< startFrame<< " empty!"<<std::endl;
				return std::vector<cv::Mat>();
			}
			startFrame++;
		}

		return out;
	}
}