/* Do not edit this file. It was automatically generated. */

#ifndef HEADER_LoadAverageMeter
#define HEADER_LoadAverageMeter
/*
htop
(C) 2004-2006 Hisham H. Muhammad
Released under the GNU GPL, see the COPYING file
in the source distribution for its full text.
*/

#include "Meter.h"

#include <curses.h>

#include "debug.h"






void LoadAverageMeter_setValues(Meter* this, char* buffer, int size);

void LoadAverageMeter_display(Object* cast, RichString* out);

void LoadMeter_setValues(Meter* this, char* buffer, int size);

void LoadMeter_display(Object* cast, RichString* out);

#endif
