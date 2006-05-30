/* Do not edit this file. It was automatically generated. */

#ifndef HEADER_Header
#define HEADER_Header
/*
htop - Header.c
(C) 2004-2006 Hisham H. Muhammad
Released under the GNU GPL, see the COPYING file
in the source distribution for its full text.
*/

#include "Meter.h"

#include "debug.h"
#include <assert.h>


typedef enum HeaderSide_ {
   LEFT_HEADER,
   RIGHT_HEADER
} HeaderSide;

typedef struct Header_ {
   Vector* leftMeters;
   Vector* rightMeters;
   ProcessList* pl;
   bool margin;
   int height;
   int pad;
} Header;


#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

Header* Header_new(ProcessList* pl);

void Header_delete(Header* this);

void Header_createMeter(Header* this, char* name, HeaderSide side);

void Header_setMode(Header* this, int i, MeterModeId mode, HeaderSide side);

Meter* Header_addMeter(Header* this, MeterType* type, int param, HeaderSide side);

int Header_size(Header* this, HeaderSide side);

char* Header_readMeterName(Header* this, int i, HeaderSide side);

MeterModeId Header_readMeterMode(Header* this, int i, HeaderSide side);

void Header_defaultMeters(Header* this);

void Header_draw(Header* this);

int Header_calculateHeight(Header* this);

#endif
