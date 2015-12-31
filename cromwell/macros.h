#ifndef __MACROS_H
#define __MACROS_H

namespace cromwell {

#ifdef __APPLE__
#include <AvailabilityMacros.h>
#endif

#ifdef __linux__
#include <linux/version.h>
#include <features.h>
#endif

/* Test for polling API */
#ifdef __linux__
#define HAVE_EPOLL 1
#endif

#if (defined(__APPLE__) && defined(MAC_OS_X_VERSION_10_6)) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined (__NetBSD__)
#define HAVE_KQUEUE 1
#endif

#ifndef BlankChr
#define BlankChr ' '
#endif

#ifndef PRINT_STRING
#define PRINT_STRING(name, str) \
    do { \
    char buf[1024]; \
    snprintf(buf, 1024, "%s: [%s]", #name, str); \
    printf("%s\n", buf); \
    } while(0)
#endif

// Only for char[n]
#ifndef FILL_BLANK
#define FILL_BLANK(str) memset(str, BlankChr, sizeof(str))
#endif

#ifndef FILL_WORD
#define FILL_WORD(dest_str, src_str) \
    do { \
    memset(dest_str, BlankChr, sizeof(dest_str)); \
    memcpy(dest_str, src_str, sizeof(dest_str) < sizeof(src_str) ? sizeof(dest_str) : sizeof(src_str)); \
    } while(0)
#endif

#ifndef FILL_WORD_FROM_INT64
#define FILL_WORD_FROM_INT64(str, i) \
    do { \
    memset(str, BlankChr, sizeof(str)); \
    snprintf(str, sizeof(str), "%lld", i); \
    } while(0)
#endif

#ifndef FILL_WORD_FROM_INT
#define FILL_WORD_FROM_INT(str, i) \
    do { \
    memset(str, BlankChr, sizeof(str)); \
    snprintf(str, sizeof(str), "%d", i); \
    } while(0)
#endif

#ifndef FILL_STR
#define FILL_STR(dest_str, src_str) \
    do { \
	memset(dest_str, ' ', sizeof(dest_str)); \
	size_t src_str_len = strlen(src_str); \
	memcpy(dest_str, src_str, src_str_len < sizeof(dest_str) ? src_str_len : sizeof(dest_str)); \
	} while(0);
#endif

#ifndef ASSERT
#define ASSERT(op) \
  assert(op)
#endif

#ifndef MEGA_CHECK
#define MEGA_CHECK(fg) \
	do { \
		if (!fg) return -1; \
	} while(0)
#endif

#ifndef MEGA_DELETE
#define MEGA_DELETE(member) \
  do { \
    if(member) { \
      delete member; \
      member = nullptr; \
    } \
  } while(0)
#endif

#ifndef LOG_TRACE
#define LOG_TRACE(logFmt, ...) trace(logFmt, __VA_ARGS__)
#endif

#ifndef LOG_DEBUG
#define LOG_DEBUG(logFmt, ...) write_debug_log(logFmt, __VA_ARGS__)
#endif

#ifndef LOG_INFO
#define LOG_INFO(logFmt, ...) write_info_log(logFmt, __VA_ARGS__)
#endif

#ifndef LOG_WARN
#define LOG_WARN(logFmt, ...) write_warning_log(logFmt, __VA_ARGS__)
#endif

#ifndef LOG_ERROR
#define LOG_ERROR(logFmt, ...) write_error_log(logFmt, __VA_ARGS__)
#endif

#ifndef LOG_FATAL
#define LOG_FATAL(logFmt, ...) write_fatal_log(logFmt, __VA_ARGS__)
#endif

}//end-cromwell.

#endif
