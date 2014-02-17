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

#include "app.h"
#include "point.h"
#include "ptr.h"

#include "image/voxel.h"
#include "image/interp/linear.h"
#include "image/buffer_scratch.h"
#include "image/copy.h"
#include "image/buffer.h"
#include "image/nav.h"

#include "math/rng.h"


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      class Properties;


      extern const App::OptionGroup ROIOption;
      void load_rois (Properties& properties);


      class Mask : public Image::BufferScratch<bool> {
        public:
          template <class InputVoxelType>
          Mask (InputVoxelType& D, const Image::Info& info, const std::string& description) :
              Image::BufferScratch<bool> (info, description),
              transform (this->info())
          {
            Image::BufferScratch<bool>::voxel_type this_vox (*this);
            Image::copy (D, this_vox);
          }
          Image::Transform transform;
      };

      Mask* get_mask (const std::string& name);



      class ROI {
        public:
          ROI (const Point<>& sphere_pos, float sphere_radius) :
            pos (sphere_pos), radius (sphere_radius), radius2 (Math::pow2 (radius)) { }

          ROI (const std::string& spec) :
            radius (NAN), radius2 (NAN)
          {
            try {
              std::vector<float> F (parse_floats (spec));
              if (F.size() != 4) throw 1;
              pos.set (F[0], F[1], F[2]);
              radius = F[3];
              radius2 = Math::pow2 (radius);
            }
            catch (...) { 
              DEBUG ("could not parse spherical ROI specification \"" + spec + "\" - assuming mask image");
              mask = get_mask (spec);
            }
          }

          std::string shape () const { return (mask ? "image" : "sphere"); }

          std::string parameters () const {
            return (mask ? mask->name() : str(pos[0]) + "," + str(pos[1]) + "," + str(pos[2]) + "," + str(radius));
          }

          bool contains (const Point<>& p) const
          {

            if (mask) {
              Mask::voxel_type voxel (*mask);
              Point<> v = mask->transform.scanner2voxel (p);
              voxel[0] = Math::round (v[0]);
              voxel[1] = Math::round (v[1]);
              voxel[2] = Math::round (v[2]);
              if (!Image::Nav::within_bounds (voxel))
                return false;
              return (voxel.value());
            }

            return ((pos-p).norm2() <= radius2);

          }

          friend inline std::ostream& operator<< (std::ostream& stream, const ROI& roi)
          {
            stream << roi.shape() << " (" << roi.parameters() << ")";
            return (stream);
          }



        private:
          Point<> pos;
          float radius, radius2;
          RefPtr<Mask> mask;

      };





      class ROISet {
        public:
          ROISet () { }

          void clear () { R.clear(); }
          size_t size () const { return (R.size()); }
          const ROI& operator[] (size_t i) const { return (R[i]); }
          void add (const ROI& roi) { R.push_back (roi); }

          bool contains (const Point<>& p) const {
            for (size_t n = 0; n < R.size(); ++n)
              if (R[n].contains (p)) return (true);
            return (false);
          }

          void contains (const Point<>& p, std::vector<bool>& retval) const {
            for (size_t n = 0; n < R.size(); ++n)
              if (R[n].contains (p)) retval[n] = true;
          }

          friend inline std::ostream& operator<< (std::ostream& stream, const ROISet& R) {
            if (R.R.empty()) return (stream);
            std::vector<ROI>::const_iterator i = R.R.begin();
            stream << *i;
            ++i;
            for (; i != R.R.end(); ++i) stream << ", " << *i;
            return (stream); 
          }

        private:
          std::vector<ROI> R;
      };



    }
  }
}

#endif


