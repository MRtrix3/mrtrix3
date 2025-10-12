/* Copyright (c) 2008-2025 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#pragma once

#include "types.h"

#ifdef MRTRIX_AS_R_LIBRARY
#include "wrap_r.h"
#endif

#include <cerrno>
#include <exception>
#include <iostream>
#include <string>

namespace MR::App {
extern int log_level;
extern int exit_error_code;
} // namespace MR::App

namespace MR {

//! print primary output to stdout as-is.
/*! This function is intended for cases where the command's primary output is text, not
 * image data, etc. It is \e not designed for error or status reports: it
 * prints to stdout, whereas all reporting functions print to stderr. This is
 * to allow the output of the command to be used directly in text processing
 * pipeline or redirected to file.
 * \note the use of stdout is normally reserved for piping data files (or at
 * least their filenames) between MRtrix commands. This function should
 * therefore never be used in commands that produce output images, as the two
 * different types of output may then interfere and cause unexpected issues. */
extern void (*print)(std::string_view msg);

//! \cond skip

// for internal use only

inline void __print_stderr(std::string_view text) {
#ifdef MRTRIX_AS_R_LIBRARY
  REprintf(std::string(text).c_str());
#else
  std::cerr << text;
#endif
}
//! \endcond

//! display error, warning, debug, etc. message to user
/*! types are: 0: error; 1: warning; 2: additional information; 3:
 * debugging information; anything else: none. */
extern void (*report_to_user_func)(std::string_view msg, int type);

#define CONSOLE(msg)                                                                                                   \
  if (MR::App::log_level >= 1)                                                                                         \
  ::MR::report_to_user_func(msg, -1)
#define FAIL(msg)                                                                                                      \
  if (MR::App::log_level >= 0)                                                                                         \
  ::MR::report_to_user_func(msg, 0)
#define WARN(msg)                                                                                                      \
  if (MR::App::log_level >= 1)                                                                                         \
  ::MR::report_to_user_func(msg, 1)
#define INFO(msg)                                                                                                      \
  if (MR::App::log_level >= 2)                                                                                         \
  ::MR::report_to_user_func(msg, 2)
#define DEBUG(msg)                                                                                                     \
  if (MR::App::log_level >= 3)                                                                                         \
  ::MR::report_to_user_func(msg, 3)

class Exception : public std::exception {
public:
  Exception() {}

  Exception(std::string_view msg) { description.push_back(std::string(msg)); }
  Exception(const Exception &previous_exception, std::string_view msg) : description(previous_exception.description) {
    description.push_back(std::string(msg));
  }

  const char *what() const noexcept override; // check_syntax off

  void display(int log_level = 0) const { display_func(*this, log_level); }

  size_t num() const { return description.size(); }
  std::string_view operator[](size_t n) const { return description[n]; }
  void push_back(std::string_view s) { description.push_back(std::string(s)); }
  void push_back(const Exception &e) {
    for (auto s : e.description)
      push_back(s);
  }

  static void (*display_func)(const Exception &E, int log_level);

  std::vector<std::string> description;
};

class InvalidImageException : public Exception {
public:
  InvalidImageException(std::string_view msg) : Exception(msg) {}
  InvalidImageException(const Exception &previous_exception, std::string_view msg)
      : Exception(previous_exception, msg) {}
};

class CancelException : public Exception {
public:
  CancelException() : Exception("operation cancelled by user") {}
};

void display_exception_cmdline(const Exception &E, int log_level);
void cmdline_print_func(std::string_view msg);
void cmdline_report_to_user_func(std::string_view msg, int type);

class LogLevelLatch {
public:
  LogLevelLatch(const int new_level) : prev_level(App::log_level) { App::log_level = new_level; }
  ~LogLevelLatch() { App::log_level = prev_level; }

private:
  const int prev_level;
};

void check_app_exit_code();

} // namespace MR
