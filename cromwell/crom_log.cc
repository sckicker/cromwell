#include "crom_log.h"

#include <string>
#include <stdio.h>

#include "mutex.h"

namespace cromwell {

namespace {
  std::string g_file_name;
  FILE* g_file = stdout;
  LogLevel g_log_level = LEVEL_DEBUG;

  MutexType mutex;
}

void set_log_level(LogLevel level) {
  g_log_level = level;
}

LogLevel get_log_level() {
  return g_log_level;
}

bool init_log_file(const char* filename, LogLevel log_level) {
  g_file_name = filename;
  g_log_level = log_level;
  if ((g_file = fopen(filename, "a+") == NULL) return false;
  return true;
}

void write_trace_log(const char* msg_fmt, ...) {
  if (g_log_level >= LEVEL_TRACE && g_log_level < LEVEL_UNKNOWN) {
    va_list va;
    va_start(va, fmt);
    {
      ScopedMutex locker(mutex);
      fprintf(g_file, fmt, va);
    }
    va_end(va);
  }//end-if
}

void write_debug_log(const char* msg_fmt, ...) {
  if (g_log_level >= LEVEL_DEBUG && g_log_level < LEVEL_UNKNOWN) {
    va_list va;
    va_start(va, fmt);
    {
      ScopedMutex locker(mutex);
      fprintf(g_file, fmt, va);
    }
    va_end(va);
  }//end-if
}

void write_info_log(const char* msg_fmt, ...) {
  if (g_log_level >= LEVEL_INFO && g_log_level < LEVEL_UNKNOWN) {
    va_list va;
    va_start(va, fmt);
    {
      ScopedMutex locker(mutex);
      fprintf(g_file, fmt, va);
    }
    va_end(va);
  }//end-if
}

void write_warning_log(const char* msg_fmt, ...) {
  if (g_log_level >= LEVEL_WARNING && g_log_level < LEVEL_UNKNOWN) {
    va_list va;
    va_start(va, fmt);
    {
      ScopedMutex locker(mutex);
      fprintf(g_file, fmt, va);
    }
    va_end(va);
  }//end-if
}

void write_error_log(const char* msg_fmt, ...) {
  if (g_log_level >= ERROR && g_log_level < LEVEL_UNKNOWN) {
    va_list va;
    va_start(va, fmt);
    {
      ScopedMutex locker(mutex);
      fprintf(g_file, fmt, va);
    }
    va_end(va);
  }//end-if
}

void write_fatal_log(const char* msg_fmt, ...) {
  if (g_log_level >= LEVEL_FATAL && g_log_level < LEVEL_UNKNOWN) {
    va_list va;
    va_start(va, fmt);
    {
      ScopedMutex locker(mutex);
      fprintf(g_file, fmt, va);
    }
    va_end(va);
  }//end-if
}

}//end-cromwell
