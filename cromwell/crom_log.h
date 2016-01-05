#ifndef __CROMWELL_LOG_H
#define __CROMWELL_LOG_H

namespace cromwell {

enum LogLevel {
  LEVEL_TRACE = 0,
  LEVEL_DEBUG,
  LEVEL_INFO,
  LEVEL_WARNING,
  LEVEL_ERROR,
  LEVEL_FATAL,
}

void set_log_level(LogLevel level);
LogLevel get_log_level();

bool init_log_file(const char* filename, LogLevel log_level);

void write_trace_log(const char* msg_fmt, ...);
void write_debug_log(const char* msg_fmt, ...);
void write_info_log(const char* msg_fmt, ...);
void write_warning_log(const char* msg_fmt, ...);
void write_error_log(const char* msg_fmt, ...);
void write_fatal_log(const char* msg_fmt, ...);

}//end-cromwell

#endif
