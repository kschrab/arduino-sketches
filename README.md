# arduino-sketches

Collection of personal arduino sketches / projects

## Buildmonitor

Shows the status of teamcity builds with RGB LEDs. Several versions exist:

#### Version 1:
A processing-script requests build status of teamcity and sends update-messages via serial connection to the Arduino, which then indicates the build status by lighting an LED.

* Hardware: Arduino UNO, WS2812-controlled RGB LED
* Libraries: WS2812

#### Version 2:
The Arduino requests the build status of teamcity builds by itself and uses several RGB LEDs to indicate the correspondive build status. The LEDs are arranged in an picture frame.

* Hardware: Arduino UNO, EthernetShield, WS2812-controlled RGB LEDs, picture frame
* Libraries: Ethernet, TinyXML, WS2812

#### Version 3:
A proxy server requests the build status of teamcity and sends light update packets to all connected clients. The Arduino connects to the proxy and retrieves those packets, and eventually updates its RGB LEDs. The LEDs are arranged in an picture frame.
* Hardware: Arduino UNO, EthernetShield, WS2812-controlled RGB LEDs, picture frame
* Libraries: Ethernet, WS2812

## Midi2Lightshow

A midi to light converter I use live on stage with my band, in order to control 50 RGB-LEDs by an MPC1000 via the MIDI protocol.

* Hardware: Arduino UNO, MidiBreakoutShield, WS2801-controlled RGB-LED-strip
* Libraries: Adafruit_WS2801
