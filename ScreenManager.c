/*
htop
(C) 2004-2006 Hisham H. Muhammad
Released under the GNU GPL, see the COPYING file
in the source distribution for its full text.
*/

#include "ScreenManager.h"
#include "Panel.h"
#include "Object.h"
#include "Vector.h"
#include "FunctionBar.h"

#include "debug.h"
#include <assert.h>

#include <stdbool.h>

/*{

typedef enum Orientation_ {
   VERTICAL,
   HORIZONTAL
} Orientation;

typedef struct ScreenManager_ {
   int x1;
   int y1;
   int x2;
   int y2;
   Orientation orientation;
   Vector* items;
   Vector* fuBars;
   int itemCount;
   FunctionBar* fuBar;
   bool owner;
} ScreenManager;

}*/

ScreenManager* ScreenManager_new(int x1, int y1, int x2, int y2, Orientation orientation, bool owner) {
   ScreenManager* this;
   this = malloc(sizeof(ScreenManager));
   this->x1 = x1;
   this->y1 = y1;
   this->x2 = x2;
   this->y2 = y2;
   this->fuBar = NULL;
   this->orientation = orientation;
   this->items = Vector_new(PANEL_CLASS, owner, DEFAULT_SIZE);
   this->fuBars = Vector_new(FUNCTIONBAR_CLASS, true, DEFAULT_SIZE);
   this->itemCount = 0;
   this->owner = owner;
   return this;
}

void ScreenManager_delete(ScreenManager* this) {
   Vector_delete(this->items);
   Vector_delete(this->fuBars);
   free(this);
}

inline int ScreenManager_size(ScreenManager* this) {
   return this->itemCount;
}

void ScreenManager_add(ScreenManager* this, Panel* item, FunctionBar* fuBar, int size) {
   if (this->orientation == HORIZONTAL) {
      int lastX = 0;
      if (this->itemCount > 0) {
         Panel* last = (Panel*) Vector_get(this->items, this->itemCount - 1);
         lastX = last->x + last->w + 1;
      }
      if (size > 0) {
         Panel_resize(item, size, LINES-this->y1+this->y2);
      } else {
         Panel_resize(item, COLS-this->x1+this->x2-lastX, LINES-this->y1+this->y2);
      }
      Panel_move(item, lastX, this->y1);
   }
   // TODO: VERTICAL
   Vector_add(this->items, item);
   if (fuBar)
      Vector_add(this->fuBars, fuBar);
   else
      Vector_add(this->fuBars, FunctionBar_new(0, NULL, NULL, NULL));
   if (!this->fuBar && fuBar) this->fuBar = fuBar;
   item->needsRedraw = true;
   this->itemCount++;
}

Panel* ScreenManager_remove(ScreenManager* this, int index) {
   assert(this->itemCount > index);
   Panel* lb = (Panel*) Vector_remove(this->items, index);
   Vector_remove(this->fuBars, index);
   this->fuBar = NULL;
   this->itemCount--;
   return lb;
}

void ScreenManager_setFunctionBar(ScreenManager* this, FunctionBar* fuBar) {
   if (this->owner && this->fuBar)
      FunctionBar_delete((Object*)this->fuBar);
   this->fuBar = fuBar;
}

void ScreenManager_resize(ScreenManager* this, int x1, int y1, int x2, int y2) {
   this->x1 = x1;
   this->y1 = y1;
   this->x2 = x2;
   this->y2 = y2;
   int items = this->itemCount;
   int lastX = 0;
   for (int i = 0; i < items - 1; i++) {
      Panel* lb = (Panel*) Vector_get(this->items, i);
      Panel_resize(lb, lb->w, LINES-y1+y2);
      Panel_move(lb, lastX, y1);
      lastX = lb->x + lb->w + 1;
   }
   Panel* lb = (Panel*) Vector_get(this->items, items-1);
   Panel_resize(lb, COLS-x1+x2-lastX, LINES-y1+y2);
   Panel_move(lb, lastX, y1);
}

void ScreenManager_run(ScreenManager* this, Panel** lastFocus, int* lastKey) {
   bool quit = false;
   int focus = 0;
         
   Panel* lbFocus = (Panel*) Vector_get(this->items, focus);
   if (this->fuBar)
      FunctionBar_draw(this->fuBar, NULL);
   
   int ch;
   while (!quit) {
      int items = this->itemCount;
      for (int i = 0; i < items; i++) {
         Panel* lb = (Panel*) Vector_get(this->items, i);
         Panel_draw(lb, i == focus);
         if (i < items) {
            if (this->orientation == HORIZONTAL) {
               mvvline(lb->y, lb->x+lb->w, ' ', lb->h+1);
            }
         }
      }
      FunctionBar* bar = (FunctionBar*) Vector_get(this->fuBars, focus);
      if (bar)
         this->fuBar = bar;
      if (this->fuBar)
         FunctionBar_draw(this->fuBar, NULL);

      ch = getch();
      
      bool loop = false;
      if (ch == KEY_MOUSE) {
         MEVENT mevent;
         int ok = getmouse(&mevent);
         if (ok == OK) {
            if (mevent.y == LINES - 1) {
               ch = FunctionBar_synthesizeEvent(this->fuBar, mevent.x);
            } else {
               for (int i = 0; i < this->itemCount; i++) {
                  Panel* lb = (Panel*) Vector_get(this->items, i);
                  if (mevent.x > lb->x && mevent.x <= lb->x+lb->w &&
                     mevent.y > lb->y && mevent.y <= lb->y+lb->h) {
                     focus = i;
                     lbFocus = lb;
                     Panel_setSelected(lb, mevent.y - lb->y + lb->scrollV - 1);
                     loop = true;
                     break;
                  }
               }
            }
         }
      }
      if (loop) continue;
      
      if (lbFocus->eventHandler) {
         HandlerResult result = lbFocus->eventHandler(lbFocus, ch);
         if (result == HANDLED) {
            continue;
         } else if (result == BREAK_LOOP) {
            quit = true;
            continue;
         }
      }
      
      switch (ch) {
      case ERR:
         continue;
      case KEY_RESIZE:
      {
         ScreenManager_resize(this, this->x1, this->y1, this->x2, this->y2);
         continue;
      }
      case KEY_LEFT:
         tryLeft:
         if (focus > 0)
            focus--;
         lbFocus = (Panel*) Vector_get(this->items, focus);
         if (Panel_getSize(lbFocus) == 0 && focus > 0)
            goto tryLeft;
         break;
      case KEY_RIGHT:
      case 9:
         tryRight:
         if (focus < this->itemCount - 1)
            focus++;
         lbFocus = (Panel*) Vector_get(this->items, focus);
         if (Panel_getSize(lbFocus) == 0 && focus < this->itemCount - 1)
            goto tryRight;
         break;
      case KEY_F(10):
      case 'q':
      case 27:
         quit = true;
         continue;
      default:
         Panel_onKey(lbFocus, ch);
         break;
      }
   }

   *lastFocus = lbFocus;
   *lastKey = ch;
}
