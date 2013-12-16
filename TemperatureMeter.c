/*
htop - TemperatureMeter.c
(C) 2013 Adam Strzelecki
Released under the GNU GPL, see the COPYING file
in the source distribution for its full text.
*/

#include "TemperatureMeter.h"

#include "ProcessList.h"
#include "CRT.h"

#include <math.h>

/*{
#include "Meter.h"
}*/

/* replace to something else matching your system */
/* however usually Intel CPUs are detected as following: */
#define TEMP_PREFIX_FMT "/sys/class/hwmon/hwmon%d/device/temp%d_"

/* suffixes for path prefix above */
#define TEMP_INPUT "input"
#define TEMP_MAX   "max"
#define TEMP_CRIT  "crit"

#define TEMP_DEGREE "\u00B0C"

int TemperatureMeter_attributes[] = {
   TEMP_COOL, TEMP_WARM, TEMP_HOT
};

static int TemperatureMeter_read(const char* fmt, double* out, char* fn) {
  FILE* fd = NULL;
  if (*fn) {
    fd = fopen(fn, "r");
  } else {
    for(int device = 0; device < 4 && !fd; ++device) {
      for(int sensor = 1; sensor < 4 && !fd; ++sensor) {
        snprintf(fn, 64, fmt, device, sensor);
        fd = fopen(fn, "r");
      }
    }
  }
  if (fd) {
    fscanf(fd, "%64lf", out);
    *out /= 1000.0;
    fclose(fd);
    return 0;
  }
  return -1;
}

static void TemperatureMeter_setValues(Meter* this, char* buffer, int len) {
   double input = 0, crit = 100;
   static char fn_input[64] = "\0";
   static char fn_crit[64] = "\0";
   TemperatureMeter_read(TEMP_PREFIX_FMT TEMP_INPUT, &input, fn_input);
   if(TemperatureMeter_read(TEMP_PREFIX_FMT TEMP_MAX,  &crit, fn_crit)) {
      TemperatureMeter_read(TEMP_PREFIX_FMT TEMP_CRIT, &crit, fn_crit);
   }
   this->values[0] = input;
   this->values[1] = crit;
   snprintf(buffer, len, "%.1f" TEMP_DEGREE, input);
}

static void TemperatureMeter_display(Object* cast, RichString* out) {
   Meter* this = (Meter*)cast;
   char buffer[20];
   double input = this->values[0];
   double crit = this->values[1];
   double warm = crit / 2.0;
   double hot = crit * 2.0 / 3.0;
   sprintf(buffer, "%.1f" TEMP_DEGREE, input);
   int temp = TEMP_WARM;
   if (input < warm) {
      temp = TEMP_COOL;
   } else if (input > hot) {
      temp = TEMP_HOT;
   }
   RichString_write(out, CRT_colors[temp], buffer);
}

MeterClass TemperatureMeter_class = {
   .super = {
      .extends = Class(Meter),
      .delete = Meter_delete,
      .display = TemperatureMeter_display
   },
   .setValues = TemperatureMeter_setValues,
   .defaultMode = TEXT_METERMODE,
   .items = 2,
   .total = 100.0,
   .attributes = TemperatureMeter_attributes,
   .name = "Temperature",
   .uiName = "Temperature",
   .caption = "Temperature: "
};
