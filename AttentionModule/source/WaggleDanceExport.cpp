#include "stdafx.h"
#include "WaggleDanceExport.h"
#include "WaggleDanceOrientator.h"

namespace wdd
{
	char root_path[] = "\\output";
	char root_fullpath[MAX_PATH];

	// save the incrementing IDs of detection per directory
	std::size_t ID = 0;

	bool root_exist_chk = false;

	// save the current day
	char buf_YYYYMMDD[MAX_PATH];
	// save the current hour:minute and CamID 0-9
	char buf_YYYYMMDD_HHMM_camID[MAX_PATH];
	char relpath_YYYYMMDD_HHMM_camID[MAX_PATH];
	char buf_camID[32];
	char buf_dirID[32];

	std::array<cv::Point2i,4> auxArena;

	void WaggleDanceExport::write(const std::vector<cv::Mat> seq, const DANCE * d_ptr, std::size_t camID)
	{
		char _buf[MAX_PATH];

		//
		// check <YYYYYMMDD> folder
		//
		// write YYYYYMMDD string & compare to last saved		
		sprintf_s(_buf, MAX_PATH, "%04d%02d%02d", d_ptr->rawtime.wYear, d_ptr->rawtime.wMonth, d_ptr->rawtime.wDay);

		if(strcmp(_buf,buf_YYYYMMDD) != 0)
		{
			// if different, save it and ensure directory exists
			strcpy_s(buf_YYYYMMDD, sizeof(buf_YYYYMMDD), _buf);
			WaggleDanceExport::createGenericFolder(buf_YYYYMMDD);
		}

		//
		// check <YYYYYMMDD_HHMM_CamID> folder
		//
		// write YYYYYMMDD_HHMM_ string
		sprintf_s(_buf, MAX_PATH, "%04d%02d%02d_%02d%02d_", 
			d_ptr->rawtime.wYear, d_ptr->rawtime.wMonth, d_ptr->rawtime.wDay,
			d_ptr->rawtime.wHour, d_ptr->rawtime.wMinute
			);

		// convert camID to string
		_itoa_s(camID, buf_camID, sizeof(buf_camID), 10);
		// append and finalize YYYYYMMDD_HHMM_camID string
		strcat_s(_buf,buf_camID);

		if(strcmp(_buf,buf_YYYYMMDD_HHMM_camID) != 0)
		{
			// if different, save it and ensure directory exists
			strcpy_s(buf_YYYYMMDD_HHMM_camID, sizeof(buf_YYYYMMDD_HHMM_camID), _buf);

			// build path
			strcpy_s(_buf, buf_YYYYMMDD);
			strcat_s(_buf, "\\");
			strcat_s(_buf, buf_YYYYMMDD_HHMM_camID);
			WaggleDanceExport::createGenericFolder(_buf);

			//save relative path
			strcpy_s(relpath_YYYYMMDD_HHMM_camID, _buf);
		}

		//
		// check <ID> folder
		//
		// get dir <ID>
		std::size_t dirID = countDirectories(relpath_YYYYMMDD_HHMM_camID);

		// convert camID to string
		_itoa_s(dirID, buf_dirID, sizeof(buf_dirID), 10);

		strcpy_s(_buf, relpath_YYYYMMDD_HHMM_camID);
		strcat_s(_buf, "\\");
		strcat_s(_buf, buf_dirID);

		WaggleDanceExport::createGenericFolder(_buf);



		//
		// write CSV file
		//
		// link full path from main.cpp
		extern char _FULL_PATH_EXE[MAX_PATH];

		char CSV_FILE[MAX_PATH];

		strcpy_s(CSV_FILE, MAX_PATH, _FULL_PATH_EXE);
		strcat_s(CSV_FILE, root_path);
		strcat_s(CSV_FILE, "\\");

		strcat_s(CSV_FILE, MAX_PATH, _buf);
		strcat_s(CSV_FILE, MAX_PATH, "\\");
		strcat_s(CSV_FILE, buf_YYYYMMDD_HHMM_camID);
		strcat_s(CSV_FILE, "_");
		strcat_s(CSV_FILE, buf_dirID);
		strcat_s(CSV_FILE, ".csv");


		puts(CSV_FILE);

		FILE * CSV_ptr;
		fopen_s(&CSV_ptr, CSV_FILE,  "w");

		char TIMESTMP[32];		
		sprintf_s(TIMESTMP, 32, "%02d:%02d:%02d:%03d", 
			d_ptr->rawtime.wHour, d_ptr->rawtime.wMinute, d_ptr->rawtime.wSecond, d_ptr->rawtime.wMilliseconds 
			);

		fprintf_s(CSV_ptr,"%.1f %.1f %.1f\n", d_ptr->positions[0].x, d_ptr->positions[0].y, uvecToRad(d_ptr->orient_uvec));
		fprintf_s(CSV_ptr,"%s %d\n", TIMESTMP, static_cast<int>(d_ptr->DANCE_FRAME_END-d_ptr->DANCE_FRAME_START+1));

		for(unsigned i=0; i< 4; i++)
		{
			fprintf_s(CSV_ptr,"%d %d ", auxArena[i].x, auxArena[i].y);
		}
		fprintf_s(CSV_ptr,"\n");

		for(auto it = d_ptr->positions.begin(); it!=d_ptr->positions.end(); ++it)
		{
			fprintf_s(CSV_ptr,"%.1f %.1f ", it->x, it->y);
		}
		fprintf_s(CSV_ptr,"\n");
		fclose(CSV_ptr);

		//
		// write images
		//
		// get image dimensions
		cv::Size size = seq[0].size();

		// get direction length
		double length = MIN(size.width/2, size.height/2) * 0.8;

		// get image center point
		cv::Point2d CENTER(size.width/2., (size.height/2.));

		// get image orientation point
		cv::Point2d HEADIN(CENTER);
		HEADIN += d_ptr->orient_uvec*length;

		// preallocate 3 channel image output buffer
		cv::Mat image_out(size, CV_8UC3);

		// set image file name buffer
		char BUFF_PATH[MAX_PATH];
		char BUFF_UID[MAX_PATH];

		std::size_t i=0;
		for (auto it=seq.begin(); it!=seq.end(); ++it)
		{
			// convert & copy input image into BGR
			cv::cvtColor(*it, image_out, CV_GRAY2BGR);

			// create dynamic path_out string
			strcpy_s(BUFF_PATH, MAX_PATH, _FULL_PATH_EXE);
			strcat_s(BUFF_PATH, root_path);
			strcat_s(BUFF_PATH, "\\");
			strcat_s(BUFF_PATH, MAX_PATH, _buf);
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
		strcpy_s(BUFF_PATH, MAX_PATH, _FULL_PATH_EXE);
		strcat_s(BUFF_PATH, root_path);
		strcat_s(BUFF_PATH, "\\");
		strcat_s(BUFF_PATH, MAX_PATH, _buf);
		strcat_s(BUFF_PATH, MAX_PATH, "\\orient.png");

		WaggleDanceOrientator::saveImage(&image_out, BUFF_PATH);
	}

	double WaggleDanceExport::uvecToRad(cv::Point2d in)
	{
		if(_isnan(in.x) | _isnan(in.y))
			return std::numeric_limits<double>::quiet_NaN();

		return atan2(in.y,in.x);
	}
	void WaggleDanceExport::setArena(std::array<cv::Point2i,4> _auxArena){
		auxArena = _auxArena;
	}
	void WaggleDanceExport::execRootExistChk()
	{

		WaggleDanceExport::createGenericFolder("");
	}

	void WaggleDanceExport::createGenericFolder(char dir_rel[])
	{
		// link to help functionin main.cpp
		extern bool dirExists(const char * dirPath);
		// link full path from main.cpp
		extern char _FULL_PATH_EXE[MAX_PATH];

		char BUFF_PATH[MAX_PATH];

		// create path to .
		strcpy_s(BUFF_PATH, _FULL_PATH_EXE);
		strcat_s(BUFF_PATH, root_path);
		strcat_s(BUFF_PATH, "\\");
		strcat_s(BUFF_PATH, dir_rel);

		// check for path_out folder
		if(!dirExists(BUFF_PATH))
		{
			if(!CreateDirectory(BUFF_PATH, NULL))
			{
				printf("ERROR! Couldn't create %s directory.\n", BUFF_PATH);
				exit(-19);
			}
		}

	}

	int WaggleDanceExport::countDirectories(char dir_rel[])
	{
		// link to help functionin main.cpp
		extern bool dirExists(const char * dirPath);
		// link full path from main.cpp
		extern char _FULL_PATH_EXE[MAX_PATH];

		char BUFF_PATH[MAX_PATH];

		// create path to .
		strcpy_s(BUFF_PATH, _FULL_PATH_EXE);
		strcat_s(BUFF_PATH, root_path);
		strcat_s(BUFF_PATH, "\\");
		strcat_s(BUFF_PATH, dir_rel);

		WIN32_FIND_DATA ffd;
		HANDLE hFind = INVALID_HANDLE_VALUE;

		if (strlen(BUFF_PATH) > (MAX_PATH - 3))
		{
			printf("WARNING! Directory path is too long: %s\n", BUFF_PATH);
			return -1;
		}	

		strcat_s(BUFF_PATH, "\\*");
		hFind = FindFirstFile(BUFF_PATH, &ffd);

		if (INVALID_HANDLE_VALUE == hFind) 
		{
			printf("WARNING! INVALID_HANDLE_VALUE: %s\n", BUFF_PATH);
			return -1;
		}

		int count = 0;
		do {
			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				count++;
			}
		}   while (FindNextFile(hFind, &ffd) != 0);

		FindClose(hFind);

		// .. and . count as "2"		
		return count -2 ;
	}
}