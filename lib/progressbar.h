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

#include "mrtrix.h"
#include "timer.h"
#include "types.h"
#include "math/math.h"

#define BUSY_INTERVAL 0.1

namespace MR {

  //! base class for the ProgressBar interface
  class ProgressInfo 
  {
    public:
      ProgressInfo () : as_percentage (false), value (0), data (NULL) { }
      ProgressInfo (const std::string& message, bool has_target) :
        as_percentage (has_target), value (0), text (message), data (NULL) { }

      //! is progress shown as a percentage or a busy indicator
      const bool as_percentage;

      //! the value of the progressbar
      /*! If the progress is shown as a percentage, this is the percentage
       * value. Otherwise, \a value is simply incremented at regular time
       * intervals. */
      size_t value; 
      //! the text to be shown with the progressbar
      const std::string text;
      //! a pointer to additional data required by alternative implementations
      void* data;
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
  class ProgressBar : private ProgressInfo
  {
    public:

      //! Create an unusable ProgressBar. 
      /*! This should not be used unless you need to initialise a member
       * ProgressBar within another class' constructor, and that ProgressBar
       * will never be used in that particular instance. */
      ProgressBar () : show (0) { }

      //! Create a new ProgressBar, displaying the specified text.
      /*! If \a target is unspecified or set to zero, the ProgressBar will
       * display a busy indicator, updated at regular time intervals.
       * Otherwise, the ProgressBar will display the percentage completed,
       * computed from the number of times the ProgressBar::operator++()
       * function was called relative to the value specified with \a target. */
      ProgressBar (const std::string& text, size_t target = 0) : 
        ProgressInfo (text, target),
        show (display),
        current_val (0) {
          if (show) {
            if (as_percentage) 
              set_max (target);
            else 
              next_val.d = BUSY_INTERVAL;
          }
        }

      ~ProgressBar () 
      { 
        if (show)
          done_func (*this);
      }

      //! returns whether the progress will be shown
      /*! The progress may not be shown if the -quiet option has been supplied
       * to the application.
        * \returns true if the progress will be shown, false otherwise. */
      operator bool () const { return (show); }

      //! returns whether the progress will be shown
      /*! The progress may not be shown if the -quiet option has been supplied
       * to the application.
       * \returns true if the progress will not be shown, false otherwise. */
      bool operator! () const { return (!show); }

      //! set the maximum target value of the ProgressBar
      /*! This function should only be called if the ProgressBar has been
       * created with a non-zero target value. In other words, the ProgressBar
       * has been created to display a percentage value, rather than a busy
       * indicator. */
      void set_max (size_t target) 
      {
        assert (target);
        assert (as_percentage);
        multiplier = 0.01 * target;
        next_val.i = multiplier;
        if (!next_val.i)
          next_val.i = 1;
      }

      //! increment the current value by one.
      void operator++ () 
      {
        if (show) {
          if (as_percentage) {
            ++current_val; 
            if (current_val >= next_val.i) {
              value = next_val.i / multiplier;
              next_val.i = (value+1) * multiplier;
              while (next_val.i <= current_val) 
                ++next_val.i;
              display_func (*this);
            }
          }
          else {
            double time = timer.elapsed();
            if (time >= next_val.d) {
              value = time / BUSY_INTERVAL;
              do {
                next_val.d += BUSY_INTERVAL;
              } while (next_val.d <= time);
              display_func (*this);
            }
          }
        }
      }

      void operator++ (int unused) { return ((*this)++); }

      static bool display;
      static void (*display_func) (ProgressInfo& p);
      static void (*done_func) (ProgressInfo& p);

    private:
      const bool show;
      size_t current_val;
      union { size_t i; double d; } next_val;
      float multiplier;
      Timer timer;
  };

}

#endif

