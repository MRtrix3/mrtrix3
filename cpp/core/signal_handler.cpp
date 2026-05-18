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

#include "signal_handler.h"

#include <atomic>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <vector>

#include "app.h"
#include "env.h"
#include "file/path.h"

#ifdef MRTRIX_WINDOWS
#define STDERR_FILENO 2 // check_syntax off
// #include <stdio.h>
// constexpr int STDERR_FILENO = GetStdHandle(STD_ERROR_HANDLE);
#endif

namespace MR::SignalHandler {

namespace {
std::vector<cleanup_function_type> cleanup_operations;

std::vector<std::filesystem::path> marked_files;
std::atomic_flag flag = ATOMIC_FLAG_INIT;

void delete_temporary_files() noexcept {
  // Use non-throwing version of std::filesystem::remove()
  std::error_code ec;
  for (const auto &i : marked_files)
    std::filesystem::remove(i, ec);
  marked_files.clear();
}

void handler(int i) noexcept {
  // Only process this once if using multi-threading:
  if (!flag.test_and_set()) {

    for (auto func : cleanup_operations)
      func();

    const char *sig = nullptr; // check_syntax off
    const char *msg = nullptr; // check_syntax off
    switch (i) {

#define MRTRIX_MANIP_SIGNAL(SIG, MSG)                                                                                  \
  case SIG:                                                                                                            \
    sig = #SIG;                                                                                                        \
    msg = MSG;                                                                                                         \
    break;
#include "signals.h"
#undef MRTRIX_MANIP_SIGNAL

    default:
      sig = "UNKNOWN";
      msg = "Unknown fatal system signal";
      break;
    }

    // Don't use std::cerr << here: Use basic C string-handling functions and a write() call to STDERR_FILENO
    // Don't attempt to use any terminal colouring
    char str[256]; // check_syntax off
    str[255] = '\0';
    snprintf(str, 255, "\n%s: [SYSTEM FATAL CODE: %s (%d)] %s\n", App::NAME.c_str(), sig, i, msg);
    (void)write(STDERR_FILENO, str, strnlen(str, 256));
    std::_Exit(i);
  }
}

} // namespace

void init() {
  on_signal(delete_temporary_files);

  // ENVVAR name: MRTRIX_NOSIGNALS
  // ENVVAR If this variable is set to any value, disable MRtrix3's custom
  // ENVVAR signal handlers. This may sometimes be useful when debugging.
  // ENVVAR Note however that this prevents the
  // ENVVAR deletion of temporary files when the command terminates
  // ENVVAR abnormally.
  if (MR::get_env("MRTRIX_NOSIGNALS").has_value())
    return;

#ifdef MRTRIX_WINDOWS
    // Use signal() rather than sigaction() for Windows, as the latter is not supported
#define MRTRIX_MANIP_SIGNAL(SIG, MSG) signal(SIG, handler)
#else
  // Construct the signal structure
  struct sigaction act;
  act.sa_handler = &handler;
  // Since we're _Exit()-ing for any of these signals, block them all
  sigfillset(&act.sa_mask);
  act.sa_flags = 0;
#define MRTRIX_MANIP_SIGNAL(SIG, MSG) sigaction(SIG, &act, nullptr)
#endif
#include "signals.h"
#undef MRTRIX_MANIP_SIGNAL
}

void on_signal(cleanup_function_type func) {
  cleanup_operations.push_back(func);
  std::atexit(func);
}

void mark_file_for_deletion(const std::filesystem::path &filepath) {
  while (!flag.test_and_set())
    ;
  marked_files.push_back(filepath);
  flag.clear();
}

void unmark_file_for_deletion(const std::filesystem::path &filepath) {
  while (!flag.test_and_set())
    ;
  auto i = marked_files.begin();
  while (i != marked_files.end()) {
    if (*i == filepath)
      i = marked_files.erase(i);
    else
      ++i;
  }
  flag.clear();
}

} // namespace MR::SignalHandler
