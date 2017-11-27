#pragma once

namespace wdd {
	struct SAMP {
		float c0,c1,c2,c3,c4,c5,c6;
		float n;
		float s0,s1,s2,s3,s4,s5,s6;
		char padding1[4];
	};

	class DotDetector
	{
	private:
		// START BLOCK0 (0)
		// container to save raw uint8 frame pixel values of a single position		
		// 32 byte
		std::array<uchar,WDD_FBUFFER_SIZE> _DD_PX_VALS_RAW;
		// END BLOCK0 (31)

		// START BLOCK1 (32 % 16 == 0)
		// save extern pointer to DotDetectorLayer cv::Mat frame
		// (is expected to be at same location over runtime)
		// 4 byte
		uchar * aux_pixel_ptr;
		//
		// save position of next sample cos/sin value (=[0;frame_rate-1])
		// 4 byte
		std::size_t _sampPos;
		//
		// save own ID
		// 4 byte
		std::size_t _UNIQUE_ID;
		//
		// save inverse amplitude for faster normalization
		// 4 byte
		float _AMPLITUDE_INV;
		//
		// save min, max, amplitude values
		// 3 byte
		uchar _MIN,_MAX, _AMPLITUDE;
		//
		// save new min or max flag, old min/max gone flag
		// 5 byte
		bool _NEWMINMAX, _OLDMINGONE, _OLDMAXGONE, _NEWMINHERE, _NEWMAXHERE;
		// END BLOCK1 (24 byte)
		char padding1[8];
		// END BLOCK1 (63)

		// START BLOCK2 (64 % 16 == 0)
		// container to save accumulated COS/SIN per freq values
		// 64 byte
		SAMP _ACC_VAL;
		// END BLOCK2 (127)

		// START BLOCK3 (128 % 16 == 0)
		// save number of each possible pixel value currently in _DD_PX_VALS_RAW
		// 256 byte
		std::array<uchar,256> _UINT8_PX_VALS_COUNT;
		// END BLOCK3 (319)

												// START BLOCK3 (320 % 16 == 0)
												// container to save [-1:1] scaled values
												// 216 bytes (128 bytes raw, 88 bytes overhead)
												//arma::Row<float>::fixed<WDD_FBUFFER_SIZE> _DD_PX_VALS_NOR;
												// END BLOCK3 (535)
												//char padding2[8];
												// END BLOCK3 (543)

		// START BLOCK4 (384 % 16 == 0)
		// container to save COS/SIN/NOR values of corresponding _DD_PX_VALS_NOR
		// 2048 byte
		SAMP _DD_PX_VALS_COSSINNOR[WDD_FBUFFER_SIZE];
		// END BLOCK4 (2367)

		// START BLOCK5 (2432 % 16 == 0)
		// container to save frequency scores
		// 28 bytes
		std::array<float,WDD_FREQ_NUMBER> _DD_FREQ_SCORES; 
		// END BLOCK5 (2459)
		char padding5[36];
		// END BLOCK5 (2495)

		// TOTAL 39 cl a 64 byte = 2496  byte

		void _getInitialNewMinMax();
		void _getNewMinMax();
		void _execFullCalculation();
		void _execSingleCalculation();
		void _execDetection();
		//		float _normalizeValue(uchar u);
		void _normalizeValue(uchar u, float * f_ptr);
		float normalizeValue(const uchar u);
		void _nextSampPos();
	public:

#ifdef WDD_DDL_DEBUG_FULL
		arma::Row<float>::fixed<1> AUX_DD_POTENTIALS;
		arma::Row<float>::fixed<WDD_FREQ_NUMBER> AUX_DD_FREQ_SCORE;
		arma::Row<unsigned int>::fixed<WDD_FBUFFER_SIZE> AUX_DD_RAW_PX_VAL;
#endif
		DotDetector(std::size_t UNIQUE_ID, uchar * pixel_src_ptr);
		~DotDetector(void);

		void copyInitialPixel(bool doDetection);
		void copyPixelAndDetect();

		/*
		static 
		*/
		// save global position of next buffer write (=[0;WDD_FBUFFER_SIZE-1])
		static std::size_t _BUFF_POS;

		//DEBUG 
		//static std::size_t _NRCALL_EXECFULL;
		//static std::size_t _NRCALL_EXECSING;
		//static std::size_t _NRCALL_EXECSLEP;
		//static std::vector<uchar> _AMP_SAMPLES;
		static void nextBuffPos();
	};
} /* namespace WaggleDanceDetector */