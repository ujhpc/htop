
#include "DisplayOptionsPanel.h"

#include "Panel.h"
#include "CheckItem.h"
#include "Settings.h"
#include "ScreenManager.h"

#include "debug.h"
#include <assert.h>

/*{

typedef struct DisplayOptionsPanel_ {
   Panel super;

   Settings* settings;
   ScreenManager* scr;
} DisplayOptionsPanel;

}*/

DisplayOptionsPanel* DisplayOptionsPanel_new(Settings* settings, ScreenManager* scr) {
   DisplayOptionsPanel* this = (DisplayOptionsPanel*) malloc(sizeof(DisplayOptionsPanel));
   Panel* super = (Panel*) this;
   Panel_init(super, 1, 1, 1, 1, CHECKITEM_CLASS, true);
   ((Object*)this)->delete = DisplayOptionsPanel_delete;

   this->settings = settings;
   this->scr = scr;
   super->eventHandler = DisplayOptionsPanel_EventHandler;

   Panel_setHeader(super, "Display options");
   Panel_add(super, (Object*) CheckItem_new(String_copy("Tree view"), &(settings->pl->treeView)));
   Panel_add(super, (Object*) CheckItem_new(String_copy("Shadow other users' processes"), &(settings->pl->shadowOtherUsers)));
   Panel_add(super, (Object*) CheckItem_new(String_copy("Hide kernel threads"), &(settings->pl->hideKernelThreads)));
   Panel_add(super, (Object*) CheckItem_new(String_copy("Hide userland threads"), &(settings->pl->hideUserlandThreads)));
   Panel_add(super, (Object*) CheckItem_new(String_copy("Highlight program \"basename\""), &(settings->pl->highlightBaseName)));
   Panel_add(super, (Object*) CheckItem_new(String_copy("Highlight megabytes in memory counters"), &(settings->pl->highlightMegabytes)));
   Panel_add(super, (Object*) CheckItem_new(String_copy("Leave a margin around header"), &(settings->header->margin)));
   Panel_add(super, (Object*) CheckItem_new(String_copy("Split System Time into System/IO-Wait/Hard-IRQ/Soft-IRQ"), &(settings->pl->expandSystemTime)));
   return this;
}

void DisplayOptionsPanel_delete(Object* object) {
   Panel* super = (Panel*) object;
   DisplayOptionsPanel* this = (DisplayOptionsPanel*) object;
   Panel_done(super);
   free(this);
}

HandlerResult DisplayOptionsPanel_EventHandler(Panel* super, int ch) {
   DisplayOptionsPanel* this = (DisplayOptionsPanel*) super;
   
   HandlerResult result = IGNORED;
   CheckItem* selected = (CheckItem*) Panel_getSelected(super);

   switch(ch) {
   case 0x0a:
   case 0x0d:
   case KEY_ENTER:
   case ' ':
      *(selected->value) = ! *(selected->value);
      result = HANDLED;
   }

   if (result == HANDLED) {
      this->settings->changed = true;
      Header* header = this->settings->header;
      Header_calculateHeight(header);
      Header_draw(header);
      ScreenManager_resize(this->scr, this->scr->x1, header->height, this->scr->x2, this->scr->y2);
   }
   return result;
}

