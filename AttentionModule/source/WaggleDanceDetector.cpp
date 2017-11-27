#include "stdafx.h"

#include "VideoFrameBuffer.h"
#include "DotDetectorLayer.h"
#include "WaggleDanceOrientator.h"
#include "WaggleDanceExport.h"

#include "WaggleDanceDetector.h"

namespace wdd
{
	unsigned long long WaggleDanceDetector::WDD_SIGNAL_FRAME_NR;
	int WaggleDanceDetector::WDD_VERBOSE;

	char * danceFile_path = "\\dance.txt";
	FILE * danceFile_ptr;
	char * signalFile_path = "\\signal.txt";
	FILE * signalFile_ptr;

	WaggleDanceDetector::WaggleDanceDetector(			
		std::vector<cv::Point2i> dd_positions,
		cv::Mat * frame_ptr,
		std::vector<double> ddl_config,
		std::vector<double> wdd_config,
		VideoFrameBuffer * videoFrameBuffer_ptr,
		CamConf auxCC,
		bool wdd_write_signal_file,
		bool wdd_write_dance_file,
		int wdd_verbose
		):
	WDD_VideoFrameBuffer_ptr(videoFrameBuffer_ptr),
		auxCC(auxCC),
		WDD_WRITE_DANCE_FILE(wdd_write_dance_file),
		WDD_WRITE_SIGNAL_FILE(wdd_write_signal_file),
		_startFrameShift(WDD_FBUFFER_SIZE),
		_endFrameShift(0)
	{	
		WaggleDanceDetector::WDD_VERBOSE = wdd_verbose;

		WaggleDanceExport::setArena(auxCC.arena);

		_initWDDSignalValues();

		_initWDDDanceValues();

		_initOutPutFiles();

		_setWDDConfig(wdd_config);

		DotDetectorLayer::init(dd_positions, frame_ptr, ddl_config);

		WDD_FBUFFER_POS = 0;
	}

	WaggleDanceDetector::~WaggleDanceDetector()
	{
		if(WDD_WRITE_DANCE_FILE)
			fclose (danceFile_ptr);

		if(WDD_WRITE_SIGNAL_FILE)
			fclose (signalFile_ptr);
	}
	/*
	* Dependencies: NULL
	*/
	void WaggleDanceDetector::_initWDDSignalValues()
	{
		WDD_SIGNAL = false;
		WDD_SIGNAL_NUMBER = 0;
	}
	/*
	* Dependencies: NULL
	*/
	void WaggleDanceDetector::_initWDDDanceValues()
	{
		WDD_DANCE = false;
		WDD_DANCE_ID = 0;
		WDD_DANCE_NUMBER = 0;
	}
	/*
	* Dependencies: NULL
	*/
	void WaggleDanceDetector::_initOutPutFiles()
	{		
		if(WDD_WRITE_DANCE_FILE || WDD_WRITE_SIGNAL_FILE)
		{
			// link full path from main.cpp
			extern char _FULL_PATH_EXE[MAX_PATH];
			extern std::string GLOB_WDD_DANCE_OUTPUT_PATH;
			char BUFF[MAX_PATH];

			if(WDD_WRITE_DANCE_FILE)
			{
				//strcpy_s(BUFF ,MAX_PATH, _FULL_PATH_EXE);
				//strcat_s(BUFF, MAX_PATH, danceFile_path);
				fopen_s (&danceFile_ptr, GLOB_WDD_DANCE_OUTPUT_PATH.c_str(), "a+");
			}
			if(WDD_WRITE_SIGNAL_FILE)
			{
				strcpy_s(BUFF ,MAX_PATH, _FULL_PATH_EXE);
				strcat_s(BUFF, MAX_PATH, signalFile_path);
				fopen_s (&signalFile_ptr, BUFF, "a+");
			}
		}
	}
	void WaggleDanceDetector::_setWDDConfig(std::vector<double> wdd_config)
	{
		if(wdd_config.size() != 5)
		{
			std::cerr<<"Error! WaggleDanceDetector::_setWDDConfig: wrong number of config arguments!"<<std::endl;
			exit(-19);
		}

		/* copy values from ddl_config*/
		// Layer 2
		double wdd_signal_dd_maxdistance = wdd_config[0];
		double wdd_signal_min_cluster_size = wdd_config[1];
		// Layer 3
		double wdd_dance_max_position_distance = wdd_config[2];
		double wdd_dance_max_frame_gap = wdd_config[3];
		double wdd_dance_min_consframes = wdd_config[4];

		// do some sanity checks
		if(wdd_signal_dd_maxdistance < 0)
		{
			std::cerr<<"Error! WaggleDanceDetector::_setWDDConfig: wdd_signal_max_cluster_dd_distance < 0!"<<std::endl;
			exit(-20);
		}
		if(wdd_signal_min_cluster_size < 0)
		{
			std::cerr<<"Error! WaggleDanceDetector::_setWDDConfig: wdd_signal_min_cluster_size < 0!"<<std::endl;
			exit(-20);
		}
		if(wdd_dance_max_position_distance < 0)
		{
			std::cerr<<"Error! WaggleDanceDetector::_setWDDConfig: wdd_dance_max_position_distance < 0!"<<std::endl;
			exit(-21);
		}
		if(wdd_dance_max_frame_gap < 0)
		{
			std::cerr<<"Error! WaggleDanceDetector::_setWDDConfig: wdd_dance_max_frame_gap < 0!"<<std::endl;
			exit(-22);
		}
		if(wdd_dance_min_consframes < 0)
		{
			std::cerr<<"Error! WaggleDanceDetector::_setWDDConfig: wdd_dance_min_consframes < 0!"<<std::endl;
			exit(-23);
		}


		WDD_SIGNAL_DD_MAXDISTANCE = wdd_signal_dd_maxdistance;
		WDD_SIGNAL_DD_MIN_CLUSTER_SIZE = static_cast<std::size_t>(wdd_signal_min_cluster_size);

		WDD_DANCE_MAX_POS_DIST = wdd_dance_max_position_distance;
		WDD_DANCE_MAX_FRAME_GAP = static_cast<std::size_t>(wdd_dance_max_frame_gap);
		WDD_DANCE_MIN_CONSFRAMES = static_cast<std::size_t>(wdd_dance_min_consframes);

		if(WDD_VERBOSE)
			printWDDDanceConfig();
	}

	/*
	* expect the frame to be in target format, i.e. resolution and grayscale,
	* enhanced etc.
	* dont run detection yet
	*/
	void WaggleDanceDetector::copyInitialFrame(unsigned long long frame_nr, bool doDetection)
	{
		WaggleDanceDetector::WDD_SIGNAL_FRAME_NR = frame_nr;
		DotDetectorLayer::copyFrame(doDetection);		
	}
	/*
	* expect the frame to be in target format, i.e. resolution and grayscale,
	* enhanced etc.
	*/
	void WaggleDanceDetector::copyFrame(unsigned long long frame_nr)
	{	
		WaggleDanceDetector::WDD_SIGNAL_FRAME_NR = frame_nr;
		_execDetection();
	}

	void WaggleDanceDetector::_execDetection()
	{
		//
		// Layer 1
		//
		DotDetectorLayer::copyFrameAndDetect();
		if(DotDetectorLayer::DD_SIGNALS_NUMBER > WDD_LAYER2_MAX_POS_DDS)
		{
			printf("[WARNING] WDD LAYER 1 OVERFLOW! Drop frame %llu (DD_SIGNALS_NUMBER: %d)\n",
				WaggleDanceDetector::WDD_SIGNAL_FRAME_NR,
				DotDetectorLayer::DD_SIGNALS_NUMBER);
			return;
		}

		//
		// Layer 2
		//

		// run detection on WaggleDanceDetector layer
		_execDetectionGetWDDSignals();

		//
		// Layer 3
		//

		// run top level detection, concat over time
		if(WDD_SIGNAL)
		{
			_execDetectionConcatWDDSignals();
		}

		_execDetectionHousekeepWDDSignals();

	}
	void WaggleDanceDetector::_execDetectionConcatWDDSignals()
	{
		// preallocate 
		double dist;
		DANCE d;		

		bool danceFound;
		cv::Point2d wdd_signal_pos;
		std::vector<cv::Point2d> * dance_position_ptr;

		// foreach WDD_SIGNAL_ID2POINT_MAP
		for(std::size_t i=0; i < WDD_SIGNAL_ID2POINT_MAP.size(); i++)
		{
			danceFound = false; wdd_signal_pos = WDD_SIGNAL_ID2POINT_MAP[i];

			// check distance against known Dances
			for (auto it_dances=WDD_UNIQ_DANCES.begin();it_dances!=WDD_UNIQ_DANCES.end(); ++it_dances)
			{
				dance_position_ptr = &(it_dances->positions);

				// ..and their associated points
				for(auto it_dancepositions= dance_position_ptr->begin(); it_dancepositions!=dance_position_ptr->end(); ++it_dancepositions)
				{
					dist = cv::norm(wdd_signal_pos - *it_dancepositions);

					// wdd signal belongs to known dance
					if( dist < WDD_DANCE_MAX_POS_DIST)
					{
						// set flag i.o. to leave loops safely
						danceFound = true;						

						// check if position is linear or there is a gap
						// if it is a consecutive detection, then frame_gap == 1
						// else there is a gap inbetween layer 2 detections, therefore fill positions
						unsigned int frame_gap = static_cast<unsigned int>(WDD_SIGNAL_FRAME_NR - it_dances->DANCE_FRAME_END);
						while(frame_gap > 1)
						{
							it_dances->positions.push_back(cv::Point2d(-1.0,-1.0));
							frame_gap--;
						}

						// add current position - assertion: possible gap filled
						it_dances->positions.push_back(wdd_signal_pos);

						// save current position as last position;
						it_dances->position_last = wdd_signal_pos;

						// save current frame as last frame
						it_dances->DANCE_FRAME_END = WDD_SIGNAL_FRAME_NR;

						if(WDD_VERBOSE>1)
							std::cout<<WDD_SIGNAL_FRAME_NR<<" - UPD: "<<it_dances->DANCE_UNIQE_ID
								<<" ["<<it_dances->DANCE_FRAME_START<<","<<it_dances->DANCE_FRAME_END<<"]\n";

						// jump to WDD_SIGNAL_ID2POINT_MAP loop
						break;break;
					}
				}
			}

			if(!danceFound)
			{
				// if reached here, new DANCE encountered!
				d.DANCE_UNIQE_ID = WDD_DANCE_ID++;
				d.DANCE_FRAME_START = WDD_SIGNAL_FRAME_NR;
				d.DANCE_FRAME_END =  WDD_SIGNAL_FRAME_NR;
				d.positions.push_back(wdd_signal_pos);
				d.position_last = wdd_signal_pos;
				GetLocalTime(&d.rawtime);
				WDD_UNIQ_DANCES.push_back(d);
			}			
		}
	}
	void WaggleDanceDetector::_execDetectionHousekeepWDDSignals()
	{
		//house keeping of Dance Signals
		for (auto it_dances=WDD_UNIQ_DANCES.begin();it_dances!=WDD_UNIQ_DANCES.end(); )
		{
			// if Dance did not recieve new signal for over WDD_UNIQ_SIGID_MAX_GAP frames
			if((WDD_SIGNAL_FRAME_NR - it_dances->DANCE_FRAME_END) > WDD_DANCE_MAX_FRAME_GAP)
			{
				// check for minimum number of consecutiv frames for a positive dance 
				// and VFB_MAX_FRAME_HISTORY boundary
				if(((it_dances->DANCE_FRAME_END - it_dances->DANCE_FRAME_START) > WDD_DANCE_MIN_CONSFRAMES) &&
					((it_dances->DANCE_FRAME_END - it_dances->DANCE_FRAME_START) < VFB_MAX_FRAME_HISTORY))
				{
					if(WDD_VERBOSE>1)
						std::cout<<WDD_SIGNAL_FRAME_NR<<" - EXEC: "<<(*it_dances).DANCE_UNIQE_ID<<" ["<<(*it_dances).DANCE_FRAME_START<<","<<(*it_dances).DANCE_FRAME_END<<"]"<<std::endl;

					_execDetectionFinalizeDance(&(*it_dances));
				}
				if(WDD_VERBOSE>1)
					std::cout<<WDD_SIGNAL_FRAME_NR<<" - DEL: "<<(*it_dances).DANCE_UNIQE_ID<<" ["<<(*it_dances).DANCE_FRAME_START<<","<<(*it_dances).DANCE_FRAME_END<<"]"<<std::endl;

				it_dances = WDD_UNIQ_DANCES.erase(it_dances);
			}
			else
			{
				++it_dances;
			}
		}
	}
	void WaggleDanceDetector::_execDetectionFinalizeDance(DANCE * d_ptr)
	{
		// set flag
		WDD_DANCE = true;
		
#ifdef WDD_EXTRACT_ORIENT
		// retrieve original frames of detected dance
		std::vector<cv::Mat> seq = WDD_VideoFrameBuffer_ptr->loadCroppedFrameSequenc(
			d_ptr->DANCE_FRAME_START - _startFrameShift,
			d_ptr->DANCE_FRAME_END - _endFrameShift, 
			cv::Point_<int>(d_ptr->positions[0]), 
			DotDetectorLayer::FRAME_REDFAC);

		//if number of cropped frames zero, just drop the detection cause something went terribly wrong
		// regarding the frame numbers
		if(seq.empty())
			return;

		d_ptr->orient_uvec = WaggleDanceOrientator::extractOrientationFromPositions(d_ptr->positions, d_ptr->position_last);
		d_ptr->naive_orientation = WaggleDanceOrientator::extractNaiveness (d_ptr->positions, d_ptr->position_last);
#else
		d_ptr->orient_uvec = cv::Point2i(0,0);
#endif

		WDD_UNIQ_FINISH_DANCES.push_back(*d_ptr);
		if(WDD_WRITE_DANCE_FILE)
			_execDetectionWriteDanceFileLine(d_ptr);

		if(WDD_VERBOSE)
		{
			extern double uvecToDegree(cv::Point2d in);
			printf("Waggle dance #%d at:\t %.1f %.1f with orient %.1f (uvec: %.1f,%.1f)\n",
				WDD_DANCE_NUMBER,
				d_ptr->positions[0].x*pow(2, DotDetectorLayer::FRAME_REDFAC),
				d_ptr->positions[0].y*pow(2, DotDetectorLayer::FRAME_REDFAC),
				uvecToDegree(d_ptr->orient_uvec),
				d_ptr->orient_uvec.x,
				d_ptr->orient_uvec.y);
		}

		WaggleDanceExport::write(seq, d_ptr, auxCC.camId);

		WDD_DANCE_NUMBER++;
	}
	/*	
	*/
	void WaggleDanceDetector::_execDetectionGetDDPotentials()
	{

		//std::cout<<"DotDetectorLayer::DD_SIGNALS_NUMBER: "<<DotDetectorLayer::DD_SIGNALS_NUMBER<<std::endl;
	}

	void WaggleDanceDetector::_execDetectionGetWDDSignals()
	{
		// reset GetWDDSignals relevant fields
		WDD_SIGNAL = false;
		WDD_SIGNAL_NUMBER = 0;
		WDD_SIGNAL_ID2POINT_MAP.clear();

		// also reset DANCE FLAG
		WDD_DANCE = false;

		// test for minimum number of potent DotDetectors
		if(DotDetectorLayer::DD_SIGNALS_NUMBER < WDD_SIGNAL_DD_MIN_CLUSTER_SIZE)
			return;

		// get ids (=index in vector) of positive DDs
		// WARNING! Heavly rely on fact that :
		// - DD_ids are [0; DD_POSITIONS_NUMBER-1]
		arma::Col<arma::uword> pos_DD_IDs(DotDetectorLayer::DD_SIGNALS_IDs);

		// init cluster ids for the positive DDs
		arma::Col<arma::sword> pos_DD_CIDs(pos_DD_IDs.size());
		pos_DD_CIDs.fill(-1);
		// init working copy of positive DD ids
		//std::vector<arma::uword> pos_DD_IDs_wset (pos_DD_IDs.begin(), pos_DD_IDs.end());

		// init unique cluster ID 
		unsigned int clusterID = 0;
		// foreach positive DD
		for(std::size_t i=0; i < pos_DD_IDs.size(); i++)
		{
			// check if DD is missing a cluster ID
			if(pos_DD_CIDs.at(i) >= 0)
				continue;

			// assign unique cluster id
			pos_DD_CIDs.at(i) = clusterID++;

			// assign source id (root id)
			arma::Col<arma::uword> root_DD_id(std::vector<arma::uword>(1,pos_DD_IDs.at(i)));

			// select only unclustered DD as working set
			arma::Col<arma::uword> pos_DD_unclustered_idx = arma::find(pos_DD_CIDs == -1);

			// make a local copy of positive ids from working set
			arma::Col<arma::uword> loc_pos_DD_IDs = pos_DD_IDs.rows(pos_DD_unclustered_idx);

			// init loop
			arma::Col<arma::uword> neighbour_DD_ids = 
				getNeighbours(root_DD_id, arma::Col<arma::uword>(), loc_pos_DD_IDs);

			arma::Col<arma::uword> Lneighbour_DD_ids;

			// loop until no new neighbours are added
			while(Lneighbour_DD_ids.size() != neighbour_DD_ids.size())
			{
				// get new discovered elements as
				// {D} = {N_i} \ {N_i-1}
				arma::Col<arma::uword> D = neighbour_DD_ids;
				for(std::size_t j=0; j<Lneighbour_DD_ids.size(); j++)
				{
					//MATLAB:  D(D==m) = [];
					arma::uvec q1 = arma::find(D == Lneighbour_DD_ids.at(j), 1);
					if(q1.size() == 1)
						D.shed_row(q1.at(0));
				}

				// save last neighbours id list
				Lneighbour_DD_ids = neighbour_DD_ids;

				// remove discovered elements from search
				for(std::size_t j=0; j<neighbour_DD_ids.size(); j++)
				{
					//MATLAB: loc_pos_DD_IDs(loc_pos_DD_IDs==m) = [];
					arma::uvec q1 = arma::find(loc_pos_DD_IDs == neighbour_DD_ids.at(j), 1);
					if(q1.size() == 1)
						loc_pos_DD_IDs.shed_row(q1.at(0));
				}

				neighbour_DD_ids = getNeighbours(D, neighbour_DD_ids, loc_pos_DD_IDs);
			}

			// set CIDs of all neighbours
			for(std::size_t j=0; j<neighbour_DD_ids.size(); j++)
			{
				arma::uvec q1 = arma::find(pos_DD_IDs == neighbour_DD_ids.at(j), 1);

				if(q1.size() == 1)
					pos_DD_CIDs.at(q1.at(0)) = pos_DD_CIDs.at(i);
				else
				{
					printf("ERROR! ::execDetectionGetWDDSignals: Unexpected assertion failure!\n");
					exit(19);
				}
			}
		}
		// assertion test
		arma::uvec q1 = arma::find(pos_DD_CIDs == -1);
		if(q1.size() > 0)
		{
			printf("ERROR! ::execDetectionGetWDDSignals: Unclassified DD signals!\n");
			exit(19);
		}

		// analyze cluster sizes
		arma::uvec count_unqClusterIDs(clusterID);
		count_unqClusterIDs.fill(0);

		// get size of each cluster
		for(std::size_t i=0; i<pos_DD_CIDs.size(); i++)
		{
			count_unqClusterIDs.at(pos_DD_CIDs.at(i))++;
		}

		arma::uvec f_unqClusterIDs;
		for(std::size_t i=0; i<count_unqClusterIDs.size(); i++)
		{
			if(count_unqClusterIDs.at(i) >= WDD_SIGNAL_DD_MIN_CLUSTER_SIZE)
			{
				f_unqClusterIDs.insert_rows(f_unqClusterIDs.size(),1);
				f_unqClusterIDs.at(f_unqClusterIDs.size()-1) = i;
			}
		}
		// decide if there is at least one WDD Signal
		if(f_unqClusterIDs.is_empty())
			return;

		WDD_SIGNAL = true;

		// foreach remaining cluster calculate center position
		for(std::size_t i=0; i<f_unqClusterIDs.size(); i++)
		{
			// find vecotr positions in pos_DD_CIDS <=> pos_DD_IDs 
			// associated with cluster id
			arma::uvec idx = arma::find(pos_DD_CIDs == f_unqClusterIDs.at(i));

			cv::Point2d center(0,0);
			for(std::size_t j=0; j<idx.size(); j++)
			{
				center += static_cast<cv::Point_<double>>(DotDetectorLayer::DD_POSITIONS[pos_DD_IDs.at(idx.at(j))]);
			}

			center *= 1/static_cast<double>(idx.size());

			WDD_SIGNAL_ID2POINT_MAP[WDD_SIGNAL_NUMBER++] = center;
		}
		// toggel signal file creation
		if(WDD_WRITE_SIGNAL_FILE)
		{
			_execDetectionWriteSignalFileLine();
		}

		//std::cout<<"WDD_SIGNAL_NUMBER: "<<WDD_SIGNAL_NUMBER<<std::endl;
	}

	arma::Col<arma::uword> WaggleDanceDetector::getNeighbours(
		arma::Col<arma::uword> sourceIDs, arma::Col<arma::uword> N, arma::Col<arma::uword> set_DD_IDs)
	{
		// anchor case
		if(sourceIDs.size() == 1)
		{
			cv::Point2i DD_pos = DotDetectorLayer::DD_POSITIONS[sourceIDs.at(0)];

			for(std::size_t i=0; i< set_DD_IDs.size(); i++)
			{
				cv::Point2i DD_pos_other = DotDetectorLayer::DD_POSITIONS[set_DD_IDs.at(i)];

				// if others DotDetectors distance is in bound, add its ID
				if(std::sqrt(cv::norm(DD_pos-DD_pos_other)) < WDD_SIGNAL_DD_MAXDISTANCE)
				{
					N.insert_rows(N.size(), 1);
					N.at(N.size()-1) = set_DD_IDs.at(i);
				}
			}
		}
		// first recursive level call
		else if(sourceIDs.size() > 1)
		{
			for(std::size_t i=0; i< sourceIDs.size(); i++)
			{
				// remove discoverd neighbours from set
				for(std::size_t j=0; j< N.size(); j++)
				{
					arma::uvec q1 = arma::find(set_DD_IDs == N.at(j), 1);
					if(q1.size() == 1)
						set_DD_IDs.shed_row(q1.at(0));
					else if(q1.size() > 1)
					{
						printf("ERROR! ::getNeighbours: Unexpected number of DD Ids found!:\n");
						exit(19);
					}
				}

				// check if some ids are left
				if(set_DD_IDs.is_empty())
					break;

				arma::Col<arma::uword> _sourceIDs;
				_sourceIDs.insert_rows(0,1);
				_sourceIDs.at(0) = sourceIDs.at(i);

				N = getNeighbours(_sourceIDs, N, set_DD_IDs);
			}
		}
		else
		{
			printf("ERROR! ::getNeighbours: Unexpected assertion failure!\n");
			exit(19);
		}

		return N;
	}
	void WaggleDanceDetector::_execDetectionWriteDanceFileLine(DANCE * d_ptr)
	{
		extern double uvecToDegree(cv::Point2d in);

		int start = d_ptr->DANCE_FRAME_START;
		int end = d_ptr->DANCE_FRAME_END;

		unsigned long long numFrames = end-start+1;

		if(numFrames == d_ptr->positions.size())
		{
			auto it = d_ptr->positions.begin();
			while(start<=end)
			{
				fprintf(danceFile_ptr, "%d %.2f %.2f %.1f\n", start, 
					it->x*pow(2, DotDetectorLayer::FRAME_REDFAC), 
					it->y*pow(2, DotDetectorLayer::FRAME_REDFAC),
					uvecToDegree(d_ptr->orient_uvec));
				start++;
				++it;
			}
		}
		else
		{
			printf("Warning! Dance Id %d (%d - %d) has corrupted position size: %d (not matching numFrames: %d)\n",
				d_ptr->DANCE_UNIQE_ID, start, end, d_ptr->positions.size(), numFrames);

		}
	}

	void WaggleDanceDetector::_execDetectionWriteSignalFileLine()
	{
		fprintf(signalFile_ptr, "%I64u", WDD_SIGNAL_FRAME_NR);
		for (auto it=WDD_SIGNAL_ID2POINT_MAP.begin(); it!=WDD_SIGNAL_ID2POINT_MAP.end(); ++it)
		{
			fprintf(signalFile_ptr, " %.5f %.5f",
				it->second.x*pow(2, DotDetectorLayer::FRAME_REDFAC), it->second.y*pow(2, DotDetectorLayer::FRAME_REDFAC));
		}
		fprintf(signalFile_ptr, "\n");
	}
	bool WaggleDanceDetector::isWDDSignal()
	{
		return WDD_SIGNAL;
	}
	bool WaggleDanceDetector::isWDDDance()
	{
		return WDD_DANCE;
	}
	std::size_t WaggleDanceDetector::getWDDSignalNumber()
	{
		return WDD_SIGNAL_NUMBER;
	}
	const std::map<std::size_t,cv::Point2d> * WaggleDanceDetector::getWDDSignalId2PointMap()
	{
		return &WDD_SIGNAL_ID2POINT_MAP;
	}
	const std::vector<DANCE> * WaggleDanceDetector::getWDDFinishedDancesVec()
	{
		return &WDD_UNIQ_FINISH_DANCES;
	}
	void WaggleDanceDetector::printWDDDanceConfig()
	{
		printf("Printing WDD signal configuration:\n");
		printf("[WDD_DANCE_MAX_POS_DIST] %.1f\n", WDD_DANCE_MAX_POS_DIST);
		printf("[WDD_DANCE_MAX_FRAME_GAP] %ul\n", WDD_DANCE_MAX_FRAME_GAP);
		printf("[WDD_DANCE_MIN_CONSFRAMES] %ul\n", WDD_DANCE_MIN_CONSFRAMES);
	}
	std::size_t WaggleDanceDetector::getWDD_SIGNAL_DD_MIN_CLUSTER_SIZE()
	{
		return WDD_SIGNAL_DD_MIN_CLUSTER_SIZE;
	}
	void WaggleDanceDetector::setWDD_SIGNAL_DD_MIN_CLUSTER_SIZE(std::size_t val)
	{
		if(val > 0)
			WDD_SIGNAL_DD_MIN_CLUSTER_SIZE = val;
	}
} /* namespace WaggleDanceDetector */