import processing.serial.*;

import processing.net.*;
import javax.xml.bind.DatatypeConverter;

final int CALL_INTERVALL = 30;  
final int PORT_ID = 1;
final int SERIAL_RATE = 9600;

final String[] build1 = new String[]{
    "branch1_build1",
    "branch1_build2",
};

final String[][] builds = new String[][] { build1 };

final String auth = ""; //http auth base 64 <username>:<passwort>

String data;
boolean[] running;
boolean[] success; 
long lastCall;
int callIndex;
int buildIndex;
boolean readBlocked = true;
boolean writeBlocked = true;
Serial serial;
Client c;

void setup() {
  println(auth);
  String[] ports = Serial.list();
  for(int i=0;i<ports.length;i++){
     println(i+": "+ports[i]); 
  }
  if(ports.length > PORT_ID) {
    String portName = Serial.list()[PORT_ID];
    serial = new Serial(this, portName, SERIAL_RATE);
    //init-> set all to zero
    serial.write(0x80);
  }
  
  data = "";  
  callIndex = 0;
  buildIndex = 0;
  
  //shutdown hook
  Runtime.getRuntime().addShutdownHook(new Thread(new Runnable() {
    public void run () {
      if(serial != null){
        serial.stop();  
      }
      println("Stopped");
    }
  }));
  
  running = new boolean [builds.length];
  success = new boolean [builds.length];
}

void draw() {
  if(callIndex == 0 && writeBlocked && readBlocked && lastCall > 0 && millis() < lastCall + CALL_INTERVALL * 1000 ) {
    if(c != null) { //drop the client since the server does not keep the connection alive forever
       c.stop(); //BUG (?): prints allways an exception.. ignore it. can't event catch it
       c = null;
    }
    return;
  }
  
  if(c == null) {
   c = new Client(this, XXX, XXX); //set hostname + port
  }
  
  if(writeBlocked && readBlocked) {
    writeBlocked = false;
  }
  
  if(!writeBlocked){
    if(callIndex == 0) {
      running[buildIndex] = false; 
      success[buildIndex] = true;
    }  
    
    if(callIndex == 0 && buildIndex == 0){
       lastCall = millis(); 
    }
    
    c.write("GET /httpAuth/app/rest/builds/?locator=buildType:"+builds[buildIndex][callIndex]+",running:any,start:0,count:1 HTTP/1.1\n");
    c.write("Authorization:  basic  " + auth + "\n");
    c.write("Host: "+XXX+"\n\n"); //add host name
    
    readBlocked = false;  
    writeBlocked = true;
    data = "";
  }  
  if(!readBlocked) {
    if (c.available() > 0) {
      data += c.readString();
    }else if(data != null && data.length() > 0){
      int xmlStart = data.indexOf("<builds");
      int xmlEnd = data.indexOf("</builds>"); 
    
      if(xmlStart >= 0 && xmlEnd > 0) {
        readBlocked = true;
        
        xmlEnd += "</builds>".length();
        String xmlData = data.substring(xmlStart, xmlEnd);
        xmlData = xmlData.replaceAll("\n","").replaceAll("\r","");
        data = data.substring(xmlEnd);
        try {
          XML xml = parseXML(xmlData);
          if (xml == null) {
            println("XML could not be parsed.");
          } else {
            XML[] children = xml.getChildren("build");
            if(children.length > 0){
              success[buildIndex] &= children[0].getString("status").equals("SUCCESS");
              running[buildIndex] |= children[0].getString("state").equals("running");
            }
          }      
        } catch(Exception e) {
          println("XML could not be parsed.");
        }
      }
    }
  }
  
  if(writeBlocked && readBlocked) {
    if(++callIndex >= master.length) {
       callIndex = 0;
       
       int status = 0; 
       if(success[buildIndex] && running[buildIndex]) {
         status = 1;
       }else if(!success[buildIndex] && running[buildIndex]) {
         status = 2;
       }else if(success[buildIndex] && !running[buildIndex]) {
         status = 3;
       }else if(!success[buildIndex] && !running[buildIndex]) {
         status = 4;
       }   
       
       if(serial != null) {
         serial.write(0x80 | ((buildIndex << 3) & 0x78) | (status & 0x07)); //send byte [1####+++] -> #### = buildIndex, +++ = status 
       }
       println(millis() + ": Last complete build: "+status);
       
       buildIndex = (buildIndex + 1) % builds.length;
       
    }
  }
   
}
