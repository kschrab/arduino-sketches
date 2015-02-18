#ifndef LightCluster_h 
#define LightCluster_h
 
#include "Arduino.h"
#include "Adafruit_WS2801.h"

class LightCluster {
	
	private:
		int noteR;
		int noteG;
		int noteB;
		int noteCtrl;
		int *lights;
		int numOfLights;
		/**one shot mode*/
		int oneHitDuration;
		int oneHitOffset;
		int channel;
		bool oneHitMode;
		
		int latestR;
		int latestG;
		int latestB;
		int latestCommand;
		/** fading */
		unsigned long lastCommandMillis;
		bool on;
		/** step light */
		int currentLight;
		int hitCounter;
		/** random strobo */
		long randomColor;
		
		static const int durations[];
		static const float stroboFrequencies[];
		static const float pulseFrequencies[];
		static const float pulseDepths[];
		
		long color(int r,int g, int b);
		int randomNumber(int min, int max);
		void commandHit();
		
	public:
		LightCluster(int noteR, int noteG, int noteB, int noteCtrl, int *lights, int numOfLights);
		LightCluster(int channel, int noteHit, int r, int g, int b, int mode, int duration, int offset, int *lights, int numOfLights);
		void incomingNoteEvent(int noteValue, int velocity, bool noteOn);
		long getColor(int lightId, long currentColor);	
		int getLight(int lightId);
		int getNumOfLights();
		int getChannel();
		void reset();
		
};
#endif