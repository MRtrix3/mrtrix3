/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

#include <array>
#include <cerrno>
#include <cstring>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

#include "app.h"
#include "debug.h"
#include "exception.h"
#include "file/config.h"
#include "mrtrix.h"

#ifdef MRTRIX_AS_R_LIBRARY
#include "wrap_r.h"
#endif

#ifdef MRTRIX_HAVE_STRERROR_R
namespace {
// Overloads resolve POSIX strerror_r (int return, fills buf) vs
// GNU strerror_r (char* return, may point to a static string).
std::string strerror_r_result(int /*errcode*/, const char *buf) { return std::string(buf); } // check_syntax off
std::string strerror_r_result(const char *result, const char * /*buf*/) {                    // check_syntax off
  return std::string(result);
}
} // namespace
#endif

namespace MR {

void display_exception_cmdline(const Exception &E, int log_level) {
  if (App::log_level >= log_level)
    for (const auto &n : E.description)
      report_to_user_func(n, log_level);
}

bool _need_newline = false;

void cmdline_report_to_user_func(std::string_view msg, int type) {

  static const std::unordered_map<int, std::string> colour_format_strings{{-1, "%s: %s%s\n"},
                                                                          {0, "%s: \033[01;31m%s%s\033[0m\n"},
                                                                          {1, "%s: \033[00;31m%s%s\033[0m\n"},
                                                                          {2, "%s: \033[00;32m%s%s\033[0m\n"},
                                                                          {3, "%s: \033[00;34m%s%s\033[0m\n"}};

  static const std::unordered_map<int, std::string> console_prefixes{
      {-1, ""}, {0, "[ERROR] "}, {1, "[WARNING] "}, {2, "[INFO] "}, {3, "[DEBUG] "}};

  if (_need_newline) {
    _print_stderr("\n");
    _need_newline = false;
  }

  auto clamp = [](int t) {
    if (t < -1 || t > 3)
      t = -1;
    return t + 1;
  };

  _print_stderr(printf(colour_format_strings.at(App::terminal_use_colour ? type : -1),
                       App::NAME.c_str(),
                       console_prefixes.at(type).c_str(),
                       std::string(msg).c_str()));
  if (type == 1 && App::fail_on_warn)
    throw Exception("terminating due to request to fail on warning");
}

void cmdline_print_func(std::string_view msg) {
#ifdef MRTRIX_AS_R_LIBRARY
  Rprintf(msg.c_str());
#else
  std::cout << msg;
#endif
}

const char * Exception::what() const noexcept { // check_syntax off
  static const std::string no_message("MR::Exception (no specific message)");
  return description.empty() ? no_message.c_str() : description.back().c_str();
}

void (*print)(std::string_view msg) = cmdline_print_func;
void (*report_to_user_func)(std::string_view msg, int type) = cmdline_report_to_user_func;
void (*Exception::display_func)(const Exception &E, int log_level) = display_exception_cmdline;

void check_app_exit_code() {
  if (App::exit_error_code != 0)
    throw Exception("Command performing delayed termination due to prior critical error");
}

std::string C_strerror(int errnum) {
#if defined(MRTRIX_HAVE_STRERROR_R)
  std::array<char, 256> buf = {};
  return strerror_r_result(strerror_r(errnum, buf.data(), buf.size()), buf.data()); // check_syntax off
#elif defined(MRTRIX_WINDOWS)
  std::array<char, 256> buf = {};
  strerror_s(buf.data(), buf.size(), errnum);
  return std::string(buf.data()); // check_syntax off
#else
  static std::mutex mutex;
  const std::lock_guard<std::mutex> lock(mutex);
  return std::string(std::strerror(errnum)); // NOLINT(concurrency-mt-unsafe) check_syntax off
#endif
}

} // namespace MR
