/*
   Copyright 2011 Brain Research Institute, Melbourne, Australia

   Written by Robert E. Smith, 2012.

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


#include "dwi/tractography/seeding/basic.h"

#include "image/adapter/subset.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Seeding
      {


        bool Sphere::get_seed (Point<float>& p)
        {
          do {
            p.set (2.0*rng.uniform()-1.0, 2.0*rng.uniform()-1.0, 2.0*rng.uniform()-1.0);
          } while (p.norm2() > 1.0);
          p = pos + rad*p;
          return true;
        }





        SeedMask::~SeedMask()
        {
          delete mask;
          mask = NULL;
        }

        bool SeedMask::get_seed (Point<float>& p)
        {
          Mask::voxel_type seed (*mask);
          do {
            seed[0] = rng.uniform_int (mask->dim(0));
            seed[1] = rng.uniform_int (mask->dim(1));
            seed[2] = rng.uniform_int (mask->dim(2));
          } while (!seed.value());
          p.set (seed[0]+rng.uniform()-0.5, seed[1]+rng.uniform()-0.5, seed[2]+rng.uniform()-0.5);
          p = mask->transform.voxel2scanner (p);
          return true;
        }





        Random_per_voxel::~Random_per_voxel()
        {
          delete mask;
          mask = NULL;
        }


        bool Random_per_voxel::get_seed (Point<float>& p)
        {

          if (expired)
            return false;

          Thread::Mutex::Lock lock (mutex);

          if (vox[2] < 0 || ++inc == num) {
            inc = 0;
            ++vox[2];
            Mask::voxel_type v (*mask);
            Image::Nav::set_pos (v, vox);

            while (v[0] != v.dim(0) && !v.value()) {
              if (++v[2] == v.dim(2)) {
                v[2] = 0;
                if (++v[1] == v.dim(1)) {
                  v[1] = 0;
                  ++v[0];
                }
              }
            }

            if (v[0] == v.dim(0)) {
              expired = true;
              return false;
            }
            vox[0] = v[0]; vox[1] = v[1], vox[2] = v[2];
          }

          p.set (vox[0]+rng.uniform()-0.5, vox[1]+rng.uniform()-0.5, vox[2]+rng.uniform()-0.5);
          p = mask->transform.voxel2scanner (p);
          return true;

        }







        Grid_per_voxel::~Grid_per_voxel()
        {
          delete mask;
          mask = NULL;
        }

        bool Grid_per_voxel::get_seed (Point<float>& p)
        {

          if (expired)
            return false;

          Thread::Mutex::Lock lock (mutex);

          if (++pos[2] >= os) {
            pos[2] = 0;
            if (++pos[1] >= os) {
              pos[1] = 0;
              if (++pos[0] >= os) {
                pos[0] = 0;
                ++vox[2];

                Mask::voxel_type v (*mask);
                Image::Nav::set_pos (v, vox);

                while (v[0] != v.dim(0) && !v.value()) {
                  if (++v[2] == v.dim(2)) {
                    v[2] = 0;
                    if (++v[1] == v.dim(1)) {
                      v[1] = 0;
                      ++v[0];
                    }
                  }
                }
                if (v[0] == v.dim(0)) {
                  expired = true;
                  return false;
                }
                vox[0] = v[0]; vox[1] = v[1], vox[2] = v[2];
              }
            }
          }

          p.set (vox[0]+offset+(pos[0]*step), vox[1]+offset+(pos[1]*step), vox[2]+offset+(pos[2]*step));
          p = mask->transform.voxel2scanner (p);
          return true;

        }


        Rejection::Rejection (const std::string& in, const Math::RNG& rng) :
          Base (in, rng, "rejection sampling"),
          max (0.0)
        {

          Image::Buffer<float> data (in);
          Image::Buffer<float>::voxel_type vox (data);
          std::vector<size_t> bottom (vox.ndim(), 0), top (vox.ndim(), 0);
          std::fill_n (bottom.begin(), 3, std::numeric_limits<size_t>::max());

          Image::Loop loop (0,3);
          for (loop.start (vox); loop.ok(); loop.next (vox)) {
            const float value = vox.value();
            if (value) {
              if (value < 0.0)
                throw Exception ("Cannot have negative values in an image used for rejection sampling!");
              max = MAX (max, value);
              volume += value;
              if (size_t(vox[0]) < bottom[0]) bottom[0] = vox[0];
              if (size_t(vox[0]) > top[0])    top[0]    = vox[0];
              if (size_t(vox[1]) < bottom[1]) bottom[1] = vox[1];
              if (size_t(vox[1]) > top[1])    top[1]    = vox[1];
              if (size_t(vox[2]) < bottom[2]) bottom[2] = vox[2];
              if (size_t(vox[2]) > top[2])    top[2]    = vox[2];
            }
          }

          if (!max)
            throw Exception ("Cannot use image " + in + " for rejection sampling - image is empty");

          if (bottom[0]) --bottom[0];
          if (bottom[1]) --bottom[1];
          if (bottom[2]) --bottom[2];

          top[0] = std::min (size_t (data.dim(0)-bottom[0]), top[0]+2-bottom[0]);
          top[1] = std::min (size_t (data.dim(1)-bottom[1]), top[1]+2-bottom[1]);
          top[2] = std::min (size_t (data.dim(2)-bottom[2]), top[2]+2-bottom[2]);

          Image::Info new_info (data);
          for (size_t axis = 0; axis != 3; ++axis) {
            new_info.dim(axis) = top[axis];
            for (size_t i = 0; i < 3; ++i)
              new_info.transform()(i,3) += bottom[axis] * new_info.vox(axis) * new_info.transform()(i,axis);
          }

          Image::Adapter::Subset< Image::Buffer<float>::voxel_type > sub (vox, bottom, top);

          image = new FloatImage (sub, new_info, in);

          volume *= image->dim(0) * image->dim(1) * image->dim(2);

        }


        bool Rejection::get_seed (Point<float>& p)
        {
          FloatImage::voxel_type seed (*image);
          float selector;
          do {
            seed[0] = rng.uniform_int (image->dim(0));
            seed[1] = rng.uniform_int (image->dim(1));
            seed[2] = rng.uniform_int (image->dim(2));
            selector = rng.uniform() * max;
          } while (seed.value() < selector);
          p.set (seed[0]+rng.uniform()-0.5, seed[1]+rng.uniform()-0.5, seed[2]+rng.uniform()-0.5);
          p = image->transform.voxel2scanner (p);
          return true;
        }



      }
    }
  }
}


