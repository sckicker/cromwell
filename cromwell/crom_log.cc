#include "crom_log.h"

#include <string>
#include <iostream>

namespace cromwell {


static std::string g_file_name;
static FILE* g_file;
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
  return true;
}

void write_trace_log(const char* msg_fmt, ...) {
    
}

}//end-cromwell
