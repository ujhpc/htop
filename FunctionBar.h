/* Do not edit this file. It was automatically generated. */

#ifndef HEADER_FunctionBar
#define HEADER_FunctionBar
/*
htop - FunctionBar.h
(C) 2004-2006 Hisham H. Muhammad
Released under the GNU GPL, see the COPYING file
in the source distribution for its full text.
*/

#include "Object.h"
#include "CRT.h"

#include "debug.h"
#include <assert.h>

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <curses.h>


typedef struct FunctionBar_ {
   Object super;
   int size;
   char** functions;
   char** keys;
   int* events;
   bool staticData;
} FunctionBar;


#ifdef DEBUG
extern char* FUNCTIONBAR_CLASS;
#else
#define FUNCTIONBAR_CLASS NULL
#endif

FunctionBar* FunctionBar_new(const char** functions, const char** keys, int* events);

void FunctionBar_delete(Object* cast);

void FunctionBar_setLabel(FunctionBar* this, int event, const char* text);

void FunctionBar_draw(FunctionBar* this, char* buffer);

void FunctionBar_drawAttr(FunctionBar* this, char* buffer, int attr);

int FunctionBar_synthesizeEvent(FunctionBar* this, int pos);

#endif
