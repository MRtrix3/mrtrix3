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

#pragma once

#ifdef FLUSH_TO_ZERO
#include <xmmintrin.h>
#endif

#include <cstdio>
#include <initializer_list>
#include <optional>

#include "app.h"
#include "env.h"
#include "exception.h"
#include "executable_version.h"
#include "mrtrix.h"
#include "mrtrix_version.h"
#ifdef MRTRIX_PROJECT
namespace MR::App {
void set_project_version();
} // namespace MR::App
#endif

#define MRTRIX_UPDATED_API

#ifdef MRTRIX_AS_R_LIBRARY

extern "C" void R_main(int *cmdline_argc, char **cmdline_argv) { // check_syntax off
  // Non-throwing [ERROR] line writer for the R-hosted entry point; routes via
  // REprintf (R's stderr-equivalent channel) and mirrors the "<name>: [colour][ERROR]
  // <msg>[reset]" layout produced by MR::cmdline_report_to_user_func. Uses only
  // noexcept primitives so it is safe inside catch handlers
  // (see bugprone-exception-escape). The "<name>: " prefix appears only once
  // ::MR::App::init() has populated ::MR::App::NAME.
  const auto fail = [](std::initializer_list<const char *> parts) noexcept { // check_syntax off
    if (!::MR::App::NAME.empty())
      REprintf("%s: ", ::MR::App::NAME.c_str());
    if (::MR::App::terminal_use_colour)
      REprintf("%s", "\033[01;31m");
    REprintf("%s", "[ERROR] ");
    for (const char *const p : parts) // check_syntax off
      REprintf("%s", p);
    if (::MR::App::terminal_use_colour)
      REprintf("%s", "\033[0m");
    REprintf("%s", "\n");
  };

  try {
#ifdef MRTRIX_PROJECT
    ::MR::App::set_project_version();
#endif
    ::MR::App::DESCRIPTION.clear();
    ::MR::App::ARGUMENTS.clear();
    ::MR::App::OPTIONS.clear();
    usage();
    ::MR::App::verify_usage();
    ::MR::App::init(*cmdline_argc, cmdline_argv);
    ::MR::App::parse();
    run();
  } catch (int retval) {
    return;
  } catch (MR::Exception &E) {
    try {
      E.display();
    } catch (...) {
      fail({"additional exception raised while displaying MR::Exception"});
    }
    return;
  } catch (const std::exception &E) {
    fail({"unhandled std::exception escaped from R_main: ", E.what()});
    return;
  } catch (...) { // NOLINT(bugprone-empty-catch)
    fail({"unhandled non-std::exception escaped from R_main"});
    return;
  }
}

extern "C" void R_usage(char **output) { // check_syntax off
  ::MR::App::DESCRIPTION.clear();
  ::MR::App::ARGUMENTS.clear();
  ::MR::App::OPTIONS.clear();
  usage();
  std::string s = MR::App::full_usage();
  *output = new char[s.size() + 1];
  strncpy(*output, s.c_str(), s.size() + 1); // check_syntax off
}

#else

int main(int cmdline_argc, char **cmdline_argv) { // check_syntax off
  // Non-throwing [ERROR] line writer; mirrors the "<name>: [colour][ERROR] <msg>[reset]"
  // layout produced by MR::cmdline_report_to_user_func, but uses only noexcept
  // primitives so it is safe to call from outside the main try block and from
  // catch handlers (see bugprone-exception-escape). The "<name>: " prefix appears
  // only once ::MR::App::init() has populated ::MR::App::NAME.
  const auto fail = [](std::initializer_list<const char *> parts) noexcept { // check_syntax off
    if (!::MR::App::NAME.empty()) {
      std::fputs(::MR::App::NAME.c_str(), stderr);
      std::fputs(": ", stderr);
    }
    if (::MR::App::terminal_use_colour)
      std::fputs("\033[01;31m", stderr);
    std::fputs("[ERROR] ", stderr);
    for (const char *const p : parts) // check_syntax off
      std::fputs(p, stderr);
    if (::MR::App::terminal_use_colour)
      std::fputs("\033[0m", stderr);
    std::fputc('\n', stderr);
  };

  // Version mismatch is reported via the noexcept "fail" helper so the check
  // can safely run before the main try block (see bugprone-exception-escape).
  if (MR::App::mrtrix_version != MR::App::mrtrix_executable_version) {
    fail({"executable was compiled for a different version of the MRtrix3 library!"});
    fail({"  executable version: ", MR::App::mrtrix_executable_version.c_str()});
    fail({"  library version: ", MR::App::mrtrix_version.c_str()});
    fail({"You may need to erase files left over from prior MRtrix3 versions;"});
    fail({"eg. core/version.cpp; src/exec_version.cpp,"});
    fail({"and re-configure cmake"});
    return 1;
  }

#ifdef FLUSH_TO_ZERO
  // use gcc switches: -msse -mfpmath=sse -ffast-math
  int mxcsr = _mm_getcsr();
  // Sets denormal results from floating-point calculations to zero:
  mxcsr |= (1 << 15) | (1 << 11); // flush-to-zero
  // Treats denormal values used as input to floating-point instructions as zero:
  mxcsr |= (1 << 6); // denormals-are-zero
  _mm_setcsr(mxcsr);
#endif
  try {
#ifdef MRTRIX_PROJECT
    ::MR::App::set_project_version();
#endif
    ::MR::App::init(cmdline_argc, cmdline_argv);
    usage();
    ::MR::App::verify_usage();
    ::MR::App::parse_special_options();
#ifdef GUI_APP_H
    const ::MR::GUI::App app(cmdline_argc, cmdline_argv);
#endif
    ::MR::App::parse();

    // ENVVAR name: MRTRIX_CLI_PARSE_ONLY
    // ENVVAR Set the command to parse the provided inputs and then quit
    // ENVVAR if it is set. This can be used in the CI of wrapping code,
    // ENVVAR such as the automatically generated Pydra interfaces.
    // ENVVAR Note that it will have no effect for R interfaces
    const std::optional<std::string> parse_only = ::MR::get_env("MRTRIX_CLI_PARSE_ONLY");
    if (parse_only.has_value() && ::MR::to<bool>(*parse_only)) {
      CONSOLE("Quitting after parsing command-line arguments successfully due to environment variable "
              "'MRTRIX_CLI_PARSE_ONLY'");
      return 0;
    }
    run();
  } catch (int retval) {
    return retval;
  } catch (::MR::Exception &E) {
    try {
      E.display();
    } catch (...) {
      fail({"additional exception raised while displaying MR::Exception"});
    }
    return 1;
  } catch (const std::exception &E) {
    fail({"unhandled std::exception escaped from main: ", E.what()});
    return 1;
  } catch (...) { // NOLINT(bugprone-empty-catch)
    fail({"unhandled non-std::exception escaped from main"});
    return 1;
  }
  return ::MR::App::exit_error_code;
}

#endif
