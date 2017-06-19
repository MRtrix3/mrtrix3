/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
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
        for (size_t i = 0; i != files.size(); ++i) {
          for (size_t j = 0; j != files[i]->size(); ++j) {
            if ((*files[i])[j])
              return false;
          }
        }
        return true;
      }




    }
  }
}

