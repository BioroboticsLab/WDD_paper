#include "stdafx.h"
#include "DotDetectorLayer.h"
#include "WaggleDanceDetector.h"
#include "ppl.h"

namespace wdd {
	// is this how c++ works? 
	std::vector<cv::Point2i>	DotDetectorLayer::DD_POSITIONS;
	std::size_t					DotDetectorLayer::DD_NUMBER;
	double						DotDetectorLayer::DD_MIN_POTENTIAL;
	double *					DotDetectorLayer::DD_POTENTIALS;
	std::size_t					DotDetectorLayer::DD_SIGNALS_NUMBER;
	std::vector<unsigned int>	DotDetectorLayer::DD_SIGNALS_IDs;
	double 						DotDetectorLayer::DD_FREQ_MIN;
	double 						DotDetectorLayer::DD_FREQ_MAX;
	double 						DotDetectorLayer::DD_FREQ_STEP;
	std::size_t					DotDetectorLayer::FRAME_RATEi;
	double						DotDetectorLayer::FRAME_REDFAC;
	std::vector<double>			DotDetectorLayer::DD_FREQS;
	std::size_t					DotDetectorLayer::DD_FREQS_NUMBER;
	SAMP						DotDetectorLayer::SAMPLES[WDD_FRAME_RATE];
	//std::vector<unsigned __int64> DotDetectorLayer::DDL_DEBUG_PERF;
	//arma::Mat<float>::fixed<WDD_FRAME_RATE,WDD_FREQ_NUMBER> DotDetectorLayer::DD_FREQS_COSSAMPLES;
	//arma::Mat<float>::fixed<WDD_FRAME_RATE,WDD_FREQ_NUMBER> DotDetectorLayer::DD_FREQS_SINSAMPLES;
	DotDetector **				DotDetectorLayer::_DotDetectors;

#ifdef WDD_DDL_DEBUG_FULL
	arma::Mat<float>			DotDetectorLayer::DDL_DEBUG_DD_POTENTIALS;
	arma::Mat<float>			DotDetectorLayer::DDL_DEBUG_DD_FREQ_SCORE;
	arma::Mat<unsigned int>		DotDetectorLayer::DDL_DEBUG_DD_RAW_PX_VAL;
	std::vector<std::size_t>	DotDetectorLayer::DDL_DEBUG_DD_FIRING_IDs(WDD_DDL_DEBUG_FULL_MAX_FIRING_DDS);
	arma::Mat<float>			DotDetectorLayer::DDL_DEBUG_DD_FIRING_ID_POTENTIALS;
	arma::Mat<float>			DotDetectorLayer::DDL_DEBUG_DD_FIRING_ID_FREQ_SCORE;
	arma::Mat<unsigned int>		DotDetectorLayer::DDL_DEBUG_DD_FIRING_ID_RAW_PX_VAL;	
#endif
	void DotDetectorLayer::init(std::vector<cv::Point2i> dd_positions, cv::Mat * frame_ptr, 
		std::vector<double> ddl_config)
	{
		DotDetectorLayer::_setDDLConfig(ddl_config);

		// save DD_POSITIONS
		DotDetectorLayer::DD_POSITIONS = dd_positions;

		// get number of DotDetectors
		DotDetectorLayer::DD_NUMBER = DotDetectorLayer::DD_POSITIONS.size();

		// allocate space for DotDetector potentials
		DotDetectorLayer::DD_POTENTIALS = new double[DotDetectorLayer::DD_NUMBER];
		
		// allocate DotDetector pointer array
		DotDetectorLayer::_DotDetectors = new DotDetector * [DD_NUMBER];

		DotDetectorLayer::DD_SIGNALS_IDs.reserve(WDD_LAYER2_MAX_POS_DDS);

		// create DotDetectors, pass unique id and location of pixel
		for(std::size_t i=0; i<DD_NUMBER; i++)
		{
			DotDetectorLayer::_DotDetectors[i] = new DotDetector(i, &((*frame_ptr).at<uchar>(DD_POSITIONS[i])));
			//std::cout<<sizeof(*DotDetectorLayer::_DotDetectors[i])<<std::endl;
			//std::cout<<&(*DotDetectorLayer::_DotDetectors[i])<<std::endl;

			//if(i==2)
			//exit(0);
		}

	}

	void DotDetectorLayer::release(void)
	{
		for(std::size_t i=0; i<DotDetectorLayer::DD_NUMBER; i++)
			delete DotDetectorLayer::_DotDetectors[i];

		delete DotDetectorLayer::_DotDetectors;
		delete DotDetectorLayer::DD_POTENTIALS;
	}

	// expect to be used as initial function to fill buffer and have only one single call
	// with doDetection = true
	void DotDetectorLayer::copyFrame(bool doDetection)
	{
#ifdef WDD_DDL_DEBUG_FULL
		if(doDetection)
		{
			DDL_DEBUG_DD_POTENTIALS.clear();DDL_DEBUG_DD_FREQ_SCORE.clear();DDL_DEBUG_DD_RAW_PX_VAL.clear();
			DDL_DEBUG_DD_FIRING_ID_RAW_PX_VAL.clear();DDL_DEBUG_DD_FIRING_IDs.clear();
			DDL_DEBUG_DD_FIRING_ID_POTENTIALS.clear();DDL_DEBUG_DD_FIRING_ID_FREQ_SCORE.clear();
		}
#endif
		DotDetectorLayer::DD_SIGNALS_NUMBER = 0;
		DotDetectorLayer::DD_SIGNALS_IDs.clear();
		concurrency::parallel_for(std::size_t(0), DotDetectorLayer::DD_NUMBER, [&](size_t i)
		{
			DotDetectorLayer::_DotDetectors[i]->copyInitialPixel(doDetection);
#ifdef WDD_DDL_DEBUG_FULL		
			if(doDetection)
			{
				if(i < WDD_DDL_DEBUG_FULL_MAX_DDS)	
				{
					DDL_DEBUG_DD_POTENTIALS.insert_rows(i,DotDetectorLayer::_DotDetectors[i]->AUX_DD_POTENTIALS);
					DDL_DEBUG_DD_FREQ_SCORE.insert_rows(i,DotDetectorLayer::_DotDetectors[i]->AUX_DD_FREQ_SCORE);
					DDL_DEBUG_DD_RAW_PX_VAL.insert_rows(i,DotDetectorLayer::_DotDetectors[i]->AUX_DD_RAW_PX_VAL);					
				}
				if(DotDetectorLayer::DD_SIGNALS[i] && (DDL_DEBUG_DD_FIRING_IDs.size() < WDD_DDL_DEBUG_FULL_MAX_FIRING_DDS))
				{
					DDL_DEBUG_DD_FIRING_IDs.push_back(i);
				}
			}
#endif
		});// end for-loop

		DotDetector::nextBuffPos();

#ifdef WDD_DDL_DEBUG_FULL
		if(doDetection)
		{
			std::size_t i=0;			
			for(auto it=DDL_DEBUG_DD_FIRING_IDs.begin();it!=DDL_DEBUG_DD_FIRING_IDs.end();++it, i++)
			{
				DDL_DEBUG_DD_FIRING_ID_POTENTIALS.insert_rows(i,DotDetectorLayer::_DotDetectors[*it]->AUX_DD_POTENTIALS);
				DDL_DEBUG_DD_FIRING_ID_FREQ_SCORE.insert_rows(i,DotDetectorLayer::_DotDetectors[*it]->AUX_DD_FREQ_SCORE);
				DDL_DEBUG_DD_FIRING_ID_RAW_PX_VAL.insert_rows(i,DotDetectorLayer::_DotDetectors[*it]->AUX_DD_RAW_PX_VAL);
			}
			DDL_DEBUG_DD_FIRING_ID_POTENTIALS.insert_cols(0,arma::conv_to<arma::Col<float>>::from(arma::Col<unsigned int>(DDL_DEBUG_DD_FIRING_IDs)));
			DDL_DEBUG_DD_FIRING_ID_FREQ_SCORE.insert_cols(0,arma::conv_to<arma::Col<float>>::from(arma::Col<unsigned int>(DDL_DEBUG_DD_FIRING_IDs)));
			DDL_DEBUG_DD_FIRING_ID_RAW_PX_VAL.insert_cols(0,arma::Col<unsigned int>(DDL_DEBUG_DD_FIRING_IDs));
			DotDetectorLayer::debugWriteFiles();
		}
#endif
	}
	/*
	unsigned __int64 inline GetRDTSC() {
		__asm {
			; Flush the pipeline
				XOR eax, eax
				CPUID
				; Get RDTSC counter in edx:eax
				RDTSC
		}
	}
	*/
	void DotDetectorLayer::copyFrameAndDetect()
	{
#ifdef WDD_DDL_DEBUG_FULL
		DDL_DEBUG_DD_POTENTIALS.clear();DDL_DEBUG_DD_FREQ_SCORE.clear();DDL_DEBUG_DD_RAW_PX_VAL.clear();
		DDL_DEBUG_DD_FIRING_ID_RAW_PX_VAL.clear();
		DDL_DEBUG_DD_FIRING_ID_POTENTIALS.clear();DDL_DEBUG_DD_FIRING_ID_FREQ_SCORE.clear();
#endif
		DotDetectorLayer::DD_SIGNALS_NUMBER = 0;
		DotDetectorLayer::DD_SIGNALS_IDs.clear();
		//unsigned __int64 t1 = GetRDTSC();

		concurrency::parallel_for(std::size_t(0), DotDetectorLayer::DD_NUMBER, [&](size_t i)
		{
			DotDetectorLayer::_DotDetectors[i]->copyPixelAndDetect();
#ifdef WDD_DDL_DEBUG_FULL		
			if(i < WDD_DDL_DEBUG_FULL_MAX_DDS)	
			{
				DDL_DEBUG_DD_POTENTIALS.insert_rows(i,DotDetectorLayer::_DotDetectors[i]->AUX_DD_POTENTIALS);
				DDL_DEBUG_DD_FREQ_SCORE.insert_rows(i,DotDetectorLayer::_DotDetectors[i]->AUX_DD_FREQ_SCORE);
				DDL_DEBUG_DD_RAW_PX_VAL.insert_rows(i,DotDetectorLayer::_DotDetectors[i]->AUX_DD_RAW_PX_VAL);					
			}
			if(DotDetectorLayer::DD_SIGNALS[i] && (DDL_DEBUG_DD_FIRING_IDs.size() < WDD_DDL_DEBUG_FULL_MAX_FIRING_DDS))
			{
				bool isNew = true;
				//check for id
				for(auto it=DDL_DEBUG_DD_FIRING_IDs.begin();it!=DDL_DEBUG_DD_FIRING_IDs.end();++it)
				{
					if(*it == i)
					{
						isNew = false; break;
					}
				}
				if(isNew)
				{
					DDL_DEBUG_DD_FIRING_IDs.push_back(i);
				}
			}
#endif

		});// end for-loop

		DotDetector::nextBuffPos();
		
		//unsigned __int64 t2 = GetRDTSC();
		//DDL_DEBUG_PERF.push_back(t2-t1);

#ifdef WDD_DDL_DEBUG_FULL
		std::size_t i=0;
		for(auto it=DDL_DEBUG_DD_FIRING_IDs.begin();it!=DDL_DEBUG_DD_FIRING_IDs.end();++it, i++)
		{
			DDL_DEBUG_DD_FIRING_ID_POTENTIALS.insert_rows(i,DotDetectorLayer::_DotDetectors[*it]->AUX_DD_POTENTIALS);
			DDL_DEBUG_DD_FIRING_ID_FREQ_SCORE.insert_rows(i,DotDetectorLayer::_DotDetectors[*it]->AUX_DD_FREQ_SCORE);
			DDL_DEBUG_DD_FIRING_ID_RAW_PX_VAL.insert_rows(i,DotDetectorLayer::_DotDetectors[*it]->AUX_DD_RAW_PX_VAL);
		}
		DDL_DEBUG_DD_FIRING_ID_POTENTIALS.insert_cols(0,arma::conv_to<arma::Col<float>>::from(arma::Col<unsigned int>(DDL_DEBUG_DD_FIRING_IDs)));
		DDL_DEBUG_DD_FIRING_ID_FREQ_SCORE.insert_cols(0,arma::conv_to<arma::Col<float>>::from(arma::Col<unsigned int>(DDL_DEBUG_DD_FIRING_IDs)));
		DDL_DEBUG_DD_FIRING_ID_RAW_PX_VAL.insert_cols(0,arma::Col<unsigned int>(DDL_DEBUG_DD_FIRING_IDs));
		DotDetectorLayer::debugWriteFiles();
#endif
	}

	/* Given a DotDetector frequency configuration consisting of min:step:max this
	* function calculates each single frequency value and triggers
	* createFreqSamples(), which creates the appropriate sinus and cosinus samples.
	* This function shall be designed in such a way that adjustments of frequency
	* configuration may be applied 'on the fly'*/
	/*
	* Dependencies: WDD_FBUFFER_SIZE, FRAME_RATE, WDD_VERBOSE
	*/
	void DotDetectorLayer::_setDDLConfig(std::vector<double> ddl_config)
	{
		/* first check right number of arguments */
		if(ddl_config.size() != 6)
		{
			std::cerr<<"Error! DotDetectorLayer::_setFreqConfig: wrong number of config arguments!"<<std::endl;
			exit(-19);
		}

		/* copy values from ddl_config*/
		double dd_freq_min = ddl_config[0];
		double dd_freq_max = ddl_config[1];
		double dd_freq_step = ddl_config[2];
		double dd_frame_rate = ddl_config[3];
		double dd_frame_redfac = ddl_config[4];
		double dd_min_potential = ddl_config[5];

		/* do some sanity checks */
		if(dd_freq_min > dd_freq_max)
		{
			std::cerr<<"Error! DotDetectorLayer::_setFreqConfig: dd_freq_min > dd_freq_max!"<<std::endl;
			exit(-20);
		}
		if((dd_freq_max-dd_freq_min)<dd_freq_step)
		{
			std::cerr<<"Error! DotDetectorLayer::_setFreqConfig: (dd_freq_max-dd_freq_min)<dd_freq_step!"<<std::endl;
			exit(-20);
		}
		if(dd_frame_rate<=0)
		{
			std::cerr<<"Error! DotDetectorLayer::_setFreqConfig: dd_frame_rate<=0!"<<std::endl;
			exit(-21);
		}
		
		if(dd_frame_redfac<0)
		{
			std::cerr<<"Error! DotDetectorLayer::_setFreqConfig: dd_frame_redfac<0!"<<std::endl;
			exit(-22);
		}
		
		if(dd_min_potential<=0)
		{
			std::cerr<<"Error! DotDetectorLayer::_setFreqConfig: dd_min_score<=0!"<<std::endl;
			exit(-23);
		}

		/* assign passed values */
		DotDetectorLayer::DD_FREQ_MIN = dd_freq_min;
		DotDetectorLayer::DD_FREQ_MAX = dd_freq_max;
		DotDetectorLayer::DD_FREQ_STEP = dd_freq_step;
		DotDetectorLayer::FRAME_RATEi = static_cast<std::size_t>(dd_frame_rate);
		DotDetectorLayer::FRAME_REDFAC = dd_frame_redfac;
		DotDetectorLayer::DD_MIN_POTENTIAL = dd_min_potential;

		/* invoke sinus and cosinus sample creation */
		_createFreqSamples();

		/* finally present output if verbose mode is on*/
		if(WaggleDanceDetector::WDD_VERBOSE)
		{
			DotDetectorLayer::printFreqConfig();
			DotDetectorLayer::printFreqSamples();
		}
	}
	/*
	* Dependencies: WDD_FBUFFER_SIZE, FRAME_RATE, DD_FREQS_NUMBER, DD_FREQS
	* Frame_rate needs to be integer type
	*/
	void DotDetectorLayer::_createFreqSamples()
	{
		/* calculate frequencies */
		std::vector<double> dd_freqs;
		double dd_freq_cur = DotDetectorLayer::DD_FREQ_MIN;

		while(dd_freq_cur<=DotDetectorLayer::DD_FREQ_MAX)
		{
			dd_freqs.push_back(dd_freq_cur);
			dd_freq_cur += DotDetectorLayer::DD_FREQ_STEP;
		}

		/* assign frequencies */
		DotDetectorLayer::DD_FREQS_NUMBER = dd_freqs.size();
		//DotDetector::DD_FREQS_NUMBER = dd_freqs.size();
		DotDetectorLayer::DD_FREQS = dd_freqs;

		double step = 1.0 / WDD_FRAME_RATE;

		for(std::size_t s=0; s<WDD_FRAME_RATE; s++)
		{
			double fac = 2*CV_PI;

			SAMPLES[s].c0 = static_cast<float>(cos(fac * dd_freqs[0] * step * s));
			SAMPLES[s].s0 = static_cast<float>(sin(fac * dd_freqs[0] * step * s));

			SAMPLES[s].c1 = static_cast<float>(cos(fac * dd_freqs[1] * step * s));
			SAMPLES[s].s1 = static_cast<float>(sin(fac * dd_freqs[1] * step * s));

			SAMPLES[s].c2 = static_cast<float>(cos(fac * dd_freqs[2] * step * s));
			SAMPLES[s].s2 = static_cast<float>(sin(fac * dd_freqs[2] * step * s));

			SAMPLES[s].c3 = static_cast<float>(cos(fac * dd_freqs[3] * step * s));
			SAMPLES[s].s3 = static_cast<float>(sin(fac * dd_freqs[3] * step * s));

			SAMPLES[s].c4 = static_cast<float>(cos(fac * dd_freqs[4] * step * s));
			SAMPLES[s].s4 = static_cast<float>(sin(fac * dd_freqs[4] * step * s));

			SAMPLES[s].c5 = static_cast<float>(cos(fac * dd_freqs[5] * step * s));
			SAMPLES[s].s5 = static_cast<float>(sin(fac * dd_freqs[5] * step * s));

			SAMPLES[s].c6 = static_cast<float>(cos(fac * dd_freqs[6] * step * s));	
			SAMPLES[s].s6 = static_cast<float>(sin(fac * dd_freqs[6] * step * s));
		}


	}
	void DotDetectorLayer::printFreqConfig()
	{
		printf("Printing frequency configuration:\n");
		printf("[DD_FREQ_MIN] %.1f\n",DotDetectorLayer::DD_FREQ_MIN);
		printf("[DD_FREQ_MAX] %.1f\n",DotDetectorLayer::DD_FREQ_MAX);
		printf("[DD_FREQ_STEP] %.1f\n",DotDetectorLayer::DD_FREQ_STEP);
		for(std::size_t i=0; i< DotDetectorLayer::DD_FREQS_NUMBER; i++)
			printf("[FREQ[%lu]] %.1f Hz\n", i, DotDetectorLayer::DD_FREQS[i]);
	}
	void DotDetectorLayer::printFreqSamples()
	{
		printf("Printing frequency samples:\n");

		printf("[%.1f Hz (cos)] ", DotDetectorLayer::DD_FREQS[0]);
		for(std::size_t j=0; j< DotDetectorLayer::FRAME_RATEi; j++)
			printf("%.3f ", SAMPLES[j].c0);
		printf("\n");

		printf("[%.1f Hz (sin)] ", DotDetectorLayer::DD_FREQS[0]);
		for(std::size_t j=0; j< DotDetectorLayer::FRAME_RATEi; j++)
			printf("%.3f ", SAMPLES[j].s0);
		printf("\n");

		printf("[%.1f Hz (cos)] ", DotDetectorLayer::DD_FREQS[1]);
		for(std::size_t j=0; j< DotDetectorLayer::FRAME_RATEi; j++)
			printf("%.3f ", SAMPLES[j].c1);
		printf("\n");

		printf("[%.1f Hz (sin)] ", DotDetectorLayer::DD_FREQS[1]);
		for(std::size_t j=0; j< DotDetectorLayer::FRAME_RATEi; j++)
			printf("%.3f ", SAMPLES[j].s1);
		printf("\n");

		printf("[%.1f Hz (cos)] ", DotDetectorLayer::DD_FREQS[2]);
		for(std::size_t j=0; j< DotDetectorLayer::FRAME_RATEi; j++)
			printf("%.3f ", SAMPLES[j].c2);
		printf("\n");

		printf("[%.1f Hz (sin)] ", DotDetectorLayer::DD_FREQS[2]);
		for(std::size_t j=0; j< DotDetectorLayer::FRAME_RATEi; j++)
			printf("%.3f ", SAMPLES[j].s2);
		printf("\n");

		printf("[%.1f Hz (cos)] ", DotDetectorLayer::DD_FREQS[3]);
		for(std::size_t j=0; j< DotDetectorLayer::FRAME_RATEi; j++)
			printf("%.3f ", SAMPLES[j].c3);
		printf("\n");

		printf("[%.1f Hz (sin)] ", DotDetectorLayer::DD_FREQS[3]);
		for(std::size_t j=0; j< DotDetectorLayer::FRAME_RATEi; j++)
			printf("%.3f ", SAMPLES[j].s3);
		printf("\n");

		printf("[%.1f Hz (cos)] ", DotDetectorLayer::DD_FREQS[4]);
		for(std::size_t j=0; j< DotDetectorLayer::FRAME_RATEi; j++)
			printf("%.3f ", SAMPLES[j].c4);
		printf("\n");

		printf("[%.1f Hz (sin)] ", DotDetectorLayer::DD_FREQS[4]);
		for(std::size_t j=0; j< DotDetectorLayer::FRAME_RATEi; j++)
			printf("%.3f ", SAMPLES[j].s4);
		printf("\n");

		printf("[%.1f Hz (cos)] ", DotDetectorLayer::DD_FREQS[5]);
		for(std::size_t j=0; j< DotDetectorLayer::FRAME_RATEi; j++)
			printf("%.3f ", SAMPLES[j].c5);
		printf("\n");

		printf("[%.1f Hz (sin)] ", DotDetectorLayer::DD_FREQS[5]);
		for(std::size_t j=0; j< DotDetectorLayer::FRAME_RATEi; j++)
			printf("%.3f ", SAMPLES[j].s5);
		printf("\n");

		printf("[%.1f Hz (cos)] ", DotDetectorLayer::DD_FREQS[6]);
		for(std::size_t j=0; j< DotDetectorLayer::FRAME_RATEi; j++)
			printf("%.3f ", SAMPLES[j].c6);
		printf("\n");

		printf("[%.1f Hz (sin)] ", DotDetectorLayer::DD_FREQS[6]);
		for(std::size_t j=0; j< DotDetectorLayer::FRAME_RATEi; j++)
			printf("%.3f ", SAMPLES[j].s6);
		printf("\n");
	}
#ifdef WDD_DDL_DEBUG_FULL
	void DotDetectorLayer::debugWriteFiles()
	{
		std::stringstream a;
		a <<"DDL_DEBUG_DD_POTENTIALS_F";
		a << WaggleDanceDetector::WDD_SIGNAL_FRAME_NR;
		a <<".txt";
		DDL_DEBUG_DD_POTENTIALS.save(a.str(), arma::arma_ascii);

		a.str("");
		a <<"DDL_DEBUG_DD_FREQ_SCORE_F";
		a <<  WaggleDanceDetector::WDD_SIGNAL_FRAME_NR;
		a <<".txt";
		DDL_DEBUG_DD_FREQ_SCORE.save(a.str(), arma::arma_ascii);

		a.str("");
		a <<"DDL_DEBUG_DD_RAW_PX_VAL_F";
		a <<  WaggleDanceDetector::WDD_SIGNAL_FRAME_NR;
		a <<".txt";
		DDL_DEBUG_DD_RAW_PX_VAL.save(a.str(), arma::arma_ascii);


		a.str("");
		a <<"DDL_DEBUG_DD_FIRING_ID_POTENTIALS_F";
		a <<  WaggleDanceDetector::WDD_SIGNAL_FRAME_NR;
		a <<".txt";
		DDL_DEBUG_DD_FIRING_ID_POTENTIALS.save(a.str(), arma::arma_ascii);

		a.str("");
		a <<"DDL_DEBUG_DD_FIRING_ID_FREQ_SCORE_F";
		a <<  WaggleDanceDetector::WDD_SIGNAL_FRAME_NR;
		a <<".txt";
		DDL_DEBUG_DD_FIRING_ID_FREQ_SCORE.save(a.str(), arma::arma_ascii);

		a.str("");
		a <<"DDL_DEBUG_DD_FIRING_ID_RAW_PX_VAL_F";
		a <<  WaggleDanceDetector::WDD_SIGNAL_FRAME_NR;
		a <<".txt";
		DDL_DEBUG_DD_FIRING_ID_RAW_PX_VAL.save(a.str(), arma::arma_ascii);
	}
#endif
} /* namespace WaggleDanceDetector */