#include "crom_log.h"

#include <string>
#include <stdio.h>

namespace cromwell {

static std::string g_file_name;
static FILE* g_file = stdout;
static LogLevel g_log_level = LEVEL_DEBUG;

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
    fprintf(g_file, fmt, va);
    va_end(va);
  }
}

void write_debug_log(const char* msg_fmt, ...) {
  if (g_log_level >= LEVEL_DEBUG && g_log_level < LEVEL_UNKNOWN) {
    va_list va;
    va_start(va, fmt);
    fprintf(g_file, fmt, va);
    va_end(va);
  }
}

void write_info_log(const char* msg_fmt, ...) {
  if (g_log_level >= LEVEL_INFO && g_log_level < LEVEL_UNKNOWN) {
    va_list va;
    va_start(va, fmt);
    fprintf(g_file, fmt, va);
    va_end(va);
  }
}

void write_warning_log(const char* msg_fmt, ...) {
  if (g_log_level >= LEVEL_WARNING && g_log_level < LEVEL_UNKNOWN) {
    va_list va;
    va_start(va, fmt);
    fprintf(g_file, fmt, va);
    va_end(va);
  }
}

void write_error_log(const char* msg_fmt, ...) {
  if (g_log_level >= ERROR && g_log_level < LEVEL_UNKNOWN) {
    va_list va;
    va_start(va, fmt);
    fprintf(g_file, fmt, va);
    va_end(va);
  }
}

void write_fatal_log(const char* msg_fmt, ...) {
  if (g_log_level >= LEVEL_FATAL && g_log_level < LEVEL_UNKNOWN) {
    va_list va;
    va_start(va, fmt);
    fprintf(g_file, fmt, va);
    va_end(va);
  }
}

}//end-cromwell
