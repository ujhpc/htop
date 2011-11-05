/* Do not edit this file. It was automatically generated. */

#ifndef HEADER_htop
#define HEADER_htop
/*
htop - htop.h
(C) 2004-2011 Hisham H. Muhammad
Released under the GNU GPL, see the COPYING file
in the source distribution for its full text.
*/

#define _GNU_SOURCE
#include <unistd.h>
#include <math.h>
#include <sys/param.h>
#include <ctype.h>
#include <stdbool.h>
#include <locale.h>
#include <getopt.h>

#include "ProcessList.h"
#include "CRT.h"
#include "Panel.h"
#include "UsersTable.h"
#include "RichString.h"
#include "Settings.h"
#include "ScreenManager.h"
#include "FunctionBar.h"
#include "ListItem.h"
#include "CategoriesPanel.h"
#include "SignalsPanel.h"
#include "TraceScreen.h"
#include "OpenFilesScreen.h"
#include "AffinityPanel.h"

#include "config.h"
#include "debug.h"

//#link m

#define INCSEARCH_MAX 40

#define COPYRIGHT "(C) 2004-2011 Hisham Muhammad"

typedef struct IncBuffer_;

int main(int argc, char** argv);

#endif
