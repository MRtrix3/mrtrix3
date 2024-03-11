/* Copyright (c) 2008-2024 the MRtrix3 contributors.
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

#ifndef __progressbar_h__
#define __progressbar_h__

#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "app.h"
#include "debug.h"
#include "mrtrix.h"
#include "timer.h"
#include "types.h"

#define BUSY_INTERVAL 0.1

namespace MR {

//! implements a progress meter to provide feedback to the user
/*! The ProgressBar class displays a text message along with a indication of
 * the progress status. For command-line applications, this will be shown on
 * the terminal. For GUI applications, this will be shown as a graphical
 * progress bar.
 *
 * It has two modes of operation:
 * - percentage completion: if the maximum value is non-zero, then the
 * percentage completed will be displayed. Each call to
 * ProgressBar::operator++() will increment the value by one, and the
 * percentage displayed is computed from the current value with respect to
 * the maximum specified.
 * - busy indicator: if the maximum value is set to zero, then a 'busy'
 * indicator will be shown instead. For the command-line version, this
 * consists of a dot moving from side to side.
 *
 * Other implementations can be created by overriding the display_func() and
 * done_func() static functions. These functions will then be used throughout
 * the application.  */
class ProgressBar {
public:
  //! Create an unusable ProgressBar.
  ProgressBar() : show(false) {}
  ProgressBar(const ProgressBar &p) = delete;
  ProgressBar(ProgressBar &&p) = default;

  FORCE_INLINE ~ProgressBar() { done(); }

  //! Create a new ProgressBar, displaying the specified text.
  /*! If \a target is unspecified or set to zero, the ProgressBar will
   * display a busy indicator, updated at regular time intervals.
   * Otherwise, the ProgressBar will display the percentage completed,
   * computed from the number of times the ProgressBar::operator++()
   * function was called relative to the value specified with \a target. */
  ProgressBar(const std::string &text, size_t target = 0, int log_level = 1);

  //! returns whether the progress will be shown
  /*! The progress may not be shown if the -quiet option has been supplied
   * to the application.
   * \returns true if the progress will be shown, false otherwise. */
  FORCE_INLINE operator bool() const { return show; }

  //! returns whether the progress will be shown
  /*! The progress may not be shown if the -quiet option has been supplied
   * to the application.
   * \returns true if the progress will not be shown, false otherwise. */
  FORCE_INLINE bool operator!() const { return !show; }

  FORCE_INLINE size_t value() const { return _value; }
  FORCE_INLINE size_t count() const { return current_val; }
  FORCE_INLINE bool show_percent() const { return _multiplier; }
  FORCE_INLINE bool text_has_been_modified() const { return _text_has_been_modified; }
  FORCE_INLINE const std::string &text() const { return _text; }
  FORCE_INLINE const std::string &ellipsis() const { return _ellipsis; }

  //! set the maximum target value of the ProgressBar
  /*! This function should only be called if the ProgressBar has been
   * created with a non-zero target value. In other words, the ProgressBar
   * has been created to display a percentage value, rather than a busy
   * indicator. */
  FORCE_INLINE void set_max(size_t new_target);

  FORCE_INLINE void set_text(const std::string &new_text);

  //! update text displayed and optionally increment counter
  /*! This expects a function, functor or lambda function that should
   * return a std::string to replace the text. This functor will only be
   * called when necessary, i.e. when BUSY_INTERVAL time has elapsed, or if
   * the percentage value to display has changed. The reason for passing a
   * functor rather than the text itself is to minimise the overhead of
   * forming the string in cases where this is sufficiently expensive to
   * impact performance if invoked every iteration. By passing a function,
   * this operation is only performed when strictly necessary.
   *
   * The simplest way to use this method is using C++11 lambda functions,
   * for example:
   * \code
   * progress.update ([&](){ return "current energy = " + str(energy_value); });
   * \endcode
   *
   * \note due to this lazy update, the text is not guaranteed to be up to
   * date by the time processing is finished. If this is important, you
   * should also use the set_text() method to set the final text displayed
   * before the ProgressBar's done() function is called (typically in the
   * destructor when it goes out of scope).*/
  template <class TextFunc> FORCE_INLINE void update(TextFunc &&text_func, bool increment = true);

  //! increment the current value by one.
  FORCE_INLINE void operator++();
  FORCE_INLINE void operator++(int) { ++(*this); }

  FORCE_INLINE void done() {
    if (show) {
      done_func(*this);
      progressbar_active = false;
    }
  }

  template <class ThreadType> void run_update_thread(const ThreadType &threads) const;

  struct SwitchToMultiThreaded {
    SwitchToMultiThreaded();
    ~SwitchToMultiThreaded();
  };

  static bool set_update_method();
  static void (*display_func)(const ProgressBar &p);
  static void (*done_func)(const ProgressBar &p);
  static void (*previous_display_func)(const ProgressBar &p);

  static std::condition_variable notifier;
  static bool notification_is_genuine;
  static std::mutex mutex;
  ;
  static void *data;

  mutable bool first_time;
  mutable size_t last_value;

private:
  const bool show;
  std::string _text, _ellipsis;
  size_t _value, current_val, next_percent;
  double next_time;
  float _multiplier;
  Timer timer;
  bool _text_has_been_modified;

  FORCE_INLINE void display_now() { display_func(*this); }

  static bool progressbar_active;
};

FORCE_INLINE ProgressBar::ProgressBar(const std::string &text, size_t target, int log_level)
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

inline void ProgressBar::set_max(size_t target) {
  if (!show)
    return;
  if (target) {
    _multiplier = 0.01 * target;
  } else {
    _multiplier = 0.0;
    timer.start();
  }
}

FORCE_INLINE void ProgressBar::set_text(const std::string &new_text) {
  if (!show)
    return;
  _text_has_been_modified = true;
  if (new_text.size()) {
#ifdef MRTRIX_WINDOWS
    size_t old_size = _text.size();
#endif
    _text = new_text;
#ifdef MRTRIX_WINDOWS
    if (_text.size() < old_size)
      _text.resize(old_size, ' ');
#endif
  }
}

template <class TextFunc> FORCE_INLINE void ProgressBar::update(TextFunc &&text_func, const bool increment) {
  if (!show)
    return;
  double time = timer.elapsed();
  const std::unique_lock<std::mutex> lock(mutex);
  if (increment && _multiplier) {
    if (++current_val >= next_percent) {
      set_text(text_func());
      _ellipsis.clear();
      _value = std::round(current_val / _multiplier);
      next_percent = std::ceil((_value + 1) * _multiplier);
      next_time = time;
      display_now();
      return;
    }
  }
  if (time >= next_time) {
    set_text(text_func());
    _ellipsis.clear();
    if (_multiplier)
      next_time = time + BUSY_INTERVAL;
    else {
      _value = time / BUSY_INTERVAL;
      do {
        next_time += BUSY_INTERVAL;
      } while (next_time <= time);
    }
    display_now();
  }
}

FORCE_INLINE void ProgressBar::operator++() {
  if (!show)
    return;
  const std::unique_lock<std::mutex> lock(mutex);
  if (_multiplier) {
    if (++current_val >= next_percent) {
      _value = std::round(current_val / _multiplier);
      next_percent = std::ceil((_value + 1) * _multiplier);
      display_now();
    }
  } else {
    double time = timer.elapsed();
    if (time >= next_time) {
      _value = time / BUSY_INTERVAL;
      do {
        next_time += BUSY_INTERVAL;
      } while (next_time <= time);
      display_now();
    }
  }
}

template <class ThreadType> inline void ProgressBar::run_update_thread(const ThreadType &threads) const {
  if (!show)
    return;
  std::unique_lock<std::mutex> lock(mutex);
  while (!threads.finished()) {
    notifier.wait_for(lock, std::chrono::milliseconds(1), [] { return notification_is_genuine; });
    if (notification_is_genuine) {
      previous_display_func(*this);
      notification_is_genuine = false;
    }
  }
}

} // namespace MR

#endif
