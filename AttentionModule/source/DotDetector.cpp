#include "stdafx.h"
#include "DotDetector.h"
#include "DotDetectorLayer.h"
#include "WaggleDanceDetector.h"
#include "mutex"

namespace wdd {

	std::size_t DotDetector::_BUFF_POS=0;
	static std::mutex vector_acces;

	DotDetector::DotDetector(std::size_t UNIQUE_ID, uchar * pixel_src_ptr):
		// copy passed unique id 
		//(= position in arrays of DotDetectorLayer)
		_UNIQUE_ID(UNIQUE_ID),
		// copy passed pixel source pointer 
		//(= position in cv::Mat frame of DotDetectorLayer)
		aux_pixel_ptr(pixel_src_ptr),
		_OLDMINGONE(false),_OLDMAXGONE(false),_NEWMINHERE(false),_NEWMAXHERE(false), _NEWMINMAX(false),
		_MAX(255),_MIN(0),_AMPLITUDE(0),_sampPos(0)
	{
		_DD_PX_VALS_RAW.fill(0);
		//_DD_PX_VALS_NOR.fill(0);
		_UINT8_PX_VALS_COUNT.fill(0);
		_DD_FREQ_SCORES.fill(0);

		/*		
		printf("aux_pixel_ptr: %d\n",
		offsetof(DotDetector, DotDetector::aux_pixel_ptr));
		printf("_UNIQUE_ID: %d\n",
		offsetof(DotDetector, DotDetector::_UNIQUE_ID));
		printf("_sampPos: %d\n",
		offsetof(DotDetector, DotDetector::_sampPos));
		printf("_MIN: %d\n",
		offsetof(DotDetector, DotDetector::_MIN));
		printf("_MAX: %d\n",
		offsetof(DotDetector, DotDetector::_MAX));
		printf("_AMPLITUDE: %d\n",
		offsetof(DotDetector, DotDetector::_AMPLITUDE));
		printf("_NEWMINMAX: %d\n",
		offsetof(DotDetector, DotDetector::_NEWMINMAX));
		printf("_OLDMINGONE: %d\n",
		offsetof(DotDetector, DotDetector::_OLDMINGONE));
		printf("_OLDMAXGONE: %d\n",
		offsetof(DotDetector, DotDetector::_OLDMAXGONE));
		printf("_NEWMINHERE: %d\n",
		offsetof(DotDetector, DotDetector::_NEWMINHERE));
		printf("_NEWMAXHERE: %d\n",
		offsetof(DotDetector, DotDetector::_NEWMAXHERE));

		printf("_DD_PX_VALS_RAW: %d\n",
		offsetof(DotDetector, DotDetector::_DD_PX_VALS_RAW));
		printf("_UINT8_PX_VALS_COUNT: %d\n",
		offsetof(DotDetector, DotDetector::_UINT8_PX_VALS_COUNT));
		printf("_DD_PX_VALS_NOR: %d\n",
		offsetof(DotDetector, DotDetector::_DD_PX_VALS_NOR));
		printf("_DD_PX_VALS_SIN: %d\n",
		offsetof(DotDetector, DotDetector::_DD_PX_VALS_SIN));
		printf("_DD_PX_VALS_COS: %d\n",
		offsetof(DotDetector, DotDetector::_DD_PX_VALS_COS));
		printf("_DD_FREQ_SCORES: %d\n",
		offsetof(DotDetector, DotDetector::_DD_FREQ_SCORES));
		printf("_ACC_SIN_VAL: %d\n",
		offsetof(DotDetector, DotDetector::_ACC_SIN_VAL));
		printf("_ACC_COS_VAL: %d\n",
		offsetof(DotDetector, DotDetector::_ACC_COS_VAL));

		exit(0);
		*/		
	}

	DotDetector::~DotDetector(void)
	{
	}

	// add pixel values until buffer is full (initial filling phase)
	void DotDetector::copyInitialPixel(bool doDetection)
	{	
		// while initializing no deletetion (overwriting) occurs
		// expect _BUFF_POS have increasing values of [0;WDD_FBUFFER_SIZE-1]

		// save raw pixel value
		_DD_PX_VALS_RAW[_BUFF_POS] = *aux_pixel_ptr;

		// check if it is last call to _copyInitialPixel, all _DD_PX_VALS_RAW[] set
		if(doDetection)
		{
			_getInitialNewMinMax();
			// DEBUG
#ifdef WDD_DDL_DEBUG_FULL
			std::vector<uchar> tmp;
			std::copy(_DD_PX_VALS_RAW.begin(),_DD_PX_VALS_RAW.end(), std::back_inserter(tmp));
			arma::Row<uchar> T(tmp);
			AUX_DD_RAW_PX_VAL = arma::conv_to<arma::Row<unsigned int>>::from(T);
#endif
			if(_AMPLITUDE == 0)
			{
				DotDetectorLayer::DD_POTENTIALS[_UNIQUE_ID] = 0;
				return;
			}

			_execFullCalculation();
			_execDetection();
		}
	}

	void DotDetector::copyPixelAndDetect()
	{
		// handle oldest raw pixel (which will be overwritten by newest raw pixel)

		// decrease count of old raw pixel value, and if it is removed..
		if ((--_UINT8_PX_VALS_COUNT[_DD_PX_VALS_RAW[_BUFF_POS]]) == 0)
		{
			//..check if a former min/max is gone
			if(_DD_PX_VALS_RAW[_BUFF_POS ] == _MIN)
			{
				_OLDMINGONE = true;				
				// set new min
				while(_UINT8_PX_VALS_COUNT[_MIN] == 0)
					_MIN++;
			}
			else if(_DD_PX_VALS_RAW[_BUFF_POS] == _MAX)
			{
				_OLDMAXGONE = true;
				// set new max
				while(_UINT8_PX_VALS_COUNT[_MAX] == 0)
					_MAX--;
			}
		}

		// save new raw pixel value
		_DD_PX_VALS_RAW[_BUFF_POS] = *aux_pixel_ptr;

		// increase count of new raw pixel value, and if it is added..
		if((++_UINT8_PX_VALS_COUNT[_DD_PX_VALS_RAW[_BUFF_POS]]) == 1)
		{
			//..check if former min/max is valid
			if(_DD_PX_VALS_RAW[_BUFF_POS] < _MIN)
			{
				_NEWMINHERE = true;
				// set new min
				_MIN = _DD_PX_VALS_RAW[_BUFF_POS];
			}
			else if(_DD_PX_VALS_RAW[_BUFF_POS] > _MAX)
			{
				_NEWMAXHERE = true;
				// set new max
				_MAX = _DD_PX_VALS_RAW[_BUFF_POS];
			}
		}

		// DEBUG
#ifdef WDD_DDL_DEBUG_FULL
		std::vector<uchar> tmp;
		std::copy(_DD_PX_VALS_RAW.begin(),_DD_PX_VALS_RAW.end(), std::back_inserter(tmp));
		arma::Row<uchar> T(tmp);
		AUX_DD_RAW_PX_VAL = arma::conv_to<arma::Row<unsigned int>>::from(T);

		/*
		if(_UNIQUE_ID == 0)
		{
		std::cout<<"[UID: "<<_UNIQUE_ID<<"]_DD_PX_VALS_RAW: ";
		for(auto it=_DD_PX_VALS_RAW.begin(); it!=_DD_PX_VALS_RAW.end(); ++it)
		std::cout<<static_cast<int>(*it)<<" ";
		std::cout<<std::endl;

		arma::Row<uchar> T(tmp);
		T.print("T:");
		}
		*/
#endif

		if(_OLDMINGONE | _OLDMAXGONE | _NEWMINHERE | _NEWMAXHERE)
		{
			_AMPLITUDE = _MAX - _MIN;
			_AMPLITUDE_INV = 1.0f /_AMPLITUDE;
			// set flags
			_OLDMINGONE = _OLDMAXGONE = _NEWMINHERE = _NEWMAXHERE = false;
			_NEWMINMAX = true;
		}

		// if amplitude is zero, no further detection needed
		//TODO ORIG 13
		if(_AMPLITUDE < 13)
		{
			DotDetectorLayer::DD_POTENTIALS[_UNIQUE_ID] = 0;
			_NEWMINMAX = true;
#ifdef WDD_DDL_DEBUG_FULL
			AUX_DD_POTENTIALS.fill(0);
			AUX_DD_FREQ_SCORE.fill(0);
#endif
			return;
		}

		// when new min/max encountered,..
		if(_NEWMINMAX)
		{
			_execFullCalculation();
			_NEWMINMAX = false;
		}
		// else only one new value needs to be transformed
		else
		{
			_execSingleCalculation();
		}

		// leave DotDetector in a state ready to calculcate FREQ_SCORES
		_execDetection();
	}

	/*
	private functions
	*/
	inline void DotDetector::_getInitialNewMinMax()
	{
		// get initial count of raw pixel values
		for(std::size_t i=0; i < WDD_FBUFFER_SIZE ; i++)
			_UINT8_PX_VALS_COUNT[_DD_PX_VALS_RAW[i]]++;

		// move maximum border towards left
		while(_UINT8_PX_VALS_COUNT[_MAX] == 0)
			_MAX--;

		// move minimum border towards left
		while(_UINT8_PX_VALS_COUNT[_MIN] == 0)
			_MIN++;

		// finally get amplitude and save inversed one
		_AMPLITUDE = _MAX - _MIN;
		_AMPLITUDE_INV = 1.0f /_AMPLITUDE;
	}
	inline void DotDetector::_execFullCalculation()
	{
		// reset accumulators to zero
		memset(&_ACC_VAL, 0, sizeof(_ACC_VAL));
		
		// recalculate cos/sin values and set accumulators
		// get correct starting position in ring buffer
		std::size_t startPos = (_BUFF_POS+1) % WDD_FBUFFER_SIZE;
		std::size_t currtPos;

		float px_val;
		SAMP res;
		for(std::size_t j=0; j<WDD_FBUFFER_SIZE; j++)
		{
			// init loop vars
			currtPos = (j + startPos) % WDD_FBUFFER_SIZE;
			
			//_normalizeValue(_DD_PX_VALS_RAW[currtPos], &_DD_PX_VALS_COSSINNOR[currtPos].n);
			_DD_PX_VALS_COSSINNOR[currtPos].n = normalizeValue(_DD_PX_VALS_RAW[currtPos]);
			px_val = _DD_PX_VALS_COSSINNOR[currtPos].n;

			//t1 = GetRDTSC();
			res.c0 = DotDetectorLayer::SAMPLES[j].c0 * px_val;
			_DD_PX_VALS_COSSINNOR[currtPos].c0 = res.c0;
			_ACC_VAL.c0 += res.c0;
			//t2 = GetRDTSC();
			//MESS[0]+=(t2-t1);

			//t1 = GetRDTSC();
			res.c1 = DotDetectorLayer::SAMPLES[j].c1 * px_val;
			_DD_PX_VALS_COSSINNOR[currtPos].c1 = res.c1;
			_ACC_VAL.c1 += res.c1;
			//t2 = GetRDTSC();
			//MESS[1]+=(t2-t1);


			//t1 = GetRDTSC();
			res.c2 = DotDetectorLayer::SAMPLES[j].c2 * px_val;
			_DD_PX_VALS_COSSINNOR[currtPos].c2 = res.c2;
			_ACC_VAL.c2 += res.c2;
			//t2 = GetRDTSC();

			//MESS[2]+=(t2-t1);

			//t1 = GetRDTSC();
			res.c3 = DotDetectorLayer::SAMPLES[j].c3 * px_val;
			_DD_PX_VALS_COSSINNOR[currtPos].c3 = res.c3;
			_ACC_VAL.c3 += res.c3;
			//t2 = GetRDTSC();

			//MESS[3]+=(t2-t1);

			//t1 = GetRDTSC();
			res.c4 = DotDetectorLayer::SAMPLES[j].c4 * px_val;
			_DD_PX_VALS_COSSINNOR[currtPos].c4 = res.c4;
			_ACC_VAL.c4 += res.c4;
			//t2 = GetRDTSC();

			//MESS[4]+=(t2-t1);

			//t1 = GetRDTSC();
			res.c5 = DotDetectorLayer::SAMPLES[j].c5 * px_val;
			_DD_PX_VALS_COSSINNOR[currtPos].c5 = res.c5;
			_ACC_VAL.c5 += res.c5;
			//t2 = GetRDTSC();

			//MESS[5]+=(t2-t1);

			//t1 = GetRDTSC();
			res.c6 = DotDetectorLayer::SAMPLES[j].c6 * px_val;
			_DD_PX_VALS_COSSINNOR[currtPos].c6 = res.c6;
			_ACC_VAL.c6 += res.c6;
			//t2 = GetRDTSC();

			//MESS[6]+=(t2-t1);

			//t1 = GetRDTSC();
			res.s0 = DotDetectorLayer::SAMPLES[j].s0 * px_val;
			_DD_PX_VALS_COSSINNOR[currtPos].s0 = res.s0;
			_ACC_VAL.s0 += res.s0;
			//t2 = GetRDTSC();


			//MESS[7]+=(t2-t1);

			//t1 = GetRDTSC();
			res.s1 = DotDetectorLayer::SAMPLES[j].s1 * px_val;
			_DD_PX_VALS_COSSINNOR[currtPos].s1 = res.s1;
			_ACC_VAL.s1 += res.s1;
			//t2 = GetRDTSC();

			//MESS[8]+=(t2-t1);

			//t1 = GetRDTSC();
			res.s2 = DotDetectorLayer::SAMPLES[j].s2 * px_val;
			_DD_PX_VALS_COSSINNOR[currtPos].s2 = res.s2;
			_ACC_VAL.s2 += res.s2;
			//t2 = GetRDTSC();

			//MESS[9]+=(t2-t1);

			//t1 = GetRDTSC();
			res.s3 = DotDetectorLayer::SAMPLES[j].s3 * px_val;
			_DD_PX_VALS_COSSINNOR[currtPos].s3 = res.s3;
			_ACC_VAL.s3 += res.s3;
			//t2 = GetRDTSC();

			//MESS[10]+=(t2-t1);

			//t1 = GetRDTSC();
			res.s4 = DotDetectorLayer::SAMPLES[j].s4 * px_val;
			_DD_PX_VALS_COSSINNOR[currtPos].s4 = res.s4;
			_ACC_VAL.s4 += res.s4;
			//t2 = GetRDTSC();

			//MESS[11]+=(t2-t1);

			//t1 = GetRDTSC();
			res.s5 = DotDetectorLayer::SAMPLES[j].s5 * px_val;
			_DD_PX_VALS_COSSINNOR[currtPos].s5 = res.s5;
			_ACC_VAL.s5 += res.s5;
			//t2 = GetRDTSC();

			//MESS[12]+=(t2-t1);

			//t1 = GetRDTSC();
			res.s6 = DotDetectorLayer::SAMPLES[j].s6 * px_val;
			_DD_PX_VALS_COSSINNOR[currtPos].s6 = res.s6;
			_ACC_VAL.s6 += res.s6;		
			//t2 = GetRDTSC();

			//MESS[13]+=(t2-t1);
		}

		_sampPos = WDD_FBUFFER_SIZE;
	}

	inline void DotDetector::_execSingleCalculation()
	{

		//_normalizeValue(_DD_PX_VALS_RAW[_BUFF_POS], &_DD_PX_VALS_COSSINNOR[_BUFF_POS].n);
		_DD_PX_VALS_COSSINNOR[_BUFF_POS].n = normalizeValue(_DD_PX_VALS_RAW[_BUFF_POS]);

		// init loop vars

		float px_val = _DD_PX_VALS_COSSINNOR[_BUFF_POS].n;
		SAMP res;

		res.c0 = DotDetectorLayer::SAMPLES[_sampPos].c0 * px_val;		
		_ACC_VAL.c0 -= _DD_PX_VALS_COSSINNOR[_BUFF_POS].c0;
		_ACC_VAL.c0 += res.c0;
		_DD_PX_VALS_COSSINNOR[_BUFF_POS].c0 = res.c0;

		res.c1 = DotDetectorLayer::SAMPLES[_sampPos].c1 * px_val;
		_ACC_VAL.c1 -= _DD_PX_VALS_COSSINNOR[_BUFF_POS].c1;		
		_ACC_VAL.c1 += res.c1;
		_DD_PX_VALS_COSSINNOR[_BUFF_POS].c1 = res.c1;

		res.c2 = DotDetectorLayer::SAMPLES[_sampPos].c2 * px_val;
		_ACC_VAL.c2 -= _DD_PX_VALS_COSSINNOR[_BUFF_POS].c2;
		_ACC_VAL.c2 += res.c2;
		_DD_PX_VALS_COSSINNOR[_BUFF_POS].c2 = res.c2;

		res.c3 = DotDetectorLayer::SAMPLES[_sampPos].c3 * px_val;
		_ACC_VAL.c3 -= _DD_PX_VALS_COSSINNOR[_BUFF_POS].c3;
		_ACC_VAL.c3 += res.c3;
		_DD_PX_VALS_COSSINNOR[_BUFF_POS].c3 = res.c3;

		res.c4 = DotDetectorLayer::SAMPLES[_sampPos].c4 * px_val;
		_ACC_VAL.c4 -= _DD_PX_VALS_COSSINNOR[_BUFF_POS].c4;
		_ACC_VAL.c4 += res.c4;
		_DD_PX_VALS_COSSINNOR[_BUFF_POS].c4 = res.c4;

		res.c5 = DotDetectorLayer::SAMPLES[_sampPos].c5 * px_val;
		_ACC_VAL.c5 -= _DD_PX_VALS_COSSINNOR[_BUFF_POS].c5;
		_ACC_VAL.c5 += res.c5;
		_DD_PX_VALS_COSSINNOR[_BUFF_POS].c5 = res.c5;		

		res.c6 = DotDetectorLayer::SAMPLES[_sampPos].c6 * px_val;
		_ACC_VAL.c6 -= _DD_PX_VALS_COSSINNOR[_BUFF_POS].c6;
		_ACC_VAL.c6 += res.c6;
		_DD_PX_VALS_COSSINNOR[_BUFF_POS].c6 = res.c6;

		res.s0 = DotDetectorLayer::SAMPLES[_sampPos].s0 * px_val;
		_ACC_VAL.s0 -= _DD_PX_VALS_COSSINNOR[_BUFF_POS].s0;
		_ACC_VAL.s0 += res.s0;
		_DD_PX_VALS_COSSINNOR[_BUFF_POS].s0 = res.s0;

		res.s1 = DotDetectorLayer::SAMPLES[_sampPos].s1 * px_val;
		_ACC_VAL.s1 -= _DD_PX_VALS_COSSINNOR[_BUFF_POS].s1;
		_ACC_VAL.s1 += res.s1;
		_DD_PX_VALS_COSSINNOR[_BUFF_POS].s1 = res.s1;

		res.s2 = DotDetectorLayer::SAMPLES[_sampPos].s2 * px_val;
		_ACC_VAL.s2 -= _DD_PX_VALS_COSSINNOR[_BUFF_POS].s2;
		_ACC_VAL.s2 += res.s2;
		_DD_PX_VALS_COSSINNOR[_BUFF_POS].s2 = res.s2;

		res.s3 = DotDetectorLayer::SAMPLES[_sampPos].s3 * px_val;
		_ACC_VAL.s3 -= _DD_PX_VALS_COSSINNOR[_BUFF_POS].s3;
		_ACC_VAL.s3 += res.s3;
		_DD_PX_VALS_COSSINNOR[_BUFF_POS].s3 = res.s3;

		res.s4 = DotDetectorLayer::SAMPLES[_sampPos].s4 * px_val;
		_ACC_VAL.s4 -= _DD_PX_VALS_COSSINNOR[_BUFF_POS].s4;
		_ACC_VAL.s4 += res.s4;
		_DD_PX_VALS_COSSINNOR[_BUFF_POS].s4 = res.s4;

		res.s5 = DotDetectorLayer::SAMPLES[_sampPos].s5 * px_val;
		_ACC_VAL.s5 -= _DD_PX_VALS_COSSINNOR[_BUFF_POS].s5;
		_ACC_VAL.s5 += res.s5;
		_DD_PX_VALS_COSSINNOR[_BUFF_POS].s5 = res.s5;

		res.s6 = DotDetectorLayer::SAMPLES[_sampPos].s6 * px_val;
		_ACC_VAL.s6 -= _DD_PX_VALS_COSSINNOR[_BUFF_POS].s6;
		_ACC_VAL.s6 += res.s6;
		_DD_PX_VALS_COSSINNOR[_BUFF_POS].s6 = res.s6;

		_nextSampPos();
	}

	inline void DotDetector::_execDetection()
	{
		//Normalize Frequency Scores to Amount of Frames Analysed
		/*
		_ACC_VAL.c0 /= (double)WDD_FBUFFER_SIZE;
		_ACC_VAL.c1 /= (double)WDD_FBUFFER_SIZE;
		_ACC_VAL.c2 /= (double)WDD_FBUFFER_SIZE;
		_ACC_VAL.c3 /= (double)WDD_FBUFFER_SIZE;
		_ACC_VAL.c4 /= (double)WDD_FBUFFER_SIZE;
		_ACC_VAL.c5 /= (double)WDD_FBUFFER_SIZE;
		_ACC_VAL.c6 /= (double)WDD_FBUFFER_SIZE;
		_ACC_VAL.s0 /= (double)WDD_FBUFFER_SIZE;
		_ACC_VAL.s1 /= (double)WDD_FBUFFER_SIZE;
		_ACC_VAL.s2 /= (double)WDD_FBUFFER_SIZE;
		_ACC_VAL.s3 /= (double)WDD_FBUFFER_SIZE;
		_ACC_VAL.s4 /= (double)WDD_FBUFFER_SIZE;
		_ACC_VAL.s5 /= (double)WDD_FBUFFER_SIZE;
		_ACC_VAL.s6 /= (double)WDD_FBUFFER_SIZE;
		*/
		// score_i = sinSum_i^2 + cosSum_i^2
		_DD_FREQ_SCORES[0] = (_ACC_VAL.c0 * _ACC_VAL.c0)+(_ACC_VAL.s0 * _ACC_VAL.s0);
		_DD_FREQ_SCORES[1] = (_ACC_VAL.c1 * _ACC_VAL.c1)+(_ACC_VAL.s1 * _ACC_VAL.s1);
		_DD_FREQ_SCORES[2] = (_ACC_VAL.c2 * _ACC_VAL.c2)+(_ACC_VAL.s2 * _ACC_VAL.s2);
		_DD_FREQ_SCORES[3] = (_ACC_VAL.c3 * _ACC_VAL.c3)+(_ACC_VAL.s3 * _ACC_VAL.s3);
		_DD_FREQ_SCORES[4] = (_ACC_VAL.c4 * _ACC_VAL.c4)+(_ACC_VAL.s4 * _ACC_VAL.s4);
		_DD_FREQ_SCORES[5] = (_ACC_VAL.c5 * _ACC_VAL.c5)+(_ACC_VAL.s5 * _ACC_VAL.s5);
		_DD_FREQ_SCORES[6] = (_ACC_VAL.c6 * _ACC_VAL.c6)+(_ACC_VAL.s6 * _ACC_VAL.s6);	

#ifdef WDD_DDL_DEBUG_FULL
		AUX_DD_FREQ_SCORE = arma::Row<float>((float *)&_DD_FREQ_SCORES.begin(), WDD_FREQ_NUMBER, true, true);
#endif
		//Test 1: Weichzeichen, Testen ob größer
		
		//Entfernen das Minimum aus den Frequenz Scores, da gesuchte Frequenzen damit (fast) nicht beeinträchtigt werden, Aprupte Helligkeitsänderungen allerdings gefiltert werden, da bei diesen jeder Score etwas erkennt.
		/*
		float min = _DD_FREQ_SCORES[0];
		for (int i = 1; i < WDD_FREQ_NUMBER; i++){
			min = (_DD_FREQ_SCORES[i]<min) ? _DD_FREQ_SCORES[i] : min;
		}
		for (int i = 0; i < WDD_FREQ_NUMBER; i++){
			_DD_FREQ_SCORES[i] -= min;
		}
		*/
		//printf("%f\t%f\t%f\n", min, max, max-min);
		
		float buff;
		float max_buff;
		int max_buff_pos = 0;
		int i = 0;
		buff = _DD_FREQ_SCORES[i++] + _DD_FREQ_SCORES[i++];
		max_buff = buff;
		for (; i < WDD_FREQ_NUMBER; i++){
			if (buff > max_buff){
				max_buff_pos = i - 2;
				max_buff = buff;
			}
			buff += - _DD_FREQ_SCORES[i-2] + _DD_FREQ_SCORES[i];
		}
		if (buff > max_buff){
			max_buff_pos = i - 2;
			max_buff = buff;
		}
		if (max_buff>DotDetectorLayer::DD_MIN_POTENTIAL * WDD_FBUFFER_SIZE*WDD_FBUFFER_SIZE){
			vector_acces.lock();
			DotDetectorLayer::DD_SIGNALS_NUMBER++;
			DotDetectorLayer::DD_SIGNALS_IDs.push_back(_UNIQUE_ID);
			vector_acces.unlock();
			return;
		}
		//buff<DotDetectorLayer::DD_MIN_POTENTIAL * WDD_FBUFFER_SIZE*WDD_FBUFFER_SIZE && buff + 3 * min>DotDetectorLayer::DD_MIN_POTENTIAL * WDD_FBUFFER_SIZE*WDD_FBUFFER_SIZE

		//Test 2: Summe größer
		/*
		float sum=0;
		for (int i = 0; i < WDD_FREQ_NUMBER; i++){
			sum += _DD_FREQ_SCORES[i];
		}
		for (int i = 0; i < WDD_FREQ_NUMBER; i++){
			if (sum>120){
				DotDetectorLayer::DD_SIGNALS_NUMBER++;
				DotDetectorLayer::DD_SIGNALS_IDs.push_back(_UNIQUE_ID);
				break;
			}
		}
		*/
		//ORIGINAL
		/*
		std::sort(_DD_FREQ_SCORES.begin(), _DD_FREQ_SCORES.begin()+WDD_FREQ_NUMBER);

		float potential = 0;

		for(std::size_t freq_i=0; freq_i<WDD_FREQ_NUMBER; freq_i++)
		{
			potential += (freq_i+1)*_DD_FREQ_SCORES[freq_i];
		}

		potential *= _AMPLITUDE;
		
		DotDetectorLayer::DD_POTENTIALS[_UNIQUE_ID] = potential;

		if(potential > DotDetectorLayer::DD_MIN_POTENTIAL)
		{
			DotDetectorLayer::DD_SIGNALS_NUMBER++;
			DotDetectorLayer::DD_SIGNALS_IDs.push_back(_UNIQUE_ID);
		}
		*/
		

#ifdef WDD_DDL_DEBUG_FULL
		AUX_DD_POTENTIALS(0) = potential;		
#endif
	}

	// alternative definition of _normalizeValue
	float DotDetector::normalizeValue(const uchar v)
	{
		float flt_v = static_cast<float>(v);
		return (2 * _AMPLITUDE_INV * (flt_v - _MIN)) - 1;
	}

	// original function
	inline void DotDetector::_normalizeValue(uchar u, float * f_ptr)
	{
		*f_ptr = static_cast<float>(u);		
		*f_ptr -= _MIN; //0.. MAX
		*f_ptr *= _AMPLITUDE_INV; //0..1
		*f_ptr *= 2; //0..2
		*f_ptr -= 1; //-1 .. 1
	}

	inline void DotDetector::_nextSampPos()
	{
		// values between [0;frame_rate-1]
		//_sampPos++;
		//_sampPos %= DotDetectorLayer::FRAME_RATEi;
		_sampPos = ( ++_sampPos < WDD_FRAME_RATE ? _sampPos : 0);
	}

	void DotDetector::nextBuffPos()
	{
		DotDetector::_BUFF_POS++;
		DotDetector::_BUFF_POS%= WDD_FBUFFER_SIZE;
		//DotDetector::_BUFF_POS = ( ++DotDetector::_BUFF_POS < WDD_FBUFFER_SIZE ? DotDetector::_BUFF_POS : 0);
	}
} /* namespace WaggleDanceDetector */