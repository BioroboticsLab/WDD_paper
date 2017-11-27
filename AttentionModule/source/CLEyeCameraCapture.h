#pragma once


namespace wdd{	
	class CLEyeCameraCapture
	{
		CHAR _windowName[256];
		GUID _cameraGUID;
		CLEyeCameraInstance _cam;
		CLEyeCameraColorMode _mode;
		CLEyeCameraResolution _resolution;
		float _fps;
		HANDLE _hThread;
		bool _running;

		int _FRAME_WIDTH;
		int _FRAME_HEIGHT;
		bool _visual;
		bool _setupModeOn;

		double aux_DD_MIN_POTENTIAL;
		int	aux_WDD_SIGNAL_MIN_CLUSTER_SIZE;
	public:
		CLEyeCameraCapture(LPSTR windowName, GUID cameraGUID, CLEyeCameraColorMode mode, CLEyeCameraResolution resolution, float fps, CamConf CC, double dd_min_potential, int wdd_signal_min_cluster_size);

		bool StartCapture();
		void StopCapture();

		void setVisual(bool visual);
		void setSetupModeOn(bool setupMode);
		const CamConf * getCamConfPtr();

		void IncrementCameraParameter(int param);

		void DecrementCameraParameter(int param);
		void handleParameterQueue();
		void Setup();

		void Run();

		void drawArena(cv::Mat &frame);
		void drawPosDDs(cv::Mat &frame);
		void makeHeartBeatFile();
		bool pointIsInArena(cv::Point p);
		double computeProduct(cv::Point p, cv::Point2i a, cv::Point2i b);
		static DWORD WINAPI CaptureThread(LPVOID instance);

	};
}