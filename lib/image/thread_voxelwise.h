/*
   Copyright 2008 Brain Research Institute, Melbourne, Australia

   Written by J-Donald Tournier, 27/06/08.

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

#ifndef __mrtrix_image_threadvoxelwise_h__
#define __mrtrix_image_threadvoxelwise_h__

#include "thread.h"
#include "ptr.h"
#include "progressbar.h"
#include "image/voxel.h"

namespace MR {
  namespace Image {

    class ThreadVoxelWise : public Thread
    {
      public:
        ThreadVoxelWise (Object& source_object, RefPtr<Voxel> mask_voxel) : source (source_object), mask (mask_voxel) { }
        virtual ~ThreadVoxelWise () { }

        void run (const std::string& progressbar_message)  
        { 
          reset();
          source.map();
          if (mask) mask->image.map();
          ProgressBar::init (source.dim(0)*source.dim(1)*source.dim(2), progressbar_message);
          Thread::run();
          ProgressBar::done();
        }   


        bool get_next (Image::Voxel& vox)
        {
          Mutex::Lock lock (mutex);
          if (done) return (true);
          do {
            bool ok = true;
            if (mask) if (mask->value() < 0.5) ok = false; 
            if (ok) {
              vox[0] = current_voxel[0];
              vox[1] = current_voxel[1];
              vox[2] = current_voxel[2];
            }

            if (increment()) return (true);
            if (ok) return (false);

          } while (true);

          return (true);
        }

      protected:
        Object& source;
        RefPtr<Voxel> mask;

        void    reset () {
          current_voxel[0] = current_voxel[1] = current_voxel[2] = 0;
          done = false;
          if (mask) mask->reset();
        }

      private:
        Mutex   mutex;
        int     current_voxel[3];
        bool    done;

        bool    increment () {
          current_voxel[0]++; 
          if (mask) (*mask)[0]++;

          if (current_voxel[0] >= source.dim(0)) {
            current_voxel[0] = 0;
            current_voxel[1]++; 
            if (mask) { (*mask)[0] = 0; (*mask)[1]++; }

            if (current_voxel[1] >= source.dim(1)) {
              current_voxel[1] = 0; 
              current_voxel[2]++; 
              if (mask) { (*mask)[1] = 0; (*mask)[2]++; }

              if (current_voxel[2] >= source.dim(2)) {
                done = true;
                return (true);
              }
            }
          }

          ProgressBar::inc();
          return (false);
        }


    };
  }
}

#endif

