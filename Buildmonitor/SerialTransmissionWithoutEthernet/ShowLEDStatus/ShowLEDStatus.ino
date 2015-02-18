#include <WS2812.h>

#define PIN            6
#define NUMPIXELS      1

WS2812 LED(NUMPIXELS);
cRGB value;

int incomingByte = 0;
int buildIndex = 0;
int buildStatus[NUMPIXELS];
double mul;

void setup() {
  LED.setOutput(PIN); 
  
  Serial.begin(9600);
  buildStatus[0] = 0;
}

void loop() {
  delay(10);
  
  while(Serial.available() > 0) {
    incomingByte = Serial.read();
  
    if((incomingByte & 0x80) == 0x80) {
      buildIndex = (incomingByte & 0x78) >> 3;
      
      if(buildIndex < NUMPIXELS){    
        buildStatus[buildIndex] = (incomingByte & 0x07);
      }
    }
  }

  mul = 1.0f-(0.5f+0.5f*sin((((double)(millis()%1000))*3.1415926535f)/500.0f))*0.5;

  for(int i=0;i<NUMPIXELS;i++){
    if(buildStatus[i] == 0) {
      value.r = 0; value.g = 0; value.b = 0;
    }else if(buildStatus[i] == 1) {
      value.r = ((double)100) * mul; value.g = ((double)40) * mul; value.b = 0;
    }else if(buildStatus[i] == 2) {
      value.r = ((double)100) * mul; value.g = ((double)10) * mul; value.b = 0;
    }else if(buildStatus[i] == 3) {
      value.r = 0; value.g = 150; value.b = 0;
    }else if(buildStatus[i] == 4) {
      value.r = 150; value.g = 0; value.b = 0;
    }
    LED.set_crgb_at(buildIndex, value);
    LED.sync();
  }
}
