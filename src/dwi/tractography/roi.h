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
#include "dataset/interp.h"
#include "dataset/buffer.h"
#include "math/rng.h"


namespace MR {
  namespace DWI {
    namespace Tractography {

      class ROI 
      {
        public:
          ROI (const Point& sphere_pos, float sphere_radius) : 
            pos (sphere_pos), rad (sphere_radius), rad2 (Math::pow2(rad)), vol (4.0*M_PI*Math::pow3(rad)/3.0) { }

          ROI (const Image::Header& mask_header) : rad (NAN), rad2(NAN), vol (0.0) { get_mask (mask_header); }

          ROI (const std::string& spec) : rad (NAN), rad2 (NAN), vol (0.0) {
            try {
              std::vector<float> F (parse_floats (spec));
              if (F.size() != 4) throw 1;
              pos.set (F[0], F[1], F[2]);
              rad = F[3];
              vol = 4.0*M_PI*Math::pow3(rad)/3.0;
            }
            catch (...) { 
              info ("error parsing spherical ROI specification \"" + spec + "\" - assuming mask image");
              Image::Header mask_header = Image::Header::open (spec);
              get_mask (mask_header);
            }
          }

          std::string shape () const { return (mask ? "image" : "sphere"); }
          std::string parameters () const { return (mask ? mask->name() : str(pos[0]) + "," + str(pos[1]) + "," + str(pos[2]) + "," + str(rad)); }
          float volume () const { return (vol); }

          bool contains (const Point& p) const {
            if (mask) {
              mask->pos(0, Math::round (p[0]));
              mask->pos(1, Math::round (p[1]));
              mask->pos(2, Math::round (p[2]));
              return (mask->value());
            }
            else return ((pos-p).norm2() <= rad2);
          }

          Point sample (Math::RNG& rng) const {
            Point p;
            if (mask) {
              do {
                mask->pos(0, rng.uniform_int (mask->dim(0)));
                mask->pos(1, rng.uniform_int (mask->dim(1)));
                mask->pos(2, rng.uniform_int (mask->dim(2)));
              } while (!mask->value());
              return (Point (mask->pos(0)+rng.uniform()-0.5, mask->pos(1)+rng.uniform()-0.5, mask->pos(2)+rng.uniform()-0.5)); 
            }

            do {
              p.set (2.0*rng.uniform()-1.0, 2.0*rng.uniform()-1.0, 2.0*rng.uniform()-1.0);
            } while (p.norm2() > 1.0);
            return (pos + rad*p);
          }


          friend inline std::ostream& operator<< (std::ostream& stream, const ROI& roi) {
            stream << roi.shape() << " (" << roi.parameters() << ")";
            return (stream);
          }

        private:
          Point  pos;
          float  rad, rad2, vol;
          RefPtr<DataSet::Buffer<bool,3> > mask;

          void get_mask (const Image::Header& mask_header);
      };





      class ROISet {
        public:
          ROISet () : total_volume (0.0) { }

          void clear () { R.clear(); }
          size_t size () const { return (R.size()); }
          const ROI& operator[] (size_t i) const { return (R[i]); }
          void add (const ROI& roi) { R.push_back (roi); total_volume += roi.volume(); }

          float volume () const { return (total_volume); }

          size_t contains (const Point& p) {
            for (size_t n = 0; n < R.size(); ++n)
              if (R[n].contains (p)) return (n);
            return (SIZE_MAX);
          }

          Point sample (Math::RNG& rng, const std::vector<ROI>& rois) {
            float seed_selection = 0.0;
            float seed_selector = total_volume * rng.uniform();
            for (std::vector<ROI>::const_iterator i = R.begin(); i != R.end(); ++i) { 
              seed_selection += i->volume(); 
              if (seed_selector < seed_selection) return (i->sample (rng));
            }
            return (sample (rng, rois));
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


