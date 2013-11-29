# DeskLights2.1

## Overview

DeskLights is a network enabled visual alert system. It receives instructions through a REST style API and controls the attached LED strips.

For background and hardware details, please see the more complete write up on Instructables: http://www.instructables.com/id/LED-Glass-Desk-v20/

## What's New

In honor of this years Maker Faire, I've finally made some big updates to address various shortcomings. I've also made some changes based on how my daily use of the system has evolved over the last two years. Originally, it was focused on incoming events (such as a message or call). That is still useful, but it now includes much more of what might be called "ambient information". The assignment of pixels or regions to represent various pieces of data. For example, a pixel representing a server may go red when under load. Or the intensity of a region might let me know I have email waiting. Or a software build needs attention.

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

## What's Next (and other notes)
I have a series of helper scripts that monitor various things and perform the API calls to the desk. I'm hoping to clean these up and share them.
They are in several different languages (Perl, NodeJS, AppleScript, etc...) and I should probably unify them before posting. 

Also, I've completely stopped using Growl. So the Growl style and helper scripts may not be updated in the future.
