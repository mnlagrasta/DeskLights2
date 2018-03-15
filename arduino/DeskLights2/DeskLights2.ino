/*

### Parameter Explanation
All exposed function calls take 0 or more single letter parameters:
r: red value (0 - 255)
g: green value (0 - 255)
b: blue value (0 - 255)
h: hex string representing color (000000 - ffffff)
x: x coordinate
y: y coordinate
d: delay in milliseconds
i: grid id of pattern/pixel (grid 0,0 = 0; grid 1,0 = 1, etc)
n: number of pixel on strip, starts at 0
s: call show() command automatically (0 or 1, defaults to 1)

Note: All color parameters can be specified as either RGB or hex

### Public Functions
off : Turn off all pixels
show : show any set, but not yet shown pixels
color: set all pixels to color; takes color
wipe: like color, but down the strip; optional delay
alert: flash all pixels; takes color and optional delay
pixel: set pixel to color; takes id or number, color, and optional show
default: set default pattern to loop, takes pattern id
gridtest: show grid pixel by pixel, no params
lighttest: show rgb on all pixels, no params

### Example

Here's an example using alert...

Flash bright white for default length of time 
http://server/alert?r=255&g=255&b=255
or
http://server/alert?h=ffffff
or with a 1 second duration
http://server/alert?h=ffffff&d=1000

*/

#define WEBDUINO_SERIAL_DEBUGGING 1
#define WEBDUINO_FAIL_MESSAGE "NOT ok\n"
#define WEBDUINO_COMMANDS_COUNT 10 //this defaults to 8 in WebServer.h
#include "SPI.h"
#include "avr/pgmspace.h"
#include "Ethernet.h"
#include "WebServer.h"
#include <Adafruit_WS2801.h>

/*** This is what you will almost certainly have to change ***/

// WEB stuff
static uint8_t mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x27, 0x05 }; // update this to match your arduino/shield
static uint8_t ip[] = { 10, 10, 16, 211 }; // update this to match your network

// LED Stuff
uint8_t dataPin = 2; // Yellow wire on Adafruit Pixels
uint8_t clockPin = 3; // Green wire on Adafruit Pixels
#define STRIPLEN 60
Adafruit_WS2801 strip = Adafruit_WS2801(STRIPLEN, dataPin, clockPin, WS2801_GRB);
int defaultPattern = 0;

// LED Grid stuff
int max_x = 9;
int max_y = 5;

//// 'screen' style x,y where 0,0 is top left
//int grid[STRIPLEN] = {
//   0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
//  19,18,17,16,15,14,13,12,11,10,
//  20,21,22,23,24,25,26,27,28,29,
//  39,38,37,36,35,34,33,32,31,30,
//  40,41,42,43,44,45,46,47,48,49,
//  59,58,57,56,55,54,53,52,51,50
//};


//// 'plot' style x,y where 0,0 is bottom left
int grid[STRIPLEN] = {
  59,58,57,56,55,54,53,52,51,50,
  40,41,42,43,44,45,46,47,48,49,
  39,38,37,36,35,34,33,32,31,30,
  20,21,22,23,24,25,26,27,28,29,
  19,18,17,16,15,14,13,12,11,10,
   0, 1, 2, 3, 4, 5, 6, 7, 8, 9
};


/*** Things you might want to change ***/

// basic web auth, not super secure without https
// see example at https://github.com/sirleech/Webduino
// use auth?
#define AUTH 0
// credentials (in base64 user:pass)
#define CRED "dXNlcjpwYXNz"

WebServer webserver("", 80); // port to listen on

// ROM-based messages for webduino lib, maybe overkill here
P(ok) = "ok\n";
P(noauth) = "User Denied\n";

// max length of param names and values
#define NAMELEN 2
#define VALUELEN 361

/*** Below here shouldn't need to change ***/

void log(char * input) {
	if (1) {
		Serial.println(input);
	}
}

// LED support functions

// create the "Color" value from rgb...This is right from Adafruit
uint32_t Color(byte r, byte g, byte b) {
	uint32_t c;
	c = r;
	c <<= 8;
	c |= g;
	c <<= 8;
	c |= b;
	return c;
}

// create a "Color" value from a hex string (no prefix)
// for example: ffffff
uint32_t hexColor(char * in) {
	return strtol(in, NULL, 16);
}

//Input a value 0 to 255 to get a color value.
//The colours are a transition r - g -b - back to r
uint32_t Wheel(byte WheelPos) {
	if (WheelPos < 85) {
		return Color(WheelPos * 3, 255 - WheelPos * 3, 0);
	} else if (WheelPos < 170) {
		WheelPos -= 85;
		return Color(255 - WheelPos * 3, 0, WheelPos * 3);
	} else {
		WheelPos -= 170;
		return Color(0, WheelPos * 3, 255 - WheelPos * 3);
	}
}

// set all pixels to a "Color" value
void colorAll(uint32_t c) {
	for (int i=0; i < strip.numPixels(); i++) {
		strip.setPixelColor(i, c);
	}
	strip.show();
}

// set all pixels to a "Color" value, one at a time, with a delay
void colorWipe(uint32_t c, uint8_t wait) {
	for (int i=0; i < strip.numPixels(); i++) {
		strip.setPixelColor(i, c);
		strip.show();
		delay(wait);
	}
}

// fade from one color to another: UNFINISHED
void fade(uint32_t c1, uint32_t c2, int wait) {
	if (c1 < c2) {
		while (c1 < c2) {
			colorAll(c1++);
			delay(wait);
		}
	} else {
		while (c1 > c2) {
			colorAll(c1--);
			delay(wait);
		}
	}
}

// this takes x/y coordinates and maps it to a pixel offset
// your grid will need to be updated to match your pixel count and layout
int g2p(int x, int y) {
	return grid[x + (y * (max_x + 1))];
}

// flash color "c" for "wait" ms
void alert(uint32_t c, int wait) {
	log("executing alert");
	colorAll(c);
	delay(wait);
	colorAll(Color(0,0,0));
}

// show the grid to verify
void gridTest(int wait) {
	int x;
	int y;
	uint32_t on = Color(255,255,255);
	uint32_t off = Color(0,0,0);
	
	if (!wait) {
		wait = 250;
	}
	
	for ( x = 0; x <= max_x; x++) {
		for ( y = 0; y <= max_y; y++) {
			strip.setPixelColor(g2p(x,y), on);
			strip.show();
			delay(wait);
			strip.setPixelColor(g2p(x,y), off);
			strip.show();
		}
	}
}

// wipe the major colors through all pixels
void lightTest(int wait) {
	colorWipe(Color(255, 0, 0), wait);
	colorWipe(Color(0, 255, 0), wait);
	colorWipe(Color(0, 0, 255), wait);
	colorWipe(Color(255, 255, 255), wait);
	colorWipe(Color(0, 0, 0), wait);
}

// next are the patterns, meant to loop
// use caution here, these block the server listening

// random pixel, random color
// short pattern, very responsive
void p_random (int wait) {
	strip.setPixelColor(
		random(0, strip.numPixels()),
		Color(random(0,255), random(0,255), random(0,255))
	);
	strip.show();
	delay(wait);
}

// If you were at maker faire, you know this pattern
// it takes about a second to run, so new requests will wait
void p_rainbow() {
	int i, j;
	for (j=0; j < 256; j++) {
		for (i=0; i < strip.numPixels(); i++) {
			strip.setPixelColor(i, Wheel( ((i * 256 / strip.numPixels()) + j) % 256) );
		}
		strip.show();
	}
}

// cylon or K.I.T.T. whichever 
void p_cylon() {
	int x;
	int wait=75;
	
	uint32_t c[6] = {
		Color(255,0,0),
		Color(200,0,0),
		Color(150,0,0),
		Color(100,0,0),
		Color(50,0,0),
		Color(0,0,0),
	};
	
	for (x=0; x <= max_x; x++) {
		int mod = 0;
		while ((mod < 6) && (x - mod >= 0)) {
			int y = 0;
			while (y <= max_y) {
				strip.setPixelColor(g2p(x-mod,y++), c[mod]);
			}
			mod++;
		}
		strip.show();
		delay(wait);
	}
	
	for (x=max_x; x >= 0; x--) {
		int mod = 0;
		while ((mod < 6) && (x + mod <= max_x)) {
			int y = 0;
			while (y <= max_y) {
				strip.setPixelColor(g2p(x+mod,y++), c[mod]);
			}
			mod++;
		}
		strip.show();
		delay(wait);
	}

}

// begin web handlers

int auth(WebServer &server) {
	if (AUTH) {
		if (!server.checkCredentials(CRED)) {
			server.printP(noauth);
			return 0;
		}
	}
	server.httpSuccess();
	return 1;
}

void cmd_index(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) {
	if (!auth(server)) { return;}
	server.printP(ok);
}

void my_failCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
	if (!auth(server)) { return;}
	server.httpFail();
}

void cmd_off(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) {
	if (!auth(server)) { return;}
	colorAll(Color(0,0,0));
	server.printP(ok);
}

void cmd_color(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) {
	if (!auth(server)) { return;}
	int r;
	int g;
	int b;
	uint32_t c;
	int use_hex = 0;
	
	URLPARAM_RESULT rc;
	char name[NAMELEN];
	char value[VALUELEN];
	
	while (strlen(url_tail)) {
		rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
		if ((rc != URLPARAM_EOS)) {
			switch(name[0]) {
				case 'h':
					c = hexColor(value);
					use_hex = 1;
					break;
				case 'r':
					b = atoi(value);
					break;
				case 'g':
					b = atoi(value);
					break;
				case 'b':
					b = atoi(value);
					break;
			}
		}
	}

	if (!use_hex) {
		c = Color(r,g,b);
	}
	colorAll(c);
	server.printP(ok);
}

void cmd_wipe(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) {
	if (!auth(server)) { return;}
	int r;
	int g;
	int b;
	int delay;
	uint32_t c;
	int use_hex = 0;

	URLPARAM_RESULT rc;
	char name[NAMELEN];
	char value[VALUELEN];
	
	while (strlen(url_tail)) {
		rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
		if ((rc != URLPARAM_EOS)) {
			switch(name[0]) {
				case 'r':
					r = atoi(value);
					break;
				case 'g':
					g = atoi(value);
					break;
				case 'b':
					b = atoi(value);
					break;
				case 'h':
					c = hexColor(value);
					use_hex++;
					break;
				case 'd':
					delay = atoi(value);
					break;
			}
		}
	}
	
	if (!use_hex) {
		c = Color(r,g,b);
	}
	
	colorWipe(c, delay);
	server.printP(ok);
}

void cmd_default(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) {
	if (!auth(server)) { return;}

	URLPARAM_RESULT rc;
	char name[NAMELEN];
	char value[VALUELEN];
	while (strlen(url_tail)) {
		rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
		if ((rc != URLPARAM_EOS)) {
			if (name[0] == 'i') {
				defaultPattern = atoi(value);
				colorAll(Color(0,0,0));
			}
		}
	}
	
	server.printP(ok);
}

void cmd_alert(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) {
	if (!auth(server)) { return;}
	int r;
	int g;
	int b;
	int d;
	int use_hex;
	uint32_t c;

	URLPARAM_RESULT rc;
	char name[NAMELEN];
	char value[VALUELEN];
	while (strlen(url_tail)) {
		rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
		if ((rc != URLPARAM_EOS)) {
			switch(name[0]) {
				case 'h':
					c = hexColor(value);
					use_hex = 1;
					break;
				case 'r':
					b = atoi(value);
					break;
				case 'g':
					b = atoi(value);
					break;
				case 'b':
					b = atoi(value);
					break;
				case 'd':
					d = atoi(value);
					break;
			}
		}
	}
	
	if (!use_hex) {
		c = Color(r,g,b);
	}
	
	if (!d) {
		d = 100;
	}
	
	alert(c, d);
	server.printP(ok);
}

void cmd_show(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) {
	if (!auth(server)) { return;}
	strip.show();
	server.printP(ok);
}

void cmd_test(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) {
	if (!auth(server)) { return;}
	
	URLPARAM_RESULT rc;
	char name[NAMELEN];
	char value[VALUELEN];
	int id;
	int d;
	while (strlen(url_tail)) {
		rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
		if ((rc != URLPARAM_EOS)) {
			switch(name[0]) {
				case 'i':
					id = atoi(value);
					break;
				case 'd':
					d = atoi(value);
					break;
			}
		}
	}
	
	switch(id) {
		case 0:
			lightTest(d);
			break;
		case 1:
			gridTest(d);
			break;
	}
	
	server.printP(ok);
}

void cmd_pixel(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) {
	if (!auth(server)) { return;}
	
	int id;
	int gid;
	int x;
	int y;
	int r;
	int g;
	int b;
	int s = 1;
	uint32_t c;
	int use_hex = 0;
	int use_id = 0;
	int use_gid = 0;

	URLPARAM_RESULT rc;
	char name[NAMELEN];
	char value[VALUELEN];
	while (strlen(url_tail)) {
		rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
		if ((rc != URLPARAM_EOS)) {
			log(name);
			log(value);
			switch(name[0]) {
				case 'i':
					gid = atoi(value);
					use_gid = 1;
					break;
				case 'n':
					id = atoi(value);
					use_id = 1;
					break;
				case 'x':
					x = atoi(value);
					break;
				case 'y':
					y = atoi(value);
					break;
				case 'h':
					c = hexColor(value);
					use_hex = 1;
					break;
				case 'r':
					r = atoi(value);
					break;
				case 'g':
					g = atoi(value);
					break;
				case 'b':
					b = atoi(value);
					break;
				case 's':
					s = atoi(value);
					break;
			}
		}
	}
	
	if (!use_id) {
		if (use_gid) {
			id = grid[gid];
		} else {
			id = g2p(x,y);
		}
	}
	
	if (!use_hex) {
		c = Color(r,g,b);
	}
	
	strip.setPixelColor(id, c);
	
	if (s) {
		strip.show();
	}
	
	server.printP(ok);
}

void cmd_frame(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) {
  URLPARAM_RESULT rc;
  char name[NAMELEN];
  char value[VALUELEN];

  if (!auth(server)) { return;}

  if (type == WebServer::POST) {
    while (server.readPOSTparam(name, NAMELEN, value, VALUELEN)) {

      // loop through value, getting 6 chars at a time
      // convert hex to color and set pixel at current id
      // repeat until input is exhausted or max pixels
      int o = 0;
      int id = 0;
      while (( o < sizeof(value) ) && ( id < STRIPLEN)) {
        char hc[7];
        int ho;
        for (ho = 0; ho < 6; ho++) {
            hc[ho+1] = '\0';
          hc[ho] = value[o];
          o++;
        }
        uint32_t c = hexColor(hc);
        strip.setPixelColor(grid[id], c);
        id++;
      }
    
    }
  }
  
    
  strip.show();
  server.printP(ok);
}

void cmd_gridTest(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) {
	if (!auth(server)) { return;}

	int d;

	URLPARAM_RESULT rc;
	char name[NAMELEN];
	char value[VALUELEN];
	while (strlen(url_tail)) {
		rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
		if ((rc != URLPARAM_EOS)) {
			if (name[0] == 'd') {
				d = atoi(value);
			}
		}
	}

	gridTest(d);
	server.printP(ok);
}

void cmd_lightTest(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete) {
	if (!auth(server)) { return;}

	int d;

	URLPARAM_RESULT rc;
	char name[NAMELEN];
	char value[VALUELEN];
	while (strlen(url_tail)) {
		rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
		if ((rc != URLPARAM_EOS)) {
			if (name[0] == 'd') {
				d = atoi(value);
			}
		}
	}

	lightTest(d);
	server.printP(ok);
}


// begin standard arduino setup and loop pattern

void setup() {
	Serial.begin(9600);
	
	//TODO: I think I've run out of memory, consolidate "tests"
	//Ethernet.begin(mac, ip);

  static uint8_t dns[] = { 8, 8, 8, 8 };
  static uint8_t gw[] = { 10, 10, 16, 254 };
  Ethernet.begin(mac, ip, dns, gw);
  
	webserver.setFailureCommand(&my_failCmd);
	webserver.setDefaultCommand(&cmd_index);
	webserver.addCommand("off", &cmd_off);
	webserver.addCommand("show", &cmd_show);
	webserver.addCommand("wipe", &cmd_wipe);
	webserver.addCommand("color", &cmd_color);
	webserver.addCommand("alert", &cmd_alert);
	webserver.addCommand("pixel", &cmd_pixel);
	webserver.addCommand("default", &cmd_default);
	webserver.addCommand("frame", &cmd_frame);
	webserver.addCommand("gridtest", &cmd_gridTest);
	webserver.addCommand("lighttest", &cmd_lightTest); //when adding extra commands, increase WEBDUINO_COMMANDS_COUNT
	webserver.begin();
	
	strip.begin();
	
	// light blip of light to signal we are ready to listen
	colorAll(Color(0,0,11));
	colorAll(Color(0,0,0));

  lightTest(0);

  Serial.println("Ready..."); 
}

void loop()
{
	// listen for connections
	char buff[64];
	int len = 64;
	webserver.processConnection(buff, &len);

	// run the default pattern
	switch(defaultPattern) {
		case 1:
			p_rainbow();
			break;
		case 2:
			p_random(500);
			break;
		case 3:
			p_cylon();
			break;
	}
}
