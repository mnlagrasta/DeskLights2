# DeskLights2.2

## Overview

DeskLights is a network enabled visual alert system and an experiment in "Ambient Information". It receives instructions through a REST style API and controls the attached LED strips.

For background and hardware details, please see the more complete write up on Instructables: http://www.instructables.com/id/LED-Glass-Desk-v20/

## What's New (July 2016)

I've updated the repo to reflect some changes I've made and things I've learned in several years fo regular use.
 * Included the command line utility "dl" that I use almost exclusively instead sending http requests to the desk.
 * Included a sample Perl script that parses Nagios' dat file and send displays status on the desk.
 * Added the "frame" command, which allows you to set all pixels of the desk in one http post request
 * Archived all the old Growl and AppleScript stuff that I really don't use anymore.
 * You can now use pass an image to the dl script and it will display it on the desk (after severely resizing, of course).

## Previous Update (November 2013)

 * New "pixel" command, allowing for the setting of individual leds, including the ability to set a series of pixel colors without displaying them until after they are all set.
 * All web server related code is now handled by the [Webduino](https://github.com/sirleech/Webduino) library. This brings with it several nice features:
   * Proper URI params
   * Basic Authentication
   * More readable code file
 * Most functions have been refactored to accept standard parameters and be more flexible
 * Unfortunately, this caused all of the API urls to change and breaks backwards compatibility with any calling scripts you may have written. Sorry, but I think it's worth it.

## API

### Parameter Explanation
All exposed function calls take 0 or more single letter parameters:
```
r: red value (0 - 255)
g: green value (0 - 255)
b: blue value (0 - 255)
h: hex string representing color (000000 - ffffff)
x: x coordinate
y: y coordinate
d: delay in milliseconds
i: id of pattern/grid id of pixel (grid 0,0 = 0; grid 1,0 = 1, etc)
n: number of pixel on strip, starts at 0
s: call show() command automatically (0 or 1, defaults to 1)
```
Note: All color parameters can be specified as either RGB or hex

### Public Functions
```
off : Turn off all pixels
show : show any set, but not yet shown pixels
color: set all pixels to color; takes color
wipe: like color, but down the strip; optional delay
alert: flash all pixels; takes color and optional delay
pixel: set pixel to color; takes id or number, color, and optional show
default: set default pattern to loop, takes pattern id
frame: set the whole desk at once, pass in string of hex colors ie: h=ffffff000000 etc...
```

These are not currently available. Seems I ran out of room in memory somewhere?
```
gridtest: show grid pixel by pixel, no params
lighttest: show rgb on all pixels, no params
```
### Example

Here's an example using alert...

Flash bright white for default length of time 
```
http://server/alert?r=255&g=255&b=255
```
or
```
http://server/alert?h=ffffff
```
or with a 1 second duration
```
http://server/alert?h=ffffff&d=1000
```

## Dependancies
 * You'll need an Arduino and ethernet shield. I currently use the Arduino Uno Ethernet.
 * Arduino dev tools http://arduino.cc
 * Adafruit WS2801 arduino library: https://github.com/adafruit/Adafruit-WS2801-Library

## Helper Script Install

The dl utility is written in Perl. It has fairly minimal dependencies for the basic functions, but will require the GD library and Image::Resize module if you want to use the image to desk functionality.