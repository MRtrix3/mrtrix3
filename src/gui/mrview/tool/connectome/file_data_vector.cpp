/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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

#include "gui/mrview/tool/connectome/file_data_vector.h"

#include <limits>

#include "file/path.h"
#include "math/math.h"

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {


        FileDataVector::FileDataVector () :
            base_t (),
            min (NaN),
            mean (NaN),
            max (NaN) { }

        FileDataVector::FileDataVector (const FileDataVector& V) :
            base_t (V),
            name (V.name),
            min (V.min),
            mean (V.mean),
            max (V.max) { }

        FileDataVector::FileDataVector (FileDataVector&& V) :
            base_t (std::move (V)),
            name (V.name),
            min (V.min),
            mean (V.mean),
            max (V.max)
        {
          V.name.clear();
          V.min = V.mean = V.max = NaN;
        }

        FileDataVector::FileDataVector (const size_t nelements) :
            base_t (nelements),
            min (NaN),
            mean (NaN),
            max (NaN) { }

        FileDataVector::FileDataVector (const std::string& file) :
            base_t (),
            name (qstr (Path::basename (file))),
            min (NaN),
            mean (NaN),
            max (NaN)
        {
          base_t temp = MR::load_vector<float> (file);
          base_t::operator= (temp);
          calc_stats();
        }




        FileDataVector& FileDataVector::operator= (const FileDataVector& that)
        {
          base_t::operator= (that);
          name = that.name;
          min = that.min;
          mean = that.mean;
          max = that.max;
          return *this;
        }
        FileDataVector& FileDataVector::operator= (FileDataVector&& that)
        {
          base_t::operator= (std::move (that));
          name = that.name;
          min = that.min;
          mean = that.mean;
          max = that.max;
          that.name.clear();
          that.min = that.mean = that.max = NaN;
          return *this;
        }




        FileDataVector& FileDataVector::load (const std::string& filename)
        {
          base_t temp = MR::load_vector<float> (filename);
          base_t::operator= (temp);
          name = qstr (Path::basename (filename));
          calc_stats();
          return *this;
        }

        FileDataVector& FileDataVector::clear ()
        {
          base_t::resize (0);
          name.clear();
          min = mean = max = NaN;
          return *this;
        }

        void FileDataVector::calc_stats()
        {
          min = std::numeric_limits<float>::infinity();
          double sum = 0.0;
          max = -std::numeric_limits<float>::infinity();
          for (size_t i = 0; i != size_t(size()); ++i) {
            min = std::min (min, operator[] (i));
            sum += operator[] (i);
            max = std::max (max, operator[] (i));
          }
          mean = sum / double(size());
        }



      }
    }
  }
}





