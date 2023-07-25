/* Copyright (c) 2008-2017 the MRtrix3 contributors.
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

#ifndef __fixel_correspondence_fixel_h__
#define __fixel_correspondence_fixel_h__


#include "image.h"
#include "image_helpers.h"

#include "fixel/correspondence/correspondence.h"


namespace MR {
  namespace Fixel {
    namespace Correspondence {



      // Information to be stored for each fixel that will be useful during correspondence
      class Fixel
      {
        public:
          Fixel (const Helper::ConstRow<Image<float>>& direction, const float density) :
              _dir ( dir_t{ direction[0], direction[1], direction[2] }.normalized() ),
              _density (density) { }
          Fixel (const dir_t& direction, const float density) :
              _dir (direction),
              _density (density) { }
          const dir_t& dir() const { return _dir; }
          float density() const { return _density; }
          float dot (const Fixel& i) const { return dir().dot (i.dir()); }
          float dot (const dir_t& d) const { return dir().dot (d); }
          float absdot (const Fixel& i) const { return std::abs (dir().dot (i.dir())); }
          float absdot (const dir_t& d) const { return std::abs (dir().dot (d)); }
        protected:
          const dir_t _dir;
          const float _density;
      };



    }
  }
}

#endif
