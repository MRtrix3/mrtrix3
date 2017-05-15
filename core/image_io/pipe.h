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


#ifndef __image_io_pipe_h__
#define __image_io_pipe_h__

#include "memory.h"
#include "image_io/base.h"
#include "file/mmap.h"

namespace MR
{
  namespace ImageIO
  {

    class Pipe : public Base
    { NOMEMALIGN
      public:
        Pipe (Base&& io_handler) : Base (std::move (io_handler)) { }

      protected:
        std::unique_ptr<File::MMap> mmap;

        virtual void load (const Header&, size_t);
        virtual void unload (const Header&);
    };

  }
}

#endif


