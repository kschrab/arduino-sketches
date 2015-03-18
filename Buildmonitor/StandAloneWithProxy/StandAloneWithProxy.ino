#include <SPI.h>
#include <Ethernet.h>

#include <WS2812.h>

#define MAGIC_BYTE               0xF0 //magic byte used for identifying light update packets

#define LIGHT_STATUS             99 //arbitrary number > NUM_OF_BRANCHES

#define STATUS_SUCCESS_RUNNING   1
#define STATUS_FAILED_RUNNING    2
#define STATUS_SUCCESS_FINISHED  3
#define STATUS_FAILED_FINISHED   4

#define PIN_BUZZER               5 //DATA pin of the buzzer
#define PIN_LIGHT                6 //DATA pin of the WS2812 LED
#define NUMPIXELS                5 //Number of LED RGBs connected

#define NUM_OF_BRANCHES          4 //Number of branches to show (should be the same as number of LEDs) (workaround: sizeof(builds) does not work properly)

#define BUZZER_DURATION          400 //Duration of each buzzer tone (ms)

/*------ CONFIGURATION vars -------- */
byte mac[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; //mac adress of the ethernet shield (should be printed on the shield)

IPAddress server(###,###,###,###); // IP addr of proxy server which sends light update packets (we don't want to use DNS)
int port = ####; //port of the said server

int lightMap[NUMPIXELS] = { //custom mapping between pixels -> branch-ids. in this example we map light id  0 as status-light (optional), light 1 shows branch 1, etc.
  LIGHT_STATUS, 1, 2, 0, 3
};

/*------- required data structures -------*/
int branchStatus[NUM_OF_BRANCHES];
EthernetClient client;
WS2812 LED(NUMPIXELS);

/*----- helper vars ---------*/
cRGB value;
byte readIndex;
byte nextByte;
byte lightId;
byte lightState;
long buzzerStart;
double mul;
int i,l;

void setup() {  
  
  pinMode(PIN_BUZZER, OUTPUT);  
  LED.setOutput(PIN_LIGHT); 
  
  Serial.begin(9600);

  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    while(true); //do nothing if DHCP is not accepting
  }      

  //init helper vars
  readIndex = 0;    
  client = EthernetClient();
  
  for(int i=0;i<NUM_OF_BRANCHES;i++) {
     branchStatus[i] = 0;
  } 
  
  buzzer(false); //deactivate buzzer
  
  delay(1000); //wait some time to init
}

void loop() {
  //Do some on/off buzzing if neccessary
   buzzer(millis() - buzzerStart < BUZZER_DURATION || 
    (millis() - buzzerStart >= BUZZER_DURATION * 2 && millis() - buzzerStart < BUZZER_DURATION * 3)  || 
    (millis() - buzzerStart >= BUZZER_DURATION * 4 && millis() - buzzerStart < BUZZER_DURATION * 5));
  
  //Every time: define color for each LED
  for(i=0;i<NUMPIXELS;i++){
    mul = 1.0f-(0.5f+0.5f*sin((((double)(millis()%1000))*3.1415926535f)/500.0f))*0.75; //pulse
    l = lightMap[i];
    if(l == LIGHT_STATUS && !client.connected()) { //update status light if disconnected (red pulsating)
      value.r = ((double)20) * mul; value.g = 0; value.b = 0;
    } else if(l == LIGHT_STATUS && client.connected()) { //update status light if connected (green pulsating )
      value.r = 0; value.g = ((double)20) * mul; value.b = 0;
    } else if(l < 0 || l >= NUM_OF_BRANCHES || branchStatus[l] == 0) { //update LED light according to branch status (ff.)
      value.r = 0; value.g = 0; value.b = 0;
    }else if(branchStatus[l] == STATUS_SUCCESS_RUNNING) {
      value.r = ((double)150) * mul; value.g = ((double)120) * mul; value.b = 0;
    }else if(branchStatus[l] == STATUS_FAILED_RUNNING) {
      value.r = ((double)150) * mul; value.g = ((double)30) * mul; value.b = 0;
    }else if(branchStatus[l] == STATUS_SUCCESS_FINISHED) {
      value.r = 0; value.g = 200; value.b = 0;
    }else if(branchStatus[l] == STATUS_FAILED_FINISHED) {
      value.r = 200; value.g = 0; value.b = 0;
    }
    LED.set_crgb_at(i, value);
  }
  LED.sync();
  
  //if not connected, try to (re)connect
  if(!client.connected()) {
    client.connect(server, port);
  }
  
  if(client.connected()) {
    while(client.available() > 0) {
      nextByte = client.read();  
      if(nextByte == MAGIC_BYTE){ //Wait for magic byte of light update packet
        lightId = 0;
        lightState = 0;      
        readIndex = 1;
      }else if(readIndex == 1) { //read the ID of the light
        lightId = nextByte;
        readIndex = 2;
      }else if(readIndex == 2) { //read the new state of the light
        lightState = nextByte;
        readIndex = 3;
      }else if(readIndex == 3) {
        if(lightId >= 0 && lightId < NUM_OF_BRANCHES && nextByte == ############################) { //continue only if checksum is valid
           if((branchStatus[lightId] == STATUS_SUCCESS_RUNNING || branchStatus[lightId] == STATUS_SUCCESS_FINISHED) &&
               (lightState == STATUS_FAILED_RUNNING || lightState == STATUS_FAILED_FINISHED)) {
               //activate buzzer if a build of the branch failed or is going to fail
               buzzerStart = millis();
           }
           //update branch status
           branchStatus[lightId] = lightState;
        }
        //reset read
        readIndex = 0; 
      }
    }
  }
}

void buzzer(boolean buzz) {
   if(buzz) {
     analogWrite(PIN_BUZZER,10);     
   } else {
     analogWrite(PIN_BUZZER,0);
   }
}



