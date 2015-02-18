#include <SPI.h>
#include <Ethernet.h>

#include <inttypes.h>
#include <TinyXML.h>
#include <WS2812.h>

#define PIN                      6
#define NUMPIXELS                1

#define NUM_OF_BUILDS_PER_BRANCH 2 
#define CALL_INTERVALL           30  

String builds[][NUM_OF_BUILDS_PER_BRANCH]  = { 
    {
        "branch1_build1", //RENAME THOSE TO YOUR BUILD-TYPE-IDS
        "branch1_build1"
    }
  };

byte mac[] = { xx,xx,xx,xx,xx,xx }; //mac adress of ethernet shield

IPAddress server(x,x,x,x);// IP addr of buildserver
EthernetClient client;

String auth = ""; //http base 64 auth

WS2812 LED(NUMPIXELS);
cRGB value;

TinyXML xml;
uint8_t buffer[150];
uint16_t buflen = 150;

boolean running[sizeof(builds)];
boolean success[sizeof(builds)]; 
int buildStatus[sizeof(builds)];

boolean readBlocked = true;
boolean writeBlocked = true;

//some helper vars
long lastCall;
long lastCharMillis;
int callIndex;
int buildIndex;
char c;
boolean xmlProcessing;
boolean xmlBegin;
int buildTagIndex = 0;
double mul;

void setup() {  
  
  LED.setOutput(PIN); 
  
  Serial.begin(9600);

  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    while(true); //do nothing if DHCP is not accepting
  }    

  //init xml
  xml.init((uint8_t*)&buffer,buflen,&XML_callback);
  
  //init helper vars
  callIndex = 0;
  buildIndex = 0;
  xmlProcessing = false;
  xmlBegin = false;
  buildTagIndex = -1;
  lastCall = 0;
  
  delay(1000); //wait
}

void loop() {
  //Every time: define color for each LED
  for(int i=0;i<NUMPIXELS;i++){
    if(buildStatus[i] == 0) {
      value.r = 0; value.g = 0; value.b = 0;
    }else if(buildStatus[i] == 1) {
      mul = 1.0f-(0.5f+0.5f*sin((((double)(millis()%1000))*3.1415926535f)/500.0f))*0.5; //pulse
      value.r = ((double)100) * mul; value.g = ((double)40) * mul; value.b = 0;
    }else if(buildStatus[i] == 2) {
      mul = 1.0f-(0.5f+0.5f*sin((((double)(millis()%1000))*3.1415926535f)/500.0f))*0.5; //pulse
      value.r = ((double)100) * mul; value.g = ((double)10) * mul; value.b = 0;
    }else if(buildStatus[i] == 3) {
      value.r = 0; value.g = 150; value.b = 0;
    }else if(buildStatus[i] == 4) {
      value.r = 150; value.g = 0; value.b = 0;
    }
    LED.set_crgb_at(i, value);
    LED.sync();
  }
  
  //if all request have been fired, stop client and return. request again if call interval has been reached
  if(callIndex == 0 && writeBlocked && readBlocked && lastCall > 0 && millis() < lastCall + CALL_INTERVALL * 1000 ) {
    if(client.connected()) { //drop the client since the server does not keep the connection alive forever
      Serial.println("Disconnecting");
      client.stop(); 
    }
    return;
  }
  
  //connect to client, if disconnected
  if(!client.connected()) {
    Serial.println("Connecting");
    client.connect(server, xxx); //ADD PORT
  }
  
  if(writeBlocked && readBlocked && client.connected()) {
    writeBlocked = false;
  }
  
  //send request to server, if writing is allowed;
  if(!writeBlocked){
    if(callIndex == 0) {
      running[buildIndex] = false; 
      success[buildIndex] = true;
    }  
    
    if(callIndex == 0 && buildIndex == 0){
       lastCall = millis(); 
    }
    
    Serial.println("Requesting");
    
    client.println("GET /httpAuth/app/rest/builds/?locator=buildType:"+builds[buildIndex][callIndex]+",running:any,start:0,count:1 HTTP/1.1");
    client.println("Authorization:  basic  " + auth);
    client.println("Host: "+xxx);//hostname
//    client.println("Connection: close");
    client.println();
    
    readBlocked = false;  
    writeBlocked = true;
  }  
  
  //read one char if there's is one available, and process it with XML-parser
  if(!readBlocked && client.available()) {
    c = client.read();
    lastCharMillis = millis();
    if(xmlProcessing) {
      xml.processChar(c);
    }
    if(xmlBegin && c=='?') {
      xmlProcessing = true;
      xmlBegin = false;
      xml.processChar('<');
      xml.processChar('?');
    }
    if (!xmlProcessing && c=='<') {
      xmlBegin = true;
    }
  }
  
  //handle time outs
  if(!readBlocked && client.connected() && lastCharMillis - millis() > 1000) {
    readBlocked = true;
    xmlProcessing = false;
    xmlBegin = false;
    Serial.println("timeout");
    client.stop();
  }
  
  //read finished: determine build-status (will be used later for LEDs) and increment callIndex/buildIndex
  if(writeBlocked && readBlocked) {
    if(++callIndex >= NUM_OF_BUILDS_PER_BRANCH) { //all builds of the branch have been read -> update status
       callIndex = 0;
       
       if(success[buildIndex] && running[buildIndex]) {
         buildStatus[buildIndex] == 1;
       }else if(!success[buildIndex] && running[buildIndex]) {
         buildStatus[buildIndex] == 2;
       }else if(success[buildIndex] && !running[buildIndex]) {
         buildStatus[buildIndex] == 3;
       }else if(!success[buildIndex] && !running[buildIndex]) {
         buildStatus[buildIndex] == 4;
       }   
       
       Serial.println("Last complete build: "+buildStatus[buildIndex]);
       
       buildIndex = (buildIndex + 1) % sizeof(builds); //next build
    }
  }
}


void XML_callback( uint8_t statusflags, char* tagName,  uint16_t tagNameLen,  char* data,  uint16_t dataLen ) {
  if (statusflags & STATUS_START_TAG) {
    //start tag
    if(tagName == "builds") {
      Serial.print("Parse response...");
      buildTagIndex = -1;
    }else if(tagName == "build"){
      buildTagIndex++;
    }
  } else if  (statusflags & STATUS_END_TAG) {
    //end tag
    if(tagName == "builds") {
      Serial.println("success");
      readBlocked = true;
      xmlProcessing = false;
      xmlBegin = false;
    }
  } else if  (statusflags & STATUS_TAG_TEXT) {
    //end tag, nop
  } else if  (statusflags & STATUS_ATTR_TEXT) {
    if(tagName == "status") {
      success[buildIndex] &= (data == "SUCCESS");
    }else if(tagName == "state") {
      running[buildIndex] |= (data == "running");
    }
  } else if  (statusflags & STATUS_ERROR) {
    Serial.println("failed");
    readBlocked = true;
    xmlProcessing = false;
    xmlBegin = false;
  }
}


