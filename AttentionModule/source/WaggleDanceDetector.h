#pragma once
#include "VideoFrameBuffer.h"

namespace wdd
{
	struct DANCE{
		unsigned long long DANCE_FRAME_START;
		unsigned long long DANCE_FRAME_END;

		std::size_t DANCE_UNIQE_ID;

		std::vector<cv::Point2d> positions;
		cv::Point2d position_last;

		cv::Point2d orient_uvec;
		cv::Point2d naive_orientation;

		SYSTEMTIME rawtime;
	};

	class WaggleDanceDetector
	{			
		std::size_t WDD_FBUFFER_POS;
		//
		// Layer 2
		//
		// saves total detection signal, either 0 or 1
		bool WDD_SIGNAL;

		// saves number of detection signals
		std::size_t WDD_SIGNAL_NUMBER;

		// saves positions of detection signals as map from dd_id -> Point2d
		std::map<std::size_t,cv::Point2d> WDD_SIGNAL_ID2POINT_MAP;

		// defines the maximum distance between two neighbor DD signals
		// TODO: calculate from bee size & DPI
		double WDD_SIGNAL_DD_MAXDISTANCE;

		// minimum number of positive DotDetector to start detection and
		// minimum size of clusters for positive detection and
		std::size_t WDD_SIGNAL_DD_MIN_CLUSTER_SIZE;

		//
		// Layer 3
		//
		// TODO: move post processing (dance image extraction & orientation
		// determination) in own class
		// indicates that a dance is completed
		bool WDD_DANCE;

		// unique dance id counter
		std::size_t WDD_DANCE_ID;

		// sequencial dance number counter
		std::size_t WDD_DANCE_NUMBER;

		// maps to show top level WDD signals (=dances)
		std::vector<DANCE> WDD_UNIQ_DANCES;

		// map to save ALL dances 
		std::vector<DANCE> WDD_UNIQ_FINISH_DANCES;

		// defines the maximum distance between two positions of same signal
		// TODO: calculate from bee size & DPI
		double 	WDD_DANCE_MAX_POS_DIST;

		// defines maximum gap of frames for a signal to connect
		std::size_t WDD_DANCE_MAX_FRAME_GAP;

		// defines minimum number of consecutive frames to form a final WD signal
		std::size_t WDD_DANCE_MIN_CONSFRAMES;


		//
		// aux pointer
		//
		// pointer to videoFrameBuffer (accessing history frames)
		VideoFrameBuffer * WDD_VideoFrameBuffer_ptr;

		int _startFrameShift;
		int _endFrameShift;

		//
		// develop options
		//
		// if set to true, appends wdd dance to output file
		bool WDD_WRITE_DANCE_FILE;

		// if set to true, appends wdd signal to output file
		bool WDD_WRITE_SIGNAL_FILE;

		CamConf auxCC;

	public:
		WaggleDanceDetector(			
			std::vector<cv::Point2i> dd_positions,
			cv::Mat * frame_ptr,
			std::vector<double> ddl_config,
			std::vector<double> wdd_dance_config,
			VideoFrameBuffer * videoFrameBuffer_ptr,
			CamConf auxCC,
			bool wdd_write_signal_file,
			bool wdd_write_dance_file,
			int wdd_verbose
			);
		~WaggleDanceDetector();

		void copyInitialFrame(unsigned long long fid, bool doDetection);
		void copyFrame(unsigned long long fid);

		bool isWDDSignal();
		bool isWDDDance();
		std::size_t getWDDSignalNumber();
		const std::map<std::size_t,cv::Point2d> * getWDDSignalId2PointMap();
		const std::vector<DANCE> * getWDDFinishedDancesVec();
		void printWDDDanceConfig();
		std::size_t getWDD_SIGNAL_DD_MIN_CLUSTER_SIZE();
		void setWDD_SIGNAL_DD_MIN_CLUSTER_SIZE(std::size_t val);
		// defines if verbose execution mode
		//TODO: check for remove as not used anymore
		static int WDD_VERBOSE;
		// saves unique #frame
		static unsigned long long WDD_SIGNAL_FRAME_NR;

	private:
		void _initWDDSignalValues();
		void _initWDDDanceValues();
		void _initOutPutFiles();

		void _setWDDConfig(std::vector<double> wdd_config);

		void _execDetection();
		void _execDetectionGetDDPotentials();
		void _execDetectionGetWDDSignals();
		void _execDetectionConcatWDDSignals();
		void _execDetectionHousekeepWDDSignals();
		void _execDetectionFinalizeDance(DANCE * d_ptr);
		void _execDetectionWriteDanceFileLine(DANCE * d_ptr);
		void _execDetectionWriteSignalFileLine();

		arma::Col<arma::uword> getNeighbours(arma::Col<arma::uword> sourceIDs, arma::Col<arma::uword> N, arma::Col<arma::uword> set_DD_IDs);
	};

} /* namespace WaggleDanceDetector */