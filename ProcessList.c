/*
htop - ProcessList.c
(C) 2004,2005 Hisham H. Muhammad
Released under the GNU GPL, see the COPYING file
in the source distribution for its full text.
*/

#ifndef CONFIG_H
#define CONFIG_H
#include "config.h"
#endif

#include "ProcessList.h"
#include "Process.h"
#include "Vector.h"
#include "UsersTable.h"
#include "Hashtable.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/utsname.h>
#include <stdarg.h>

#include "debug.h"
#include <assert.h>

/*{
#ifndef PROCDIR
#define PROCDIR "/proc"
#endif

#ifndef PROCSTATFILE
#define PROCSTATFILE "/proc/stat"
#endif

#ifndef PROCMEMINFOFILE
#define PROCMEMINFOFILE "/proc/meminfo"
#endif

#ifndef MAX_NAME
#define MAX_NAME 128
#endif

#ifndef MAX_READ
#define MAX_READ 8192
#endif

}*/

/*{

typedef struct ProcessList_ {
   Vector* processes;
   Vector* processes2;
   Hashtable* processTable;
   Process* prototype;
   UsersTable* usersTable;

   int processorCount;
   int totalTasks;
   int runningTasks;

   long int* totalTime;
   long int* userTime;
   long int* systemTime;
   long int* idleTime;
   long int* niceTime;
   long int* totalPeriod;
   long int* userPeriod;
   long int* systemPeriod;
   long int* idlePeriod;
   long int* nicePeriod;

   long int totalMem;
   long int usedMem;
   long int freeMem;
   long int sharedMem;
   long int buffersMem;
   long int cachedMem;
   long int totalSwap;
   long int usedSwap;
   long int freeSwap;

   ProcessField* fields;
   ProcessField sortKey;
   int direction;
   bool hideThreads;
   bool shadowOtherUsers;
   bool hideKernelThreads;
   bool hideUserlandThreads;
   bool treeView;
   bool highlightBaseName;
   bool highlightMegabytes;
   #ifdef DEBUG
   FILE* traceFile;
   #endif

} ProcessList;
}*/

/* private property */
ProcessField defaultHeaders[] = { PID, USER, PRIORITY, NICE, M_SIZE, M_RESIDENT, M_SHARE, STATE, PERCENT_CPU, PERCENT_MEM, TIME, COMM, 0 };

#ifdef DEBUG

/* private property */
typedef int(*vxscanf)(void*, const char*, va_list);

#define ProcessList_read(this, buffer, format, ...) ProcessList_xread(this, (vxscanf) vsscanf, buffer, format, ## __VA_ARGS__ )
#define ProcessList_fread(this, file, format, ...)  ProcessList_xread(this, (vxscanf) vfscanf, file, format, ## __VA_ARGS__ )

/* private */
FILE* ProcessList_fopen(ProcessList* this, const char* path, const char* mode) {
   fprintf(this->traceFile, "[%s]\n", path);
   return fopen(path, mode);
}

/* private */
static inline int ProcessList_xread(ProcessList* this, vxscanf fn, void* buffer, char* format, ...) {
   va_list ap;
   va_start(ap, format);
   int num = fn(buffer, format, ap);
   va_end(format);
   va_start(ap, format);
   while (*format) {
      char ch = *format;
      char* c; int* d; long int* ld; unsigned long int* lu; char** s;
      if (ch != '%') {
         fprintf(this->traceFile, "%c", ch);
         format++;
         continue;
      }
      format++;
      switch(*format) {
      case 'c': c = va_arg(ap, char*);  fprintf(this->traceFile, "%c", *c); break;
      case 'd': d = va_arg(ap, int*);   fprintf(this->traceFile, "%d", *d); break;
      case 's': s = va_arg(ap, char**); fprintf(this->traceFile, "%s", *s); break;
      case 'l':
         format++;
         switch (*format) {
         case 'd': ld = va_arg(ap, long int*); fprintf(this->traceFile, "%ld", *ld); break;
         case 'u': lu = va_arg(ap, unsigned long int*); fprintf(this->traceFile, "%lu", *lu); break;
         }
      }
      format++;
   }
   fprintf(this->traceFile, "\n");
   va_end(format);
   return num;
}

#else

#ifndef ProcessList_read
#define ProcessList_fopen(this, path, mode) fopen(path, mode)
#define ProcessList_read(this, buffer, format, ...) sscanf(buffer, format, ## __VA_ARGS__ )
#define ProcessList_fread(this, file, format, ...) fscanf(file, format, ## __VA_ARGS__ )
#endif

#endif

ProcessList* ProcessList_new(UsersTable* usersTable) {
   ProcessList* this;
   this = malloc(sizeof(ProcessList));
   this->processes = Vector_new(PROCESS_CLASS, true, DEFAULT_SIZE);
   this->processTable = Hashtable_new(20, false);
   this->prototype = Process_new(this);
   this->usersTable = usersTable;
   
   /* tree-view auxiliary buffers */
   this->processes2 = Vector_new(PROCESS_CLASS, true, DEFAULT_SIZE);
   
   #ifdef DEBUG
   this->traceFile = fopen("/tmp/htop-proc-trace", "w");
   #endif

   FILE* status = fopen(PROCSTATFILE, "r");
   assert(status != NULL);
   char buffer[256];
   int procs = -1;
   do {
      procs++;
      fgets(buffer, 255, status);
   } while (String_startsWith(buffer, "cpu"));
   fclose(status);
   this->processorCount = procs - 1;
   this->totalTime = calloc(procs, sizeof(long int));
   this->userTime = calloc(procs, sizeof(long int));
   this->systemTime = calloc(procs, sizeof(long int));
   this->niceTime = calloc(procs, sizeof(long int));
   this->idleTime = calloc(procs, sizeof(long int));
   this->totalPeriod = calloc(procs, sizeof(long int));
   this->userPeriod = calloc(procs, sizeof(long int));
   this->systemPeriod = calloc(procs, sizeof(long int));
   this->nicePeriod = calloc(procs, sizeof(long int));
   this->idlePeriod = calloc(procs, sizeof(long int));
   for (int i = 0; i < procs; i++) {
      this->totalTime[i] = 1;
      this->totalPeriod[i] = 1;
   }

   this->fields = calloc(sizeof(ProcessField), LAST_PROCESSFIELD+1);
   // TODO: turn 'fields' into a Vector,
   // (and ProcessFields into proper objects).
   for (int i = 0; defaultHeaders[i]; i++) {
      this->fields[i] = defaultHeaders[i];
   }
   this->sortKey = PERCENT_CPU;
   this->direction = 1;
   this->hideThreads = false;
   this->shadowOtherUsers = false;
   this->hideKernelThreads = false;
   this->hideUserlandThreads = false;
   this->treeView = false;
   this->highlightBaseName = false;
   this->highlightMegabytes = false;

   return this;
}

void ProcessList_delete(ProcessList* this) {
   Hashtable_delete(this->processTable);
   Vector_delete(this->processes);
   Vector_delete(this->processes2);
   Process_delete((Object*)this->prototype);

   free(this->totalTime);
   free(this->userTime);
   free(this->systemTime);
   free(this->niceTime);
   free(this->idleTime);
   free(this->totalPeriod);
   free(this->userPeriod);
   free(this->systemPeriod);
   free(this->nicePeriod);
   free(this->idlePeriod);

   #ifdef DEBUG
   fclose(this->traceFile);
   #endif

   free(this->fields);
   free(this);
}

void ProcessList_invertSortOrder(ProcessList* this) {
   if (this->direction == 1)
      this->direction = -1;
   else
      this->direction = 1;
}

RichString ProcessList_printHeader(ProcessList* this) {
   RichString out = RichString_new();
   ProcessField* fields = this->fields;
   for (int i = 0; fields[i]; i++) {
      char* field = Process_printField(fields[i]);
      if (this->sortKey == fields[i])
         RichString_append(&out, CRT_colors[PANEL_HIGHLIGHT_FOCUS], field);
      else
         RichString_append(&out, CRT_colors[PANEL_HEADER_FOCUS], field);
   }
   return out;
}


void ProcessList_prune(ProcessList* this) {
   Vector_prune(this->processes);
}

void ProcessList_add(ProcessList* this, Process* p) {
   Vector_add(this->processes, p);
   Hashtable_put(this->processTable, p->pid, p);
}

void ProcessList_remove(ProcessList* this, Process* p) {
   Hashtable_remove(this->processTable, p->pid);
   ProcessField pf = this->sortKey;
   this->sortKey = PID;
   int index = Vector_indexOf(this->processes, p);
   Vector_remove(this->processes, index);
   this->sortKey = pf;
}

Process* ProcessList_get(ProcessList* this, int index) {
   return (Process*) (Vector_get(this->processes, index));
}

int ProcessList_size(ProcessList* this) {
   return (Vector_size(this->processes));
}

/* private */
void ProcessList_buildTree(ProcessList* this, int pid, int level, int indent, int direction) {
   Vector* children = Vector_new(PROCESS_CLASS, false, DEFAULT_SIZE);

   for (int i = 0; i < Vector_size(this->processes); i++) {
      Process* process = (Process*) (Vector_get(this->processes, i));
      if (process->ppid == pid) {
         Process* process = (Process*) (Vector_take(this->processes, i));
         Vector_add(children, process);
         i--;
      }
   }
   int size = Vector_size(children);
   for (int i = 0; i < size; i++) {
      Process* process = (Process*) (Vector_get(children, i));
      if (direction == 1)
         Vector_add(this->processes2, process);
      else
         Vector_insert(this->processes2, 0, process);
      int nextIndent = indent;
      if (i < size - 1)
         nextIndent = indent | (1 << level);
      ProcessList_buildTree(this, process->pid, level+1, nextIndent, direction);
      process->indent = indent | (1 << level);
   }
   Vector_delete(children);
}

void ProcessList_sort(ProcessList* this) {
   if (!this->treeView) {
      Vector_sort(this->processes);
   } else {
      int direction = this->direction;
      int sortKey = this->sortKey;
      this->sortKey = PID;
      this->direction = 1;
      Vector_sort(this->processes);
      this->sortKey = sortKey;
      this->direction = direction;
      Process* init = (Process*) (Vector_take(this->processes, 0));
      assert(init->pid == 1);
      init->indent = 0;
      Vector_add(this->processes2, init);
      ProcessList_buildTree(this, init->pid, 0, 0, direction);
      Vector* t = this->processes;
      this->processes = this->processes2;
      this->processes2 = t;
   }
}

/* private */
int ProcessList_readStatFile(ProcessList* this, Process *proc, FILE *f, char *command) {
   static char buf[MAX_READ];
   long int zero;

   int size = fread(buf, 1, MAX_READ, f);
   if(!size) return 0;

   proc->pid = atoi(buf);
   char *location = strchr(buf, ' ');
   if(!location) return 0;

   location += 2;
   char *end = strrchr(location, ')');
   if(!end) return 0;
   
   int commsize = end - location;
   memcpy(command, location, commsize);
   command[commsize] = '\0';
   location = end + 2;
   
   int num = ProcessList_read(this, location, 
      "%c %d %d %d %d %d %lu %lu %lu %lu "
      "%lu %lu %lu %ld %ld %ld %ld %ld %ld "
      "%lu %lu %ld %lu %lu %lu %lu %lu "
      "%lu %lu %lu %lu %lu %lu %lu %lu "
      "%d %d",
      &proc->state, &proc->ppid, &proc->pgrp, &proc->session, &proc->tty_nr, 
      &proc->tpgid, &proc->flags, &proc->minflt, &proc->cminflt, &proc->majflt, 
      &proc->cmajflt, &proc->utime, &proc->stime, &proc->cutime, &proc->cstime, 
      &proc->priority, &proc->nice, &zero, &proc->itrealvalue, 
      &proc->starttime, &proc->vsize, &proc->rss, &proc->rlim, 
      &proc->startcode, &proc->endcode, &proc->startstack, &proc->kstkesp, 
      &proc->kstkeip, &proc->signal, &proc->blocked, &proc->sigignore, 
      &proc->sigcatch, &proc->wchan, &proc->nswap, &proc->cnswap, 
      &proc->exit_signal, &proc->processor);
   
   // This assert is always valid on 2.4, but reportedly not always valid on 2.6.
   // TODO: Check if the semantics of this field has changed.
   // assert(zero == 0);
   
   if(num != 37) return 0;
   return 1;
}

bool ProcessList_readStatusFile(ProcessList* this, Process* proc, char* dirname, char* name) {
   char statusfilename[MAX_NAME+1];
   statusfilename[MAX_NAME] = '\0';
   snprintf(statusfilename, MAX_NAME, "%s/%s/status", dirname, name);
   FILE* status = ProcessList_fopen(this, statusfilename, "r");
   bool success = false;
   if (status) {
      char buffer[1024];
      buffer[1023] = '\0';
      while (!feof(status)) {
         char* ok = fgets(buffer, 1023, status);
         if (!ok)
            break;
         if (String_startsWith(buffer, "Uid:")) {
            int uid1, uid2, uid3, uid4;
            // TODO: handle other uid's.
            int ok = ProcessList_read(this, buffer, "Uid:\t%d\t%d\t%d\t%d", &uid1, &uid2, &uid3, &uid4);
            if (ok >= 1) {
               proc->st_uid = uid1;
               success = true;
            }
            break;
         }
      }
      fclose(status);
   }
   if (!success) {
      snprintf(statusfilename, MAX_NAME, "%s/%s/stat", dirname, name);
      struct stat sstat;
      int statok = stat(statusfilename, &sstat);
      if (statok == -1)
         return false;
      proc->st_uid = sstat.st_uid;
   }
   return success;
}

void ProcessList_processEntries(ProcessList* this, char* dirname, int parent, float period) {
   DIR* dir;
   struct dirent* entry;
   Process* prototype = this->prototype;

   dir = opendir(dirname);
   assert(dir != NULL);
   while ((entry = readdir(dir)) != NULL) {
      char* name = entry->d_name;
      int pid;
      // filename is a number: process directory
      pid = atoi(name);

      // The RedHat kernel hides threads with a dot.
      // I believe this is non-standard.
      bool isThread = false;
      if ((!this->hideThreads) && pid == 0 && name[0] == '.') {
         char* tname = name + 1;
         pid = atoi(tname);
         if (pid > 0)
            isThread = true;
      }

      if (pid > 0 && pid != parent) {
         if (!this->hideUserlandThreads) {
            char subdirname[MAX_NAME+1];
            snprintf(subdirname, MAX_NAME, "%s/%s/task", dirname, name);
   
            if (access(subdirname, X_OK) == 0) {
               ProcessList_processEntries(this, subdirname, pid, period);
            }
         }

         FILE* status;
         char statusfilename[MAX_NAME+1];
         char command[PROCESS_COMM_LEN + 1];

         Process* process;
         Process* existingProcess = (Process*) Hashtable_get(this->processTable, pid);
         if (!existingProcess) {
            process = Process_clone(prototype);
            process->pid = pid;
            ProcessList_add(this, process);
            if (! ProcessList_readStatusFile(this, process, dirname, name))
               goto errorReadingProcess;
         } else {
            process = existingProcess;
         }
         process->updated = true;

         char* username = UsersTable_getRef(this->usersTable, process->st_uid);
         if (username) {
            strncpy(process->user, username, PROCESS_USER_LEN);
         } else {
            snprintf(process->user, PROCESS_USER_LEN, "%d", process->st_uid);
         }

         int lasttimes = (process->utime + process->stime);

         snprintf(statusfilename, MAX_NAME, "%s/%s/stat", dirname, name);
         
         status = ProcessList_fopen(this, statusfilename, "r");
         if (status == NULL) 
            goto errorReadingProcess;

         int success = ProcessList_readStatFile(this, process, status, command);
         fclose(status);
         if(!success) {
            goto errorReadingProcess;
         }

         process->percent_cpu = (process->utime + process->stime - lasttimes) / 
            period * 100.0;

         if(!existingProcess) {
            snprintf(statusfilename, MAX_NAME, "%s/%s/cmdline", dirname, name);
            status = ProcessList_fopen(this, statusfilename, "r");
            if (!status) {
               goto errorReadingProcess;
            }

            int amtRead = fread(command, 1, PROCESS_COMM_LEN - 1, status);
            if (amtRead > 0) {
               for (int i = 0; i < amtRead; i++)
                  if (command[i] == '\0' || command[i] == '\n')
                     command[i] = ' ';
               command[amtRead] = '\0';
            }
            command[PROCESS_COMM_LEN] = '\0';
            process->comm = String_copy(command);
            fclose(status);
         }

         snprintf(statusfilename, MAX_NAME, "%s/%s/statm", dirname, name);
         status = ProcessList_fopen(this, statusfilename, "r");

         if(!status) {
            goto errorReadingProcess;
         }
         int num = ProcessList_fread(this, status, "%d %d %d %d %d %d %d",
             &process->m_size, &process->m_resident, &process->m_share, 
             &process->m_trs, &process->m_drs, &process->m_lrs, 
             &process->m_dt);

         fclose(status);
         if(num != 7)
            goto errorReadingProcess;

         process->percent_mem = process->m_resident / 
            (float)(this->usedMem - this->cachedMem - this->buffersMem) * 
            100.0;

         this->totalTasks++;
         if (process->state == 'R') {
            this->runningTasks++;
         }

         if (this->hideKernelThreads && process->m_size == 0)
            ProcessList_remove(this, process);

         continue;

         // Exception handler.
         errorReadingProcess: {
            ProcessList_remove(this, process);
         }
      }
   }
   closedir(dir);
}

void ProcessList_scan(ProcessList* this) {
   long int usertime, nicetime, systemtime, idletime, totaltime;
   long int swapFree;

   FILE* status;
   char buffer[128];
   status = ProcessList_fopen(this, PROCMEMINFOFILE, "r");
   assert(status != NULL);
   while (!feof(status)) {
      fgets(buffer, 128, status);

      switch (buffer[0]) {
      case 'M':
         if (String_startsWith(buffer, "MemTotal:"))
            ProcessList_read(this, buffer, "MemTotal: %ld kB", &this->totalMem);
         else if (String_startsWith(buffer, "MemFree:"))
            ProcessList_read(this, buffer, "MemFree: %ld kB", &this->freeMem);
         else if (String_startsWith(buffer, "MemShared:"))
            ProcessList_read(this, buffer, "MemShared: %ld kB", &this->sharedMem);
         break;
      case 'B':
         if (String_startsWith(buffer, "Buffers:"))
            ProcessList_read(this, buffer, "Buffers: %ld kB", &this->buffersMem);
         break;
      case 'C':
         if (String_startsWith(buffer, "Cached:"))
            ProcessList_read(this, buffer, "Cached: %ld kB", &this->cachedMem);
         break;
      case 'S':
         if (String_startsWith(buffer, "SwapTotal:"))
            ProcessList_read(this, buffer, "SwapTotal: %ld kB", &this->totalSwap);
         if (String_startsWith(buffer, "SwapFree:"))
            ProcessList_read(this, buffer, "SwapFree: %ld kB", &swapFree);
         break;
      }
   }

   this->usedMem = this->totalMem - this->freeMem;
   this->usedSwap = this->totalSwap - swapFree;
   fclose(status);

   status = ProcessList_fopen(this, PROCSTATFILE, "r");

   assert(status != NULL);
   for (int i = 0; i <= this->processorCount; i++) {
      char buffer[256];
      int cpuid;
      long int ioWait, irq, softIrq, steal;
      ioWait = irq = softIrq = steal = 0;
      // Dependending on your kernel version,
      // 5, 7 or 8 of these fields will be set.
      // The rest will remain at zero.
      fgets(buffer, 255, status);
      if (i == 0)
         ProcessList_read(this, buffer, "cpu  %ld %ld %ld %ld %ld %ld %ld %ld", &usertime, &nicetime, &systemtime, &idletime, &ioWait, &irq, &softIrq, &steal);
      else {
         ProcessList_read(this, buffer, "cpu%d %ld %ld %ld %ld %ld %ld %ld %ld", &cpuid, &usertime, &nicetime, &systemtime, &idletime, &ioWait, &irq, &softIrq, &steal);
         assert(cpuid == i - 1);
      }
      // Fields existing on kernels >= 2.6
      // (and RHEL's patched kernel 2.4...)
      systemtime += ioWait + irq + softIrq + steal;
      totaltime = usertime + nicetime + systemtime + idletime;
      assert (usertime >= this->userTime[i]);
      assert (nicetime >= this->niceTime[i]);
      assert (systemtime >= this->systemTime[i]);
      assert (idletime >= this->idleTime[i]);
      assert (totaltime >= this->totalTime[i]);
      this->userPeriod[i] = usertime - this->userTime[i];
      this->nicePeriod[i] = nicetime - this->niceTime[i];
      this->systemPeriod[i] = systemtime - this->systemTime[i];
      this->idlePeriod[i] = idletime - this->idleTime[i];
      this->totalPeriod[i] = totaltime - this->totalTime[i];
      this->userTime[i] = usertime;
      this->niceTime[i] = nicetime;
      this->systemTime[i] = systemtime;
      this->idleTime[i] = idletime;
      this->totalTime[i] = totaltime;
   }
   float period = (float)this->totalPeriod[0] / this->processorCount;
   fclose(status);

   // mark all process as "dirty"
   for (int i = 0; i < Vector_size(this->processes); i++) {
      Process* p = (Process*) Vector_get(this->processes, i);
      p->updated = false;
   }
   
   this->totalTasks = 0;
   this->runningTasks = 0;
   
   ProcessList_processEntries(this, PROCDIR, 0, period);
   
   for (int i = Vector_size(this->processes) - 1; i >= 0; i--) {
      Process* p = (Process*) Vector_get(this->processes, i);
      if (p->updated == false)
         ProcessList_remove(this, p);
      else
         p->updated = false;
   }

}
