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

#include "dwi/tractography/seeding/list.h"
#include "dwi/tractography/rng.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Seeding
      {




        void List::add (Base* const in)
        {
          if (seeders.size() && !(in->is_finite() == is_finite()))
            throw Exception ("Cannot use a combination of seed types where some are number-limited and some are not!");

          if (!App::get_options ("max_seed_attempts").size())
            for (auto& i : seeders)
              if (i->get_max_attempts() != in->get_max_attempts())
                throw Exception ("Cannot use a combination of seed types where the default maximum number "
                    "of sampling attempts per seed is unequal, unless you use the -max_seed_attempts option.");

          seeders.push_back (std::unique_ptr<Base> (in));
          total_volume += in->vol();
          total_count += in->num();
        }



        void List::clear()
        {
          seeders.clear();
          total_volume = 0.0;
          total_count = 0.0;
        }



        bool List::get_seed (Eigen::Vector3f& p, Eigen::Vector3f& d)
        {
          std::uniform_real_distribution<float> uniform;

          if (is_finite()) {

            for (auto& i : seeders)
              if (i->get_seed (p, d))
                return true;

            p = { NaN, NaN, NaN };
            return false;

          } else {

            if (seeders.size() == 1)
              return seeders.front()->get_seed (p, d);

            do {
              float incrementer = 0.0;
              const float sample = uniform (rng) * total_volume;
              for (auto& i : seeders)
                if ((incrementer += i->vol()) > sample)
                  return i->get_seed (p, d);

            } while (1);
            return false;

          }

        }




      }
    }
  }
}


