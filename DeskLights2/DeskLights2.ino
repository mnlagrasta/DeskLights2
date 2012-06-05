#include <Ethernet.h>
#include <SPI.h>
#include <Adafruit_WS2801.h>

// LED Stuff
int dataPin  = 2;    // Yellow wire on Adafruit Pixels
int clockPin = 3;    // Green wire on Adafruit Pixels
// update the next line to match your number of lights and chip type
Adafruit_WS2801 strip = Adafruit_WS2801(40, dataPin, clockPin, WS2801_GRB);
int defaultPattern = 0;

/************ ETHERNET STUFF ************/
byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x27, 0x05 }; // update this to match your arduino/shield
byte ip[] = { 10, 10, 16, 211 }; // update this to match your network
EthernetServer server(80);
#define BUFSIZ 100

// store error strings in flash to save RAM
#define error(s) error_P(PSTR(s))

/********* Helper Functions are here, real meat is later *************/

char key[8];
int keyset = 0;

/*
 I'm trying to add the option to require a key to auth requests
 The first call with key=xxx will set the key
 all subsequent calls would need to provide the key
 if the key is never set, all requests will be honored
 to change the key, restart the arduino and send a new key
 
 Unfortunately, this will require more sophisticated url parsing,
 which I don't have time to work on right now...any takers?
*/
void setKey(char * in) {
 if (!keyset) {
  Serial.println("key is not set");
  strcpy(key, in);
  keyset++;
  Serial.print("key is now set: ");
  Serial.println(key);
 }
}

// This function was meant to help with the url parsing
// it can seperate the parts, but not the parameters themselves
void splitURL(char *url, char *ohost, char *ofile, char *oparams) {
 char *host;
 char *file;
 char *params;
 
 char cpy[100]; //TODO: use len of url?
 strcpy(cpy, url);
 host = strstr(cpy, "//") + 2;
 file = strstr(host, "/");
 params = strstr(cpy, "?");
 file[0] = '\0';
 file++;
 params[0] = '\0';
 params++;

 strcpy(ohost, host);
 strcpy(ofile, file);
 strcpy(oparams, params);
}

// ascii hex to rgb
// in is a RGB hex color string "ff00ff"
// out is a 3 cell int array in which to store the result
// this is NOT a fast function, use it sparingly and not in tight loops
void ah2rgb(char * in, int * out) {
 char hr[3];
 char hg[3];
 char hb[3];
 hr[0] = in[0];
 hr[1] = in[1];
 hr[2] = 0;
 hg[0] = in[2];
 hg[1] = in[3];
 hg[2] = 0;
 hb[0] = in[4];
 hb[1] = in[5];
 hb[2] = 0;
 
 out[0] = (int) strtol(hr, NULL, 16);
 out[1] = (int) strtol(hg, NULL, 16);
 out[2] = (int) strtol(hb, NULL, 16);
}

// ascii hex to "Color"
uint32_t ah2c(char * in) {
 int rgb[3];
 ah2rgb(in, rgb);
 return Color(rgb[0], rgb[1], rgb[2]);
}

// this takes x/y coordinates and maps it to a pixel offset
// just for ease of making patterns
// your grid will need to be updated to match your pixel count and layout
int g2p(int x, int y) {
 int grid[4][10] = {
  {39,32,31,24,23,16,15, 8,7,0},
  {38,33,30,25,22,17,14, 9,6,1},
  {37,34,29,26,21,18,13,10,5,2},
  {36,35,28,27,20,19,12,11,4,3}
 };
 return grid[y][x];
}

void pixelHex(char *param) {
 char *x;
 char *y;
 char *hexColor;
 x = strtok(param, ",");
 y = strtok(NULL, ",");
 hexColor = strtok(NULL, ",");
 strip.setPixelColor(g2p(atoi(x), atoi(y)), ah2c(hexColor));
 strip.show();
};

void pixelRGB(char *param) {
 char *x;
 char *y;
 char *r;
 char *g;
 char *b;
 x = strtok(param, ",");
 y = strtok(NULL, ",");
 r = strtok(NULL, ",");
 g = strtok(NULL, ",");
 b = strtok(NULL, ",");
 strip.setPixelColor(g2p(atoi(x), atoi(y)), Color(atoi(r),atoi(g),atoi(b)));
 strip.show();
};

void colorAllRGB(char *param) {
 char *r;
 char *g;
 char *b;
 r = strtok(param, ",");
 g = strtok(NULL, ",");
 b = strtok(NULL, ",");
 colorAll(Color(atoi(r), atoi(g), atoi(b)));
}

// fade from off to "target" rgb values
void off2rgb(int tr, int tg, int tb) {
 colorAll(Color(0,0,0));
 int r = 0;
 int g = 0;
 int b = 0;
  
 while (r < tr | g < tg | b < tb) {
  if (r < tr) { r++; }
  if (g < tg) { g++; }
  if (b < tb) { b++; }
  colorAll(Color(r,g,b));
 }
}

// fade from rgb color to off
void rgb2off(int r, int g, int b) {
 while ( r > 0 | g > 0 | b > 0) {
  if (r > 0) { r--; }
  if (g > 0) { g--; }
  if (b > 0) { b--; }
  colorAll(Color(r,g,b));
 }
}

// set all pixels to a "Color" value
void colorAll(uint32_t c) {
 int i;
 for (i=0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
 }
 strip.show();
}

// set all pixels to a "Color" value, one at a time, with a delay
void colorWipe(uint32_t c, uint8_t wait) {
  int i;
  for (i=0; i < strip.numPixels(); i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(wait);
  }
}

// create the "Color" value from rgb...This is right from Adafruit
uint32_t Color(byte r, byte g, byte b) {
// Create a 24 bit color value from R,G,B
  uint32_t c;
  c = r;
  c <<= 8;
  c |= g;
  c <<= 8;
  c |= b;
  return c;
}

//Input a value 0 to 255 to get a color value.
//The colours are a transition r - g -b - back to r
uint32_t Wheel(byte WheelPos)
{
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

// go from a "Color" to rgb, good for acting on already active pixels
// P.S. not well tested yet
/*
void c2rgb(uint32_t c, int *p_rgb) {
 int r = c>>16;
 int g = c>>8 & 255;
 int b = c & 255;
 p_rgb[0] = r;
 p_rgb[1] = g;
 p_rgb[2] = b;
}

// the idea was to be able to read a pixel color and transition to another color
// ran out of time and left this for later.
void fade2rgb(int pixel, int r, int g, int b) {
 uint32_t start = strip.getPixelColor(pixel);
 int o[3];
 c2rgb(start, o);

 int rmod = 1;
 int gmod = 1;
 int bmod = 1;
 if (r < o[0]) { rmod = -1;}
 if (g < o[1]) { gmod = -1;}
 if (b < o[2]) { bmod = -1;}
 
 while ( r != o[0] || g != o[1] || b != o[2] ) {
  o[0] += rmod;
  if (r == o[0]) { rmod = 0;}
  o[1] += gmod;
  if (g == o[1]) { gmod = 0;}
  o[2] += bmod;
  if (b == o[2]) { bmod = 0;}
  strip.setPixelColor(pixel, Color(o[0], o[1], o[2]));
  strip.show();
 }
}
 */


/************** here come the patterns, these are intended to run between notifications ****************/

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

// sweeping red lights with a trail
// unfortunately, it's wrapping one pixel at each end
// no time to figure it out now...I've got deadlines and a new baby
void p_cylon() { // or knight rider if that's more your thing.
 uint32_t colors[4] = {Color(255,0,0), Color(150,0,0), Color(50,0,0), Color(0,0,0)};
 int wait = 100;

 int i;
 for (i=0; i<13; i++) {
  int j;
  for (j=0; j<4; j++) {
   int x = i-j;
   if (x < 40) {
    strip.setPixelColor(g2p(x,0), colors[j]);
    strip.setPixelColor(g2p(x,1), colors[j]);
    strip.setPixelColor(g2p(x,2), colors[j]);
    strip.setPixelColor(g2p(x,3), colors[j]);
   }
  }
  strip.show();
  delay(wait);
 }

 for (i=10; i>-3; i--) {
  int j;
  for (j=0; j<4; j++) {
   int x = i+j;
   if (x < 40) { // max x value, ought to be a var
    strip.setPixelColor(g2p(i+j,0), colors[j]);
    strip.setPixelColor(g2p(i+j,1), colors[j]);
    strip.setPixelColor(g2p(i+j,2), colors[j]);
    strip.setPixelColor(g2p(i+j,3), colors[j]);
   }
  }
  strip.show();
  delay(wait);
 }
 
}

// generic alert, fade in to specified color and out
void alert(char * hexcolor) {
 int rgb[3];
 ah2rgb(hexcolor, rgb);
 off2rgb(rgb[0],rgb[1],rgb[2]);
 rgb2off(rgb[0],rgb[1],rgb[2]);
}

// oooh, helper functions...um...helping
void tweet() {
 alert("00ff00");
}

// I should answer the phone, so this one is obnoxious
void skype() {
 int i;
 for (i=0;i<4;i++) {
  colorAll(Color(255,0,255));
  delay(200);
  colorAll(Color(0,0,0));
  delay(50);
 }
}

// could just use alert('ff00ff'), but it's got a slow in, fast out pace that I like.
void growl() {
 int i, j;
 
 //fade in
 for (i=0; i<255; i++) { 
  for (j=0; j<strip.numPixels(); j++) {
   strip.setPixelColor(j, Color(i,0,0));
  }
  strip.show();
 }

 // now fade out a little faster
 for (i=255; i >= 0; i=i-5) { 
  for (j=0; j<strip.numPixels(); j++) {
   strip.setPixelColor(j, Color(i,0,0));
  }
  strip.show();
 }
 
}

// deja vu, growl in blue...deja blue? deja growl?
void email() {
 int i, j;
 
 //fade in
 for (i=0; i<255; i++) { 
  for (j=0; j<strip.numPixels(); j++) {
   strip.setPixelColor(j, Color(0,0,i));
  }
  strip.show();
 }

 // noow fade out a little faster
 for (i=255; i >= 0; i=i-5) { 
  for (j=0; j<strip.numPixels(); j++) {
   strip.setPixelColor(j, Color(0,0,i));
  }
  strip.show();
 }
 
}

// horizontal color stripes...just cause
void colorLines() {
 int i;
 for (i=0; i<10; i++) {
  strip.setPixelColor(g2p(i,0), Color(255,0,0));
 }
 strip.show();
 delay(500);
 for (i=0; i<10; i++) {
  strip.setPixelColor(g2p(i,1), Color(0,255,0));
 }
 strip.show();
 delay(500);
 for (i=0; i<10; i++) {
  strip.setPixelColor(g2p(i,2), Color(0,0,255));
 }
 strip.show();
 delay(500);
 for (i=0; i<10; i++) {
  strip.setPixelColor(g2p(i,3), Color(255,255,255));
 }
 strip.show();
 delay(500);
 colorWipe(Color(0, 0, 0), 50);
}

// sweep all color extremes through all pixels
// nice test to find bad pixels, and it looks cool
void lightTest() {
 colorWipe(Color(255, 0, 0), 50);
 colorWipe(Color(0, 255, 0), 50);
 colorWipe(Color(0, 0, 255), 50);
 colorWipe(Color(255, 255, 255), 50);
 colorWipe(Color(0, 0, 0), 50);
}

// the short version, when all you need is a response to close the http connection
char * clientResponse() {
 char * buffer = "Content-Type: text/plain\n\nok\n";
 return buffer;
}

/*
// ugh, how to output something useful to the browser
// not really needed in a server script, but good for demos
// you can reclaim some memory by commenting this out and going with the earlier one.
// as of right now this doesn't even work. crashes and reboots the arduino
// probably over running the buffer or something...use the short one above for now.
char * clientResponse() {
 char * buffer = "Content-Type: text/html\n\n \
  <html> <head><meta name = \"viewport\" content = \"width = device-width\"> <title>DeskLights Version 2</title></head><body> \
  <h1>DeskLights Version 2</h1><hr> \
  <h2>Send A Notification</h2> \
  <a href=\"/growl\">Growl</a><br> \
  <a href=\"/email\">Email</a><br> \
  <a href=\"/tweet\">Twitter</a><br> \
  <a href=\"/skype\">Skype</a><br> \
  <a href=\"/lines\">Lines</a><br><hr> \
  <h2>Send A Color Alert</h2> \
  <form> Color: <input id=\"alertColor\" type=\"text\"> <input type=\"button\" value=\"Submit\" \
  onclick=\"window.location.href = '/alert?' + document.getElementById('alertColor').value;\"></form><hr> \
  <h2>Set A Static Color</h2> <form> Set Hex Color: <input id=\"staticColor\" type=\"text\"> <input type=\"button\" value=\"Submit\" \
  onclick=\"window.location.href = '/color?' + document.getElementById('staticColor').value;\" > </form> <hr> \
  <h2>Set The Default Pattern</h2> <form> Change Default Pattern: <select id=\"defaultID\" \
  onchange=\"window.location.href = '/default?' + document.getElementById('defaultID').value;\" > \
  <option value=\"0\">None</option> \
  <option value=\"1\">Rainbow</option> \
  <option value=\"2\">Cylon</option> \
  <option value=\"3\">Slow Random</option> \
  <option value=\"4\">Fast Random</option> \
  </select> </form> </body> </html>";
 return buffer;
}
*/

/*
 * Begin main code here
 */
 
// the content of this comes mainly from the ethernet shield demo
//TODO: refactor this (pull out client...or command parsing?)
bool listen() {
  char clientline[BUFSIZ];
  char command[20];
  int index = 0;
  
  EthernetClient client = server.available();
  if (client) {
    // an http request ends with a blank line
    boolean current_line_is_blank = true;
    
    // reset the input buffer
    index = 0;
    
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        
        // If it isn't a new line, add the character to the buffer
        if (c != '\n' && c != '\r') {
          clientline[index] = c;
          index++;
          // are we too big for the buffer? start tossing out data
          if (index >= BUFSIZ) 
            index = BUFSIZ -1;
          
          // continue to read more data!
          continue;
        }
        
        // got a \n or \r new line, which means the string is done
        clientline[index] = 0;
        
        // Print it out for debugging
        //Serial.println(clientline);
        
        // Look for substring such as a request to get the root file
        if (strstr(clientline, "GET / ") != 0) {
          strcpy(command, "/");
        } else if (strstr(clientline, "GET /") != 0) {
          // this time no space after the /, so a sub-file!
          char *filename;
          filename = clientline + 5; // look after the "GET /" (5 chars)
          (strstr(clientline, " HTTP"))[0] = 0;
          strcpy(command, filename);
        }
        break;
      }
    }
    
    // split out params
    char * param = strstr(command, "?");
    if (param) {
     param[0] = 0;
     param++;
    }
    
    // thought about doing some function ref stuff here
    // then a said pfffffffttttt and did this.
    /*********
    if you want a new alert or pattern, this is where you tell the server what to listen for
    **********/
    if (!strcmp(command, "/")) {
     // no command
    } else if (!strcmp(command, "key")) {
     Serial.println("got key request:");
     Serial.println(param);
     setKey(param);
    } else if (!strcmp(command, "color")) {
     if (param) {  colorAll(ah2c(param)); }
    } else if (!strcmp(command, "colorrgb")) {
     if (param) {  colorAllRGB(param); }
    } else if (!strcmp(command, "cylon")) {
     p_cylon();
    } else if (!strcmp(command, "growl")) {
     growl();
    } else if (!strcmp(command, "email")) {
     email();
    } else if (!strcmp(command, "tweet")) {
     tweet();
    } else if (!strcmp(command, "skype")) {
     skype();
    } else if (!strcmp(command, "test")) {
     lightTest();
    } else if (!strcmp(command, "lines")) {
     colorLines();
    } else if (!strcmp(command, "alert")) {
     alert(param);
    } else if (!strcmp(command, "pixel")) {
     pixelHex(param);
    } else if (!strcmp(command, "pixelrgb")) {
     pixelRGB(param);
    } else if (!strcmp(command, "default")) {
     colorAll(Color(0,0,0));
     defaultPattern = atoi(param);
    } else {
     //Serial.print("unknown command");
    }
     client.print(clientResponse());    
    // give the web browser time to receive the data
    delay(1);
    client.stop();
    return 1;
  } else {
   return 0;
  }
}

// typical arduino setup
// prime the serial comm to the ethernet "shield" and the lights
void setup() {
 Serial.begin(9600);
 Ethernet.begin(mac, ip);
 server.begin();
 strip.begin();
 //lightTest();
}

// burn baby, burn
void loop() {
 if (!listen()) {
  switch(defaultPattern) {
   case (1):
    p_rainbow();
    break;
   case (2):
    p_cylon();
    break;
   case (3):
    p_random(0);
   case (4):
    p_random(500);
    break;
  }
 }
}
