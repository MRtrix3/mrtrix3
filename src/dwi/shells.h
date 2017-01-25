/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __dwi_shells_h__
#define __dwi_shells_h__


#include <fstream>
#include <limits>
#include <vector>

#include "app.h"
#include "bitset.h"

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
//CONF Specifies the b-value threshold for determining those image
//CONF volumes that correspond to b=0.



namespace MR
{

  namespace App { class OptionGroup; }

  namespace DWI
  {

    extern const App::OptionGroup ShellOption;

    FORCE_INLINE default_type bzero_threshold () {
      static const default_type value = File::Config::get_float ("BZeroThreshold", 10.0);
      return value;
    }



    class Shell
    {

      public:

        Shell() : mean (0.0), stdev (0.0), min (0.0), max (0.0) { }
        Shell (const Eigen::MatrixXd& grad, const std::vector<size_t>& indices);

        const std::vector<size_t>& get_volumes() const { return volumes; }
        size_t count() const { return volumes.size(); }

        default_type get_mean()  const { return mean; }
        default_type get_stdev() const { return stdev; }
        default_type get_min()   const { return min; }
        default_type get_max()   const { return max; }

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
        default_type mean, stdev, min, max;

    };





    class Shells
    {
      public:
        Shells (const Eigen::MatrixXd& grad);

        const Shell& operator[] (const size_t i) const { return shells[i]; }
        const Shell& smallest() const { return shells.front(); }
        const Shell& largest()  const { return shells.back(); }
        size_t       count()    const { return shells.size(); }
        size_t       volumecount()    const { 
          size_t count = 0;
          for (const auto& it : shells)
            count += it.count();
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

        Shells& select_shells (const bool force_singleshell, const bool force_with_bzero, const bool force_without_bzero);

        Shells& reject_small_shells (const size_t min_volumes = DWI_SHELLS_MIN_DIRECTIONS);

        bool is_single_shell() const {
          // only if exactly 1 non-bzero shell
          return ((count() == 1 && !has_bzero()) || (count() == 2 && has_bzero()));
        }

        bool has_bzero() const {
          return smallest().is_bzero();
        }

        friend std::ostream& operator<< (std::ostream& stream, const Shells& S)
        {
          stream << "Total of " << S.count() << " DWI shells:" << std::endl;
          for (const auto& it : S.shells)
            stream << it << std::endl;
          return stream;
        }


      protected:
        std::vector<Shell> shells;


      private:

        typedef decltype(std::declval<const Eigen::MatrixXd>().col(0)) BValueList;

        // Functions for current b-value clustering implementation
        size_t clusterBvalues (const BValueList&, std::vector<size_t>&) const;
        void regionQuery (const BValueList&, const default_type, std::vector<size_t>&) const;


    };




  }
}

#endif

