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

#include "progressbar.h"
#include "app.h"
#include <thread>
#include <utility>

// MSYS2 supports VT100, and file redirection is handled explicitly so this can be used globally
#define CLEAR_LINE_CODE "\033[0K" // check_syntax off
#define WRAP_ON_CODE "\033[?7h"   // check_syntax off
#define WRAP_OFF_CODE "\033[?7l"  // check_syntax off

namespace MR {

extern bool __need_newline;

namespace {

const std::array<std::string, 6> busy{".   ", " .  ", "  . ", "   .", "  . ", " .  "};

void display_func_multithreaded(const ProgressBar &p) {
  ProgressBar::notification_is_genuine = true;
  ProgressBar::notifier.notify_all();
}

void display_func_terminal(const ProgressBar &p) {
  __need_newline = true;
  if (p.show_percent())
    __print_stderr(printf(WRAP_OFF_CODE "\r%s: [%3" PRI_SIZET "%%] %s%s" CLEAR_LINE_CODE WRAP_ON_CODE,
                          App::NAME.c_str(),
                          p.value(),
                          p.text_cstr(),
                          p.ellipsis_cstr()));
  else
    __print_stderr(printf(WRAP_OFF_CODE "\r%s: [%s] %s%s" CLEAR_LINE_CODE WRAP_ON_CODE,
                          App::NAME.c_str(),
                          busy[p.value() % busy.size()].c_str(),
                          p.text_cstr(),
                          p.ellipsis_cstr()));
}

void done_func_terminal(const ProgressBar &p) {
  if (p.show_percent())
    __print_stderr(printf("\r%s: [100%%] %s" CLEAR_LINE_CODE "\n", App::NAME.c_str(), p.text_cstr()));
  else
    __print_stderr(printf("\r%s: [done] %s" CLEAR_LINE_CODE "\n", App::NAME.c_str(), p.text_cstr()));
  __need_newline = false;
}

void display_func_redirect(const ProgressBar &p) {
  static size_t count = 0;
  static size_t next_update_at = 0;
  // need to update whole line since text may have changed:
  if (p.text_has_been_modified()) {
    __need_newline = false;
    if (p.value() == 0 && p.count() == 0)
      count = next_update_at = 0;
    if (count++ == next_update_at) {
      if (p.show_percent()) {
        __print_stderr(
            printf("%s: [%3" PRI_SIZET "%%] %s%s\n", App::NAME.c_str(), p.value(), p.text_cstr(), p.ellipsis_cstr()));
        ;
      } else {
        __print_stderr(printf("%s: [%s] %s%s\n",
                              App::NAME.c_str(),
                              busy[p.value() % busy.size()].c_str(),
                              p.text_cstr(),
                              p.ellipsis_cstr()));
      }
      if (next_update_at)
        next_update_at *= 2;
      else
        next_update_at = 1;
    }
  }
  // text is static - can simply append to the current line:
  else {
    __need_newline = true;
    if (p.show_percent()) {
      if (p.first_time) {
        p.first_time = false;
        __print_stderr(printf("%s: %s%s [", App::NAME.c_str(), p.text_cstr(), p.ellipsis_cstr()));
        ;
      } else
        while (p.last_value < p.value()) {
          __print_stderr(printf("="));
          p.last_value += 2;
        }
    } else {
      if (p.value() == 0) {
        __print_stderr(printf("%s: %s%s ", App::NAME.c_str(), p.text_cstr(), p.ellipsis_cstr()));
        ;
      } else if (!(p.value() & (p.value() - 1))) {
        __print_stderr(".");
      }
    }
  }
}

void done_func_redirect(const ProgressBar &p) {
  if (p.text_has_been_modified()) {
    if (p.show_percent()) {
      __print_stderr(printf("%s: [100%%] %s\n", App::NAME.c_str(), p.text_cstr()));
      ;
    } else {
      __print_stderr(printf("%s: [done] %s\n", App::NAME.c_str(), p.text_cstr()));
    }
  } else {
    if (p.show_percent())
      __print_stderr(printf("]\n"));
    else
      __print_stderr(printf("done\n"));
  }
  __need_newline = false;
}

} // namespace

const default_type ProgressBar::busy_interval = 0.1;

void (*ProgressBar::display_func)(const ProgressBar &p) = display_func_terminal;
void (*ProgressBar::done_func)(const ProgressBar &p) = done_func_terminal;
void (*ProgressBar::previous_display_func)(const ProgressBar &p) = nullptr;

std::condition_variable ProgressBar::notifier;
bool ProgressBar::notification_is_genuine = false;
std::mutex ProgressBar::mutex;
void *ProgressBar::data = nullptr;
bool ProgressBar::progressbar_active = false;

ProgressBar::SwitchToMultiThreaded::SwitchToMultiThreaded() {
  ProgressBar::previous_display_func = ProgressBar::display_func;
  ProgressBar::display_func = display_func_multithreaded;
}

ProgressBar::SwitchToMultiThreaded::~SwitchToMultiThreaded() {
  ProgressBar::display_func = ProgressBar::previous_display_func;
  ProgressBar::previous_display_func = nullptr;
}

void (*previous_display_func)(ProgressBar &p);

bool ProgressBar::set_update_method() {
  bool stderr_to_file = false;

  struct stat buf;
  if (fstat(STDERR_FILENO, &buf))
    // unable to determine nature of stderr; assuming socket
    stderr_to_file = false;
  else
    stderr_to_file = S_ISREG(buf.st_mode) || S_ISFIFO(buf.st_mode);

  if (stderr_to_file) {
    ProgressBar::display_func = display_func_redirect;
    ProgressBar::done_func = done_func_redirect;
  } else {
    ProgressBar::display_func = display_func_terminal;
    ProgressBar::done_func = done_func_terminal;
  }

  return stderr_to_file;
}

ProgressBar::ProgressBar(std::string_view text, size_t target, int log_level)
    : first_time(true),
      last_value(0),
      show(std::this_thread::get_id() == ::MR::App::main_thread_ID && !progressbar_active &&
           App::log_level >= log_level),
      _text(text),
      _ellipsis("..."),
      _value(0),
      current_val(0),
      next_percent(0),
      next_time(0.0),
      _multiplier(0.0),
      _text_has_been_modified(false) {
  if (show) {
    set_max(target);
    progressbar_active = true;
  }
}

ProgressBar::ProgressBar(ProgressBar &&other) noexcept
    : show(other.show),
      _text(std::move(other._text)),
      _ellipsis(std::move(other._ellipsis)),
      _value(other._value),
      current_val(other.current_val),
      next_percent(other.next_percent),
      next_time(other.next_time),
      _multiplier(other._multiplier),
      timer(other.timer),
      _text_has_been_modified(other._text_has_been_modified),
      first_time(other.first_time),
      last_value(other.last_value) {
  other.show = false;
}

ProgressBar &MR::ProgressBar::operator=(ProgressBar &&other) noexcept {
  if (this == &other) {
    return *this;
  }

  // If the current object is managing an active progress bar, finish it.
  if (show) {
    done();
  }

  _text = std::move(other._text);
  _ellipsis = std::move(other._ellipsis);
  show = other.show;
  _value = other._value;
  current_val = other.current_val;
  next_percent = other.next_percent;
  next_time = other.next_time;
  _multiplier = other._multiplier;
  timer = other.timer;
  _text_has_been_modified = other._text_has_been_modified;
  first_time = other.first_time;
  last_value = other.last_value;
  other.show = false;
  return *this;
}

} // namespace MR
