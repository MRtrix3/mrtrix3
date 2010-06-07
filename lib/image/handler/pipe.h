/*
   Copyright 2009 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 22/08/09.

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

#ifndef __image_handler_pipe_h__
#define __image_handler_pipe_h__

#include "image/handler/base.h"
#include "file/mmap.h"

namespace MR {
  namespace Image {

    namespace Handler {

      class Pipe : public Base {
        public:
          Pipe (Header& header, bool image_is_new) : Base (header, image_is_new) { }
          virtual ~Pipe ();
          virtual void execute ();

        protected:
          Ptr<File::MMap> file;
      };

    }
  }
}

#endif


