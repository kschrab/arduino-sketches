#include "Adafruit_WS2801.h"
#include "SPI.h" // Comment out this line if using Trinket or Gemma
#include "LightCluster.h"
#ifdef __AVR_ATtiny85__
#include <avr/power.h>
#endif

#define OFF 1
#define ON 2
#define CC 4
#define WAIT 9


byte incomingByte;
int data1;
int data2;

int action;
int channel;
byte command;


uint8_t dataPin  = 11;    // White wire on Adafruit Pixels
uint8_t clockPin = 13;    // Green wire on Adafruit Pixels
// Connect the ground wire to Arduino ground and the +5V wire to a +5V supply

//strip with 50 leds
Adafruit_WS2801 strip = Adafruit_WS2801(50, dataPin, clockPin);

int lights_block1[] = {
  0,1,2,3,4,5,6,7,8,9};
int lights_block2[] = {
  10,11,12,13,14,15,16,17,18,19};
int lights_block3[] = {
  20,21,22,23,24,25,26,27,28,29};
int lights_block4[] = {
  30,31,32,33,34,35,36,37,38,39};
int lights_block5[] = {
  40,41,42,43,44,45,46,47,48,49};

int lights_all[] = {
  0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49};
int lights_third_1[] = {
  0,3,6,9,12,15,18,21,24,27,30,33,36,39,42,45,48};
int lights_third_2[] = {
  1,4,7,10,13,16,19,22,25,28,31,34,37,40,43,46,49};
int lights_third_3[] = {
  2,5,8,11,14,17,20,23,26,29,32,35,38,41,44,47};


int numOfClusters = 9; //keep updated!!
LightCluster clusters[] = {
  LightCluster(35,36,37,38,lights_block1, 10),
  LightCluster(39,40,41,42,lights_block2, 10),
  LightCluster(43,44,45,46,lights_block3, 10),
  LightCluster(47,48,49,50,lights_block4, 10),
  LightCluster(51,52,53,54,lights_block5, 10),
  LightCluster(55,56,57,58,lights_third_1, 17),
  LightCluster(59,60,61,62,lights_third_2, 17),
  LightCluster(63,64,65,66,lights_third_3, 16),
  LightCluster(67,68,69,70,lights_all, 50)
  };

boolean updatedLights[] = {
  false,false,false,false,false,false,false,false,false,false,
  false,false,false,false,false,false,false,false,false,false,
  false,false,false,false,false,false,false,false,false,false,
  false,false,false,false,false,false,false,false,false,false,
  false,false,false,false,false,false,false,false,false,false}; //50
  
  
int numSingleHits = 11; //keep updated!!
LightCluster singleHits[] = {
  LightCluster(1,48,240,0,0,11,60,0,lights_third_1, 17),//regret bd = fade out red
  LightCluster(1,47,80,30,80,85,375,750,lights_third_2, 17),//regret snare = random light strobo (250ms offset)
  LightCluster(1,43,90,0,90,95,2250,0,lights_third_3, 16),
  LightCluster(1,49,60,10,10,95,1125,0,lights_third_3, 16),
  LightCluster(1,55,10,60,10,95,1125,0,lights_third_3, 16),
  LightCluster(1,51,10,10,60,95,1125,0,lights_third_3, 16),
  LightCluster(1,53,60,10,60,95,1125,0,lights_third_3, 16),
  
  LightCluster(2,52,100,0,100,13,10,0,lights_third_1, 17),
  LightCluster(2,57,180,150,150,14,10,0,lights_block1, 10),
  LightCluster(2,57,180,150,150,14,10,0,lights_block4, 10),
  
  LightCluster(2,58,20,5,5,71,12,0,lights_third_2, 16)
    
    
};

void setup() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
  clock_prescale_set(clock_div_1); // Enable 16 MHz on Trinket
#endif

  strip.begin();  
  strip.show();

  Serial.begin(31250);  

  action = WAIT;
  data1 = -1;
  data2 = -1;
}


void loop() {
  //read notes via serial connection
  if(Serial.available() > 0) { //read 3 bytes
    incomingByte = Serial.read();

    command = incomingByte & 0xF0; //extract status byte

    if (command == 0x90) { // Note on
      action = ON;
      channel = incomingByte & 0x0F;
    } 
    else if (command == 0x80)  {  // Note off
      action = OFF;
      channel = incomingByte & 0x0F;
    } 
    else if (command == 0xB0)  {  // Note off
      action = CC;
      channel = incomingByte & 0x0F;
    } 
    else if (data1 < 0 && action != WAIT) { // wait for note value
      data1=(int)incomingByte;
    } 
    else if (data1 >=0 && action != WAIT) { // wait for velocity
      data2=(int)incomingByte;

      if(action == ON && data2 == 0) { //some midi controlles send "on" with zero velocity instead of "off"-events
        action = OFF;
      }

      if(channel == 0) { //channel 1 == light clusters
        if(action == ON){ 
          incomingNote(data1, data2, true);
        }
        else if(action == OFF) {
          incomingNote(data1, data2, false);
        }
        else if(action == CC && data1 == 0x7B) { //turn everything off
          resetAllClusters();
        }
      }

      if(channel >= 1) { //channel 2 == single notes
        //TODO
        if(action == ON){ 
          singleHit(channel, data1);
        }else if(action == CC && data1 == 0x7B) { //turn everything off
           for(int i=0;i<numSingleHits;i++) {
            singleHits[i].reset();
           } 
        }
      }

      data1=-1;
      data2=-1;
      channel = -1;
      action=WAIT; 
    }

  }
  if(action == WAIT) { //update each 2 ms
    update();   
  }

}

void resetAllClusters() {
  for(int i=0;i<numOfClusters;i++) {
    clusters[i].reset();
  } 
}

void incomingNote(int note, int velocity, boolean noteOn) {
  for(int i=0;i<numOfClusters;i++) {
    clusters[i].incomingNoteEvent(note, velocity, noteOn);
  } 
}

void singleHit(int channel, int note) {
  for(int i=0;i<numSingleHits;i++) {
    if(singleHits[i].getChannel() == channel) {
      singleHits[i].incomingNoteEvent(note, 127, true);
    }

  } 
}



void update() {
  int i;
  for(i=0;i<strip.numPixels();i++) {
    updatedLights[i] = false;
  }

  for(i=0;i<numOfClusters;i++) {
    for(int j=0;j<clusters[i].getNumOfLights();j++) {
      int light = clusters[i].getLight(j);   
      long currentColor = strip.getPixelColor(light);
      long color = clusters[i].getColor(j, currentColor); 
      if(!updatedLights[light] || color > 0) {//avoids that overlaying clusters deactivate pixels
        strip.setPixelColor(light, color);              
        updatedLights[light] = true; 
      }        
    } 
  } 
  
  for(i=0;i<numSingleHits;i++) {
    for(int j=0;j<singleHits[i].getNumOfLights();j++) {
      int light = singleHits[i].getLight(j);   
      long currentColor = strip.getPixelColor(light);
      long color = singleHits[i].getColor(j, currentColor); 
      if(!updatedLights[light] || color > 0) {//avoids that overlaying clusters deactivate pixels
        strip.setPixelColor(light, color);              
        updatedLights[light] = true; 
      }        
    } 
  } 
  strip.show();
}


