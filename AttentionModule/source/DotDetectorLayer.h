#pragma once
#include "DotDetector.h"

namespace wdd{

	class DotDetectorLayer
	{
	public:
		/*
		members
		*/
		// save sampled target sinus and cosinus signals as array of SAMP
		static SAMP SAMPLES[WDD_FRAME_RATE];

		// saves positions of used DotDetectors
		static std::vector<cv::Point2i> DD_POSITIONS;
		// saves number n of DotDetectors in DD_POSITIONS
		static std::size_t DD_NUMBER;
		// defines the absolute score value a DD needs for signal
		static double DD_MIN_POTENTIAL;

		// saves potential of n = DD_POSITIONS_NUMBER DotDetectors (potentials are
		// matched against WDD_SIGNAL_DD_MIN_SCORE - their calculation is part of
		// magic)
		static double *	DD_POTENTIALS;
		// saves number of positive DDs
		static std::size_t DD_SIGNALS_NUMBER;
		// saves ids of positive DDs (interface to WDD Layer 2)
		//(a DotDetecotor has a positive signal if DD_SIGNAL_POTENTIALS[i] > WDD_SIGNAL_DD_MIN_SCORE)
		static std::vector<unsigned int> DD_SIGNALS_IDs;

		// defines detection frequency lower border
		static double 	DD_FREQ_MIN;
		// defines detection frequency upper border
		static double 	DD_FREQ_MAX;
		// defines step size to use between upper and lower border
		static double 	DD_FREQ_STEP;
		// saves each target frequency, created with min,max,step
		static std::vector<double> DD_FREQS;
		// saves number of DD_FREQS
		static std::size_t DD_FREQS_NUMBER;

		// saves frame rate
		static std::size_t FRAME_RATEi;
		// saves frame reduction factor
		static double FRAME_REDFAC;

		//static std::vector<unsigned __int64> DDL_DEBUG_PERF;
#ifdef WDD_DDL_DEBUG_FULL
		static arma::Mat<float> DDL_DEBUG_DD_POTENTIALS;
		static arma::Mat<float> DDL_DEBUG_DD_FREQ_SCORE;
		static arma::Mat<unsigned int> DDL_DEBUG_DD_RAW_PX_VAL;
		
		static std::vector<std::size_t> DDL_DEBUG_DD_FIRING_IDs;
		static arma::Mat<float> DDL_DEBUG_DD_FIRING_ID_POTENTIALS;
		static arma::Mat<float> DDL_DEBUG_DD_FIRING_ID_FREQ_SCORE;
		static arma::Mat<unsigned int> DDL_DEBUG_DD_FIRING_ID_RAW_PX_VAL;
		
		static void debugWriteFiles();
#endif
		/*
		functions
		*/
		static void init(std::vector<cv::Point2i> dd_positions, cv::Mat * aux_frame_ptr, 
			std::vector<double> ddl_config);

		static void release();

		static void copyFrame(bool doDetection);
		static void copyFrameAndDetect();

		static void printFreqConfig();
		static void printFreqSamples();

	private:
		/*
		members
		*/
		static DotDetector ** _DotDetectors;
		/*
		functions
		*/
		static void _setDDLConfig(std::vector<double> ddl_config);
		static void _createFreqSamples();
	};

} /* namespace WaggleDanceDetector */