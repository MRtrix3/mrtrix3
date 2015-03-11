/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef __progressbar_h__
#define __progressbar_h__

#include <string>
#include <memory>


#include "mrtrix.h"
#include "timer.h"
#include "types.h"
#include "math/math.h"
#include "debug.h"

#define BUSY_INTERVAL 0.1

namespace MR
{

  //! base class for the ProgressBar interface
  class ProgressInfo
  {
    public:
      ProgressInfo () = delete;
      ProgressInfo (const ProgressInfo& p) = delete;
      ProgressInfo (ProgressInfo&& p) = delete; 

      ProgressInfo (const std::string& text, size_t target) :
        value (0), text (text), current_val (0), multiplier (0.0), data (nullptr) { 
          set_max (target);
        }

      ~ProgressInfo  () {
        done_func (*this);
      }

      //! the value of the progressbar
      /*! If the progress is shown as a percentage, this is the percentage
       * value. Otherwise, \a value is simply incremented at regular time
       * intervals. */
      size_t value;
      //! the text to be shown with the progressbar
      std::string text;

      //! the current absolute value 
      /*! only used when progress is shown as a percentage */
      size_t current_val;

      //! next value to trigger a UI update
      /*! when progress is shown as a percentage, \c next_val.i is the value of
       * \c current_val that will trigger the next update. Otherwise, \c
       * next_val.d is the time interval (from the start of the progressbar)
       * that will trigger the next update. */
      union {
        size_t i;
        double d;
      } next_val;

      //! the factor to convert from absolute value to percentage
      /*! if zero, the progressbar is used as a busy indicator */
      float multiplier;

      //! used for busy indicator.
      Timer timer;
      //
      //! a pointer to additional data required by alternative implementations
      void* data;

      void set_max (size_t target) {
        if (target) {
          multiplier = 0.01 * target;
          next_val.i = multiplier;
          if (!next_val.i)
            next_val.i = 1;
        }
        else {
          multiplier = 0.0;
          next_val.d = BUSY_INTERVAL;
          timer.start();
        }
        display_func (*this);
      }

      void set_text (const std::string& new_text) {
        text = new_text;
        display_func (*this);
      };

      //! increment the current value by one.
      void operator++ () {
        if (multiplier) {
          if (++current_val >= next_val.i) {
            value = current_val / multiplier;
            next_val.i = std::ceil ((value+1.0) * multiplier);
            display_func (*this);
          }
        }
        else {
          double time = timer.elapsed();
          if (time >= next_val.d) {
            value = time / BUSY_INTERVAL;
            do {
              next_val.d += BUSY_INTERVAL;
            }
            while (next_val.d <= time);
            display_func (*this);
          }
        }
      }

      void operator++ (int) {
        ++ (*this);
      }

      static void (*display_func) (ProgressInfo& p);
      static void (*done_func) (ProgressInfo& p);

  };



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
  class ProgressBar 
  {
    public:

      //! Create an unusable ProgressBar.
      ProgressBar () : show (false) { }

      ProgressBar (const ProgressBar& p) : 
        show (p.show), text (p.text), target (p.target) {
          assert (!p.prog);
        }

      //! Create a new ProgressBar, displaying the specified text.
      /*! If \a target is unspecified or set to zero, the ProgressBar will
       * display a busy indicator, updated at regular time intervals.
       * Otherwise, the ProgressBar will display the percentage completed,
       * computed from the number of times the ProgressBar::operator++()
       * function was called relative to the value specified with \a target. */
      ProgressBar (const std::string& text, size_t target = 0, int log_level = 1) :
        show (App::log_level >= log_level), text (text), target (target) { }

      //! returns whether the progress will be shown
      /*! The progress may not be shown if the -quiet option has been supplied
       * to the application.
        * \returns true if the progress will be shown, false otherwise. */
      operator bool () const {
        return show;
      }

      //! returns whether the progress will be shown
      /*! The progress may not be shown if the -quiet option has been supplied
       * to the application.
       * \returns true if the progress will not be shown, false otherwise. */
      bool operator! () const {
        return !show;
      }

      //! set the maximum target value of the ProgressBar
      /*! This function should only be called if the ProgressBar has been
       * created with a non-zero target value. In other words, the ProgressBar
       * has been created to display a percentage value, rather than a busy
       * indicator. */
      void set_max (size_t new_target) {
        target = new_target;
        if (show && prog)
          prog->set_max (target);
      }

      void set_text (const std::string& new_text) {
        text = new_text;
        if (show && prog)
          prog->set_text (new_text);
      };

      //! increment the current value by one.
      void operator++ () {
        if (show) {
          if (!prog) 
            prog = std::unique_ptr<ProgressInfo> (new ProgressInfo (text, target));
          (*prog)++;
        }
      }

      void operator++ (int) {
        ++ (*this);
      }

      void done () {
        prog.reset();
      }

    private:
      const bool show;
      std::string text;
      size_t target;
      std::unique_ptr<ProgressInfo> prog;
  };

}

#endif

