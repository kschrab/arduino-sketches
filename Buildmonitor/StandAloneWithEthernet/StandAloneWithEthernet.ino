#include <SPI.h>
#include <Ethernet.h>

#include <inttypes.h>
#include <TinyXML.h>
#include <WS2812.h>

#define PIN                      6 //DATA pin of the WS2812 LED
#define NUMPIXELS                1 //Number of LED RGBs connected

#define NUM_OF_BRANCHES          1 //Number of branches to show (should be the same as number of LEDs) (workaround: sizeof(builds) does not work properly)
#define NUM_OF_BUILDS_PER_BRANCH 2 //Number of builds to watch per branch 
#define CALL_INTERVALL           20 //Interval in seconds until the build status is requested again

String builds[NUM_OF_BRANCHES][NUM_OF_BUILDS_PER_BRANCH]  = { //collection of buildTypeIds for branches and their builds
    {//branch 0
      "branch0_build0",
      "branch0_build1"
    }
  };

byte mac[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }; //mac adress of the ethernet shield (should be printed on the shield)

IPAddress server(0,0,0,0); // IP addr of teamcity server (we don't want to use DNS)
int port = 8080; //port of teamcity server
String host = ""; //hostname of teamcity server, only used in HTTP request header

String auth = ""; //http auth: base 64 of "<username>:<password>"

/*--------------*/

boolean running[NUM_OF_BRANCHES];
boolean success[NUM_OF_BRANCHES]; 
int branchStatus[NUM_OF_BRANCHES];

EthernetClient client;

WS2812 LED(NUMPIXELS);
cRGB value;

TinyXML xml;
uint8_t buffer[150];
uint16_t buflen = 150;

boolean readFinished = true;
boolean requestFinished = true;

//some helper vars
long lastCall;
long lastCharMillis;
int buildIndex;
int branchIndex;
char c;
boolean xmlProcessing;
boolean xmlBegin;
int buildTagIndex = 0;
double mul;
boolean connectAttempt;

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
  buildIndex = 0;
  branchIndex = 0;
  xmlProcessing = false;
  xmlBegin = false;
  buildTagIndex = -1;
  lastCall = 0;
  connectAttempt = false;
  readFinished = true;
  
  
  client = EthernetClient();
  
  delay(1000); //wait
  
  for(int i=0;i<NUM_OF_BRANCHES;i++) {
     running[i] = false;
     success[i] = false;
     branchStatus[i] = 0;
  } 
}

void loop() {
  //Every time: define color for each LED
  for(int i=0;i<NUMPIXELS;i++){
    if(branchStatus[i] == 0) {
      value.r = 0; value.g = 0; value.b = 0;
    }else if(branchStatus[i] == 1) {
      mul = 1.0f-(0.5f+0.5f*sin((((double)(millis()%1000))*3.1415926535f)/500.0f))*0.5; //pulse
      value.r = ((double)100) * mul; value.g = ((double)40) * mul; value.b = 0;
    }else if(branchStatus[i] == 2) {
      mul = 1.0f-(0.5f+0.5f*sin((((double)(millis()%1000))*3.1415926535f)/500.0f))*0.5; //pulse
      value.r = ((double)100) * mul; value.g = ((double)10) * mul; value.b = 0;
    }else if(branchStatus[i] == 3) {
      value.r = 0; value.g = 150; value.b = 0;
    }else if(branchStatus[i] == 4) {
      value.r = 150; value.g = 0; value.b = 0;
    }
    LED.set_crgb_at(i, value);
    LED.sync();
  }
  //if all request have been fired, stop client and return. request again if call interval has been reached
  if(buildIndex == 0 && requestFinished && readFinished && lastCall > 0 && millis() < lastCall + CALL_INTERVALL * 1000 ) {
    if(client.connected()) { //drop the client since the server does not keep the connection alive forever
      Serial.println("Disconnecting");
      client.stop(); 
      connectAttempt = false;
    }
    return;
  }
  
  //connect to client, if disconnected
  if(readFinished && !client.connected() && !connectAttempt) {
    Serial.println("Connecting");
    client = EthernetClient();
    client.connect(server, port);
    connectAttempt = true;
    readFinished = true;
    requestFinished = true;
  }
  
  if(requestFinished && readFinished) {
    requestFinished = false;
  }
  
  //send request to server, if writing is allowed;
  if(!requestFinished && client.connected()){
    connectAttempt = false;
    
    if(buildIndex == 0) {
      //reset build status if requesting for the first branch
      running[branchIndex] = false; 
      success[branchIndex] = true;
    }  
    
    if(buildIndex == 0 && branchIndex == 0){
       lastCall = millis(); 
    }
    
    Serial.println("Requesting");
    
    xml.reset();
    xmlProcessing = false;
    xmlBegin = false;
    
    client.println("GET /httpAuth/app/rest/builds/?locator=buildType:"+builds[branchIndex][buildIndex]+",running:any,start:0,count:1 HTTP/1.1");
    client.println("Authorization: basic  " + auth);
    client.println("Host: "+host);
    client.println();
    client.println();
    
    lastCharMillis = millis();
    
    readFinished = false;  
    requestFinished = true;
  }  
  //read one char if there's is one available, and process it with XML-parser
  if(!readFinished && client.available()) {
    c = client.read();
//    Serial.print(c);
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
    }else{
      xmlBegin = false;
    }
  }
  
  //handle timeouts
  if(!readFinished && client.connected() && millis() - lastCharMillis > 2000) {
     Serial.println("Timeout");
     readFinished = true; 
  }
  
  //read finished: determine build-status (will be used later for LEDs) and increment buildIndex/branchIndex
  if(requestFinished && readFinished) {
    if(++buildIndex >= NUM_OF_BUILDS_PER_BRANCH) { //all builds of the branch have been read -> update status

             
       //update status of current branch
       if(success[branchIndex] && running[branchIndex]) {
         branchStatus[branchIndex] = 1;
       }else if(!success[branchIndex] && running[branchIndex]) {
         branchStatus[branchIndex] = 2;
       }else if(success[branchIndex] && !running[branchIndex]) {
         branchStatus[branchIndex] = 3;
       }else if(!success[branchIndex] && !running[branchIndex]) {
         branchStatus[branchIndex] = 4;
       }   
       
       Serial.print("Last complete build: ");
       Serial.println(branchStatus[branchIndex]);
       
       buildIndex = 0; //reset buildIndex to zero, if all builds have been read
       branchIndex = (branchIndex + 1) % NUM_OF_BRANCHES; //and continue with next branch
    }
  }
}


void XML_callback( uint8_t statusflags, char* tagName,  uint16_t tagNameLen,  char* data,  uint16_t dataLen ) {

  if (statusflags & STATUS_START_TAG) {
    //start tag
    if(strcmp(tagName, "/builds") == 0) {
      Serial.print("Parse response...");
      buildTagIndex = -1;
    }else if(strcmp(tagName, "/builds/build") == 0){
      buildTagIndex++;
    }
  } else if  (statusflags & STATUS_END_TAG) {
    //end tag
    if(strcmp(tagName, "/builds") == 0) {
      Serial.println("success");
      readFinished = true; //stop reading
    }
  } else if  (statusflags & STATUS_TAG_TEXT) {
    //end tag, nop
  } else if  (statusflags & STATUS_ATTR_TEXT) {
    if(strcmp(tagName, "status") == 0) {
      success[branchIndex] &= (strcmp(data,"SUCCESS")==0);
    }else if(strcmp(tagName, "state") == 0) {
      running[branchIndex] |= (strcmp(data,"running")==0);
    }
  } else if  (statusflags & STATUS_ERROR) {
    Serial.println("failed");
    readFinished = true; //stop reading
  }
}



