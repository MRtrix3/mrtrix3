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

#ifndef __mrview_scaling_h__
#define __mrview_scaling_h__

#define CONTRAST_SENS -0.001
#define BRIGHTNESS_SENS 0.1

#include "mrtrix.h"

namespace MR {
  namespace Viewer {

    class Scaling {
      public:
        Scaling () : multiplier (NAN), offset (NAN), min (NAN), max (NAN) { }
        Scaling (const Scaling& S) : multiplier (S.multiplier), offset (S.offset), min (NAN), max (NAN) { }

        float         multiplier, offset;

        void          operator= (const Scaling& S) { multiplier = S.multiplier; offset = S.offset; }

        operator bool () const           { return (!isnan (multiplier) && !isnan (offset)); }
        bool          operator! () const { return (isnan (multiplier) || isnan (offset)); }

        float         operator () (float val) const   { return (offset + multiplier * val); }

        void          adjust (float brightness, float contrast);
        void          reset () { multiplier = offset = NAN; }

        void          rescale_start () const         { min = INFINITY; max = -INFINITY; }
        void          rescale_add (float val) const  { if (min > val) min = val; if (max < val) max = val; }
        void          rescale_add (float val[3]) const { rescale_add (val[0]); rescale_add (val[1]); rescale_add (val[2]); }
        void          rescale_end ()
        { 
          if (max > min) { multiplier = 255.0/(max-min); offset = -multiplier*min; }
          else { multiplier = offset = NAN; }
        }

        bool          operator== (const Scaling& s) const         { return (multiplier == s.multiplier && offset == s.offset); }
        bool          operator!= (const Scaling& s) const         { return (multiplier != s.multiplier || offset != s.offset); }


      protected:
        mutable float   min, max;

        friend std::ostream& operator<< (std::ostream& stream, const Scaling& S);
    };










    inline void Scaling::adjust (float brightness, float contrast)
    { 
      float old = multiplier;
      multiplier *= (1.0 + CONTRAST_SENS*contrast);
      offset = 127.5 - multiplier * (127.5-offset)/old + BRIGHTNESS_SENS*brightness;
    }



    inline std::ostream& operator<< (std::ostream& stream, const Scaling& S)
    {
      stream << "Y = " << S.offset << " + " << S.multiplier << " * X, min/max = [ " << S.min << " " << S.max << " ]";
      return (stream);
    }


  }
}

#endif


