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

#include "math/stats/import.h"

namespace MR
{
  namespace Math
  {
    namespace Stats
    {





      vector_type CohortDataImport::operator() (const size_t element) const
      {
        vector_type result (files.size());
        for (size_t i = 0; i != files.size(); ++i)
          result[i] = (*files[i]) [element]; // Get the intensity for just a particular element from this input data file
        return result;
      }




      bool CohortDataImport::allFinite() const
      {
        // TESTME Should be possible to do this faster by populating matrix data
        if (!size())
          return true;
        matrix_type data (size(), files[0]->size());
        for (size_t i = 0; i != size(); ++i)
          (*files[i]) (data.row (i));
        return data.allFinite();
/*
        for (size_t i = 0; i != files.size(); ++i) {
          for (size_t j = 0; j != files[i]->size(); ++j) {
            if ((*files[i])[j])
              return false;
          }
        }
        return true;
*/
      }




    }
  }
}

