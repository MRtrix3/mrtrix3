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
#include "image.h"
#include "image.h"
#include "interp/linear.h"
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


      class Mask : public Image<bool> {
        public:
          typedef Eigen::Transform<float, 3, Eigen::AffineCompact> transform_type;
          Mask (const Mask&) = default;
          Mask (const std::string& name) :
              Image<bool> (__get_mask (name)),
              scanner2voxel (new transform_type (Transform(header()).scanner2voxel.cast<float>())),
              voxel2scanner (new transform_type (Transform(header()).voxel2scanner.cast<float>())) { }

          std::shared_ptr<transform_type> scanner2voxel, voxel2scanner; // Ptr to prevent unnecessary copy-construction

        private:
          static Image<bool> __get_mask (const std::string& name);
      };




      class ROI {
        public:
          ROI (const Eigen::Vector3f& sphere_pos, float sphere_radius) :
            pos (sphere_pos), radius (sphere_radius), radius2 (Math::pow2 (radius)) { }

          ROI (const std::string& spec) :
            radius (NaN), radius2 (NaN)
          {
            try {
              auto F = parse_floats (spec);
              if (F.size() != 4) 
                throw 1;
              pos[0] = F[0];
              pos[1] = F[1];
              pos[2] = F[2];
              radius = F[3];
              radius2 = Math::pow2 (radius);
            }
            catch (...) { 
              DEBUG ("could not parse spherical ROI specification \"" + spec + "\" - assuming mask image");
              mask.reset (new Mask (spec));
            }
          }

          std::string shape () const { return (mask ? "image" : "sphere"); }

          std::string parameters () const {
            return mask ? mask->name() : str(pos[0]) + "," + str(pos[1]) + "," + str(pos[2]) + "," + str(radius);
          }

          bool contains (const Eigen::Vector3f& p) const
          {

            if (mask) {
              Eigen::Vector3f v = *(mask->scanner2voxel) * p;
              Mask temp (*mask); // Required for thread-safety
              temp.index(0) = std::round (v[0]);
              temp.index(1) = std::round (v[1]);
              temp.index(2) = std::round (v[2]);
              if (is_out_of_bounds (temp))
                return false;
              return temp.value();
            }

            return (pos-p).squaredNorm() <= radius2;

          }

          friend inline std::ostream& operator<< (std::ostream& stream, const ROI& roi)
          {
            stream << roi.shape() << " (" << roi.parameters() << ")";
            return stream;
          }



        private:
          Eigen::Vector3f pos;
          float radius, radius2;
          std::shared_ptr<Mask> mask;

      };





      class ROISet {
        public:
          ROISet () { }

          void clear () { R.clear(); }
          size_t size () const { return (R.size()); }
          const ROI& operator[] (size_t i) const { return (R[i]); }
          void add (const ROI& roi) { R.push_back (roi); }

          bool contains (const Eigen::Vector3f& p) const {
            for (size_t n = 0; n < R.size(); ++n)
              if (R[n].contains (p)) return (true);
            return false;
          }

          void contains (const Eigen::Vector3f& p, std::vector<bool>& retval) const {
            for (size_t n = 0; n < R.size(); ++n)
              if (R[n].contains (p)) retval[n] = true;
          }

          friend inline std::ostream& operator<< (std::ostream& stream, const ROISet& R) {
            if (R.R.empty()) return (stream);
            std::vector<ROI>::const_iterator i = R.R.begin();
            stream << *i;
            ++i;
            for (; i != R.R.end(); ++i) stream << ", " << *i;
            return stream; 
          }

        private:
          std::vector<ROI> R;
      };



    }
  }
}

#endif


