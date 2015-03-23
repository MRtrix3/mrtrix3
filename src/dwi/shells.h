/*
    Copyright 2013 Brain Research Institute, Melbourne, Australia

    Written by B Jeurissen, 12/08/13.

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

#ifndef __dwi_shells_h__
#define __dwi_shells_h__


#include <fstream>
#include <limits>
#include <vector>

#include "app.h"
#include "bitset.h"

#include "math/matrix.h"
#include "math/vector.h"

#include "file/config.h"


// Don't expect these values to change depending on the particular command that is initialising the Shells class;
//   method should be robust to all incoming data

// Maximum absolute difference in b-value for two volumes to be considered to be in the same shell
#define DWI_SHELLS_EPSILON 100
// Minimum number of volumes within DWI_SHELL_EPSILON necessary to continue expansion of the cluster selection
#define DWI_SHELLS_MIN_LINKAGE 3
// Default number of volumes necessary for a shell to be retained
//   (note: only applies if function reject_small_shells() is called explicitly)
#define DWI_SHELLS_MIN_DIRECTIONS 6



//CONF option: BZeroThreshold
//CONF default: 10.0
//CONF specifies the b-value threshold for determining those image
//CONF volumes that correspond to b=0



namespace MR
{

  namespace App { class OptionGroup; }

  namespace DWI
  {


    extern const App::OptionGroup ShellOption;

    inline float bzero_threshold () {
      static const float value = File::Config::get_float ("BZeroThreshold", 10.0);
      return value;
    }



    class Shell
    {

      public:

        Shell() : mean (0.0), stdev (0.0), min (0.0), max (0.0) { }

        Shell (const Math::Matrix<float>& grad, const std::vector<size_t>& indices);

        Shell& operator= (const Shell& rhs)
        {
          volumes = rhs.volumes;
          mean = rhs.mean;
          stdev = rhs.stdev;
          min = rhs.min;
          max = rhs.max;
          return *this;
        }


        const std::vector<size_t>& get_volumes() const { return volumes; }
        size_t count() const { return volumes.size(); }

        float get_mean()  const { return mean; }
        float get_stdev() const { return stdev; }
        float get_min()   const { return min; }
        float get_max()   const { return max; }

        bool is_bzero()   const { return (mean < bzero_threshold()); }


        bool operator< (const Shell& rhs) const { return (mean < rhs.mean); }

        friend std::ostream& operator<< (std::ostream& stream, const Shell& S)
        {
          stream << "Shell: " << S.volumes.size() << " volumes, b-value "
              << S.mean << " +- " << S.stdev << " (range [" << S.min << " - " << S.max << "])";
          return stream;
        }


      protected:
        std::vector<size_t> volumes;
        float mean, stdev, min, max;

    };





    class Shells
    {

      public:
        Shells (const Math::Matrix<float>& grad) { initialise (grad); }
        Shells (const Math::Matrix<double>& grad) { Math::Matrix<float> gradF (grad); initialise (gradF); }


        const Shell& operator[] (const size_t i) const { return shells[i]; }
        const Shell& smallest() const { return shells.front(); }
        const Shell& largest()  const { return shells.back(); }
        size_t       count()    const { return shells.size(); }
		size_t       volumecount()    const { 
		  size_t count = 0;
		  for (std::vector<Shell>::const_iterator it = shells.begin(); it != shells.end(); ++it)
		    count += it->count();
	      return count;
		}

        std::vector<size_t> get_counts() const { 
          std::vector<size_t> c (count()); 
          for (size_t n = 0; n < count(); ++n)
            c[n] = shells[n].count();
          return c;
        }

        std::vector<size_t> get_bvalues() const { 
          std::vector<size_t> b (count()); 
          for (size_t n = 0; n < count(); ++n)
            b[n] = shells[n].get_mean();
          return b;
        }

        void select_shells (const bool keep_bzero = false, const bool force_single_shell = true);

        void reject_small_shells (const size_t min_volumes = DWI_SHELLS_MIN_DIRECTIONS);

        bool is_single_shell() const {
          return ((shells.size() == 1) || ((shells.size() == 2 && smallest().is_bzero())));
        }


        friend std::ostream& operator<< (std::ostream& stream, const Shells& S)
        {
          stream << "Total of " << S.count() << " DWI shells:" << std::endl;
          for (std::vector<Shell>::const_iterator it = S.shells.begin(); it != S.shells.end(); ++it)
            stream << *it << std::endl;
          return stream;
        }


      protected:
        std::vector<Shell> shells;


      private:

        typedef Math::Vector<float>::View BValueList;

        void initialise (const Math::Matrix<float>&);

        // Functions for current b-value clustering implementation
        size_t clusterBvalues (const BValueList&, std::vector<size_t>&) const;
        void regionQuery (const BValueList&, const float, std::vector<size_t>&) const;


    };




  }
}

#endif

