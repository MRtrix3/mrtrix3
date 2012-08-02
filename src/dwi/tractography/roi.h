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

#ifndef __dwi_tractography_roi_h__
#define __dwi_tractography_roi_h__

#include "point.h"
#include "ptr.h"
#include "image/voxel.h"
#include "image/interp/linear.h"
#include "image/buffer_scratch.h"
#include "image/copy.h"
#include "image/buffer.h"
#include "image/nav.h"
#include "math/rng.h"


namespace MR {
  namespace DWI {
    namespace Tractography {

      class ROI {
        public:
          ROI (const Point<>& sphere_pos, float sphere_radius) : 
            pos (sphere_pos), rad (sphere_radius), rad2 (Math::pow2(rad)), vol (4.0*M_PI*Math::pow3(rad)/3.0) { }

          ROI (const std::string& spec) :
            rad (NAN), rad2 (NAN), vol (0.0)
          {
            try {
              std::vector<float> F (parse_floats (spec));
              if (F.size() != 4) throw 1;
              pos.set (F[0], F[1], F[2]);
              rad = F[3];
              rad2 = Math::pow2(rad);
              vol = 4.0*M_PI*Math::pow3(rad)/3.0;
            }
            catch (...) { 
              INFO ("could not parse spherical ROI specification \"" + spec + "\" - assuming mask image");
              Image::Header H (spec);
              if (H.datatype() == DataType::Bit)
                get_mask (spec);
              else
                get_image (spec);
            }
          }

          std::string shape () const { return (mask ? "mask" : (image ? "image" : "sphere")); }

          std::string parameters () const {
            return (mask ? mask->name() : (image ? image->name() : str(pos[0]) + "," + str(pos[1]) + "," + str(pos[2]) + "," + str(rad)));
          }

          float volume () const { return (vol); }

          bool contains (const Point<>& p) const
          {

            if (mask) {
              Mask::voxel_type voxel (*mask);
              Point<> v = mask->interp.scanner2voxel (p);
              voxel[0] = Math::round (v[0]);
              voxel[1] = Math::round (v[1]);
              voxel[2] = Math::round (v[2]);
              if (!Image::Nav::within_bounds (voxel))
                return false;
              return (voxel.value());
            }

            if (image)
              throw Exception ("cannot use non-binary image as a binary roi");

            return ((pos-p).norm2() <= rad2);

          }

          Point<> sample (Math::RNG& rng) const 
          {

            Point<> p;

            if (mask) {
              Mask::voxel_type seed (*mask);
              do {
                seed[0] = rng.uniform_int (mask->dim(0));
                seed[1] = rng.uniform_int (mask->dim(1));
                seed[2] = rng.uniform_int (mask->dim(2));
              } while (!seed.value());
              p.set (seed[0]+rng.uniform()-0.5, seed[1]+rng.uniform()-0.5, seed[2]+rng.uniform()-0.5);
              return (mask->interp.voxel2scanner (p));
            }

            if (image) {
              SeedImage::voxel_type seed (*image);
              float sampler = 0.0;
              do {
                seed[0] = rng.uniform_int (image->dim(0));
                seed[1] = rng.uniform_int (image->dim(1));
                seed[2] = rng.uniform_int (image->dim(2));
                sampler = rng.uniform() * image->max_value;
              } while (seed.value() < MAX (sampler, std::numeric_limits<float>::epsilon()));
              p.set (seed[0]+rng.uniform()-0.5, seed[1]+rng.uniform()-0.5, seed[2]+rng.uniform()-0.5);
              return (image->interp.voxel2scanner (p));
            }

            do {
              p.set (2.0*rng.uniform()-1.0, 2.0*rng.uniform()-1.0, 2.0*rng.uniform()-1.0);
            } while (p.norm2() > 1.0);
            return (pos + rad*p);

          }


          friend inline std::ostream& operator<< (std::ostream& stream, const ROI& roi) 
          {
            stream << roi.shape() << " (" << roi.parameters() << ")";
            return (stream);
          }



        private:

          class Mask : public Image::BufferScratch<bool> {
            public:
              template <class InputVoxelType> 
                Mask (InputVoxelType& D, const Image::Info& info, const std::string& description) :
                  Image::BufferScratch<bool> (info, description),
                  interp (*this)
                {
                  Image::BufferScratch<bool>::voxel_type this_vox (*this);
                  Image::copy (D, this_vox);
                }
              Image::Interp::Base< Image::BufferScratch<bool> > interp;
          };

          class SeedImage : public Image::BufferScratch<float> {
            public:
              template <class InputVoxelType> 
                SeedImage (InputVoxelType& D, const Image::Info& info, const std::string& description, const float max) :
                  Image::BufferScratch<float> (info, description),
                  interp (*this),
                  max_value (max)
                {
                  Image::BufferScratch<float>::voxel_type this_vox (*this);
                  Image::copy (D, this_vox);
                }
              Image::Interp::Base< Image::BufferScratch<float> > interp;
              float max_value;
          };

          Point<>  pos;
          float  rad, rad2, vol;
          RefPtr<Mask> mask;
          RefPtr<SeedImage> image;

          void get_mask  (const std::string& name);
          void get_image (const std::string& name);

      };





      class ROISet {
        public:
          ROISet () : total_volume (0.0) { }

          void clear () { R.clear(); }
          size_t size () const { return (R.size()); }
          const ROI& operator[] (size_t i) const { return (R[i]); }
          void add (const ROI& roi) { R.push_back (roi); total_volume += roi.volume(); }

          float volume () const { return (total_volume); }

          bool contains (const Point<>& p) {
            for (size_t n = 0; n < R.size(); ++n)
              if (R[n].contains (p)) return (true);
            return (false);
          }

          void contains (const Point<>& p, std::vector<bool>& retval) {
            for (size_t n = 0; n < R.size(); ++n)
              if (R[n].contains (p)) retval[n] = true;
          }

          Point<> sample (Math::RNG& rng) {
            float seed_selection = 0.0;
            float seed_selector = total_volume * rng.uniform();
            for (std::vector<ROI>::const_iterator i = R.begin(); i != R.end(); ++i) { 
              seed_selection += i->volume(); 
              if (seed_selector < seed_selection) return (i->sample (rng));
            }
            return (sample (rng));
          }

          friend inline std::ostream& operator<< (std::ostream& stream, const ROISet& R) {
            std::vector<ROI>::const_iterator i = R.R.begin();
            stream << *i;
            ++i;
            for (; i != R.R.end(); ++i) stream << ", " << *i;
            return (stream); 
          }

        private:
          std::vector<ROI> R;
          float total_volume;
      };



    }
  }
}

#endif


