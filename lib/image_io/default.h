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


#ifndef __image_handler_default_h__
#define __image_handler_default_h__

#include "types.h"
#include "image_io/base.h"
#include "file/mmap.h"

namespace MR
{
  namespace ImageIO
  {

    class Default : public Base
    {
      public:
        Default (const Header& header) : 
          Base (header),
          bytes_per_segment (0) { }
        Default (Default&&) noexcept = default;
        Default& operator=(Default&&) = default;

      protected:
        std::vector<std::shared_ptr<File::MMap> > mmaps;
        int64_t bytes_per_segment;

        virtual void load (const Header&, size_t);
        virtual void unload (const Header&);

        void map_files (const Header&);
        void copy_to_mem (const Header&);

    };

  }
}

#endif


