#include "Arduino.h"
#include "LightCluster.h"
#include "math.h"
#include <cstdlib>

const int LightCluster::durations[] = {6000,4000,2000,1000,600,400,200,100};
const float LightCluster::pulseFrequencies[] = {0.25f, 0.5f, 1.f, 2.f, 3.f, 4.f, 8.f, 16.f};
const float LightCluster::pulseDepths[] = {0.25f, 0.5f, 0.75f, 1.0f};
const float LightCluster::stroboFrequencies[] = {1.5f, 3.0f, 5.0f, 10.0, 15.0f, 20.0f, 30.0f, 50.0f};

LightCluster::LightCluster(int noteR, int noteG, int noteB, int noteCtrl, int *lights, int numOfLights) {
	this->noteR = noteR;
	this->noteG = noteG;
	this->noteB = noteB;
	this->noteCtrl = noteCtrl;
	this->oneHitMode = false;
	this->lights = lights;
	this->numOfLights = numOfLights;
	
	this->latestCommand = 0;
	this->latestR = 0;
	this->latestG = 0;
	this->latestB = 0;
	this->on = false;
	
	this->currentLight = -1;
}

LightCluster::LightCluster(int channel, int hitNode, int r, int g, int b, int mode, int duration, int offset, int *lights, int numOfLights) {
	this->latestR = r;
	this->latestG = g;
	this->latestB = b;
	this->noteCtrl = hitNode;
	this->latestCommand = mode;
	this->oneHitOffset = offset;
	this->oneHitDuration = duration;
	this->oneHitMode = true;
	this->lights = lights;
	this->numOfLights = numOfLights;
	
	this->on = false;
	
	this->currentLight = -1;
	this->channel = channel;
}

int LightCluster::getChannel() {
	return channel;
}

void LightCluster::incomingNoteEvent(int noteValue, int velocity, bool noteOn){
	if(oneHitMode && noteOn && noteValue == noteCtrl) {
		on = true;
		commandHit();
		lastCommandMillis = millis() + oneHitOffset;	
		return;
	}

	if(noteOn) {		
		if(noteValue == noteR) {
			latestR = (velocity == 1) ? 0 : 255*velocity/127;
		}else if(noteValue == noteG) {
			latestG = (velocity == 1) ? 0 : 255*velocity/127;
		}else if(noteValue == noteB) {
			latestB = (velocity == 1) ? 0 : 255*velocity/127;
		}
		
		if(noteValue == noteCtrl) {
			if(velocity == 127) {
				velocity = latestCommand; //keep latest if 127
			}
			if(velocity == latestCommand) {
				hitCounter++;
			}else{
				hitCounter = 0;
			}
			commandHit();
			latestCommand = velocity;
			on = true;
			lastCommandMillis = millis();
		}
	}else if(noteValue == noteCtrl) {
		on = false;
		lastCommandMillis = millis();
	}
}

int LightCluster::getNumOfLights() {
	return this->numOfLights;
}

int LightCluster::getLight(int lightId) {
	return *(lights+lightId);
}

long LightCluster::getColor(int lightId, long currentColor) {

	long millisSinceLastCommand = millis() - lastCommandMillis;
	if(millisSinceLastCommand < 0) {
		return currentColor;
	}

	if(oneHitMode && on && millisSinceLastCommand > oneHitDuration) {
		on = false;
		lastCommandMillis = millis();
	}

	int internalId = getLight(lightId);
	
	/** TURN OFF (0) **/
	if(latestCommand == 0) {
		return 0;
	}
	
	/** FADE IN (1-7), FADE OUT (8-15), FADE IN+ FADE OUT (16-23)**/
	if(latestCommand > 0 && latestCommand <= 23) {
		int duration = durations[latestCommand%8];
		
		float mul = 1.0f;
		if(on && (latestCommand < 8 || latestCommand >= 16) && millisSinceLastCommand < duration) { //fade in
			mul = (float)millisSinceLastCommand / (float)duration;
		}else if(on && (millisSinceLastCommand >= duration || ( latestCommand >= 8 && latestCommand < 16) )){ //no fade
			mul = 1.0f;
		}else if(!on && millisSinceLastCommand < duration && latestCommand >= 8) { //fade out
			mul = 1.0f - ((float)(millisSinceLastCommand) / (float)duration);
		}else{ //turn off
			mul = 0;
			on = false;
		}		
		mul = mul*mul; //looks much better! :D
		return color(mul * latestR, mul * latestG, mul * latestB);
	}

	/** STROBO (24-31)**/
	if(on && latestCommand > 23 && latestCommand <= 31) { 
		float frequency = stroboFrequencies[latestCommand%8];
		if(millisSinceLastCommand % (int)(2000/frequency) > 1000/frequency) {
			return color(latestR, latestG, latestB);			
		}else {
			return 0;
		}
	}
	
	/** PULSE -25% (32-39), -50% (40-47), -75% (48-55), -100% (56-63) **/
	if((on || millisSinceLastCommand < 20) && latestCommand > 31 && latestCommand <= 63) {
		double frequency = pulseFrequencies[latestCommand%8];
		double depth = pulseDepths[(latestCommand-32)/8];
		double mul = 1.0f-(0.5f+0.5f*sin((((double)(millisSinceLastCommand%1000))*frequency*3.1415926535f)/500.0f))*depth; //science!
		return color(mul * latestR, mul * latestG, mul * latestB);
	}

	/** STEP LIGHT (64 - 71) & STEP RANDOM (72 - 79) */
	if(latestCommand > 63 && latestCommand <= 79) {
		int duration = durations[latestCommand%8]; //fade out duration
		float mul;
		if(getLight(currentLight) == internalId) {
			if(on) {
				mul =1.0f;
			}else if(!on &&  millisSinceLastCommand < duration) {
				mul = 1.0f-((float)millisSinceLastCommand / (float)duration);
			}else{
				on = false;
				mul = 0.0f;
			}
			mul = mul*mul;
			return color(mul * latestR, mul * latestG, mul * latestB);
		}else {
			return 0;
		}
	}
	
	
	// /** RANDOM LIGHTS STROBO (61-65)**/
	if(on && latestCommand > 79 && latestCommand <= 87) { 
		float frequency = stroboFrequencies[latestCommand%8];
		if(millisSinceLastCommand % (int)(2000/frequency) > 1000/frequency) {
			return color(randomNumber(0,latestR), randomNumber(0,latestG), randomNumber(0,latestB));
		}else {
			return 0;
		}
	}

	if(on && latestCommand > 87 && latestCommand <= 95) { 
		return randomColor;
	}
	
	
	if(!on) { //simple on/off
		return 0;
	}else{
		return color(latestR, latestG, latestB);
	}
}

void LightCluster::commandHit() {
	/** STEP LIGHT (64 - 71) */
	if(latestCommand > 63 && latestCommand <= 71) {
		currentLight = hitCounter++ % numOfLights;
	}

	/** STEP RANDOM (72 - 79) */
	if(latestCommand > 71 && latestCommand <= 79) {
		if(numOfLights > 1 ) {
			int current = currentLight;
			while(current == currentLight) {
				currentLight = min(numOfLights - 1, randomNumber(0, numOfLights));
			}
		}else{
			currentLight = 0;
		}
	}

	if(latestCommand > 87 && latestCommand <= 95) {
		randomColor = color(randomNumber(0, latestR),randomNumber(0, latestG),randomNumber(0, latestB));
	}


}

void LightCluster::reset() {
	on = false;
	if(!oneHitMode) {
		latestCommand = 0;
		latestB = 0;
		latestG = 0;
		latestR = 0;
	}
}

int LightCluster::randomNumber(int min, int max){
	if(max <= min){
		return min;
	}
	return min + (rand() % (int)(max - min + 1));
}



long LightCluster::color(int r, int g, int b) {
	long c;
	c = r;
	c <<= 8;
	c |= g;
	c <<= 8;
	c |= b;
	return c;
}