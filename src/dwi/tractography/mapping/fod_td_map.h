/*
    Copyright 2011 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2012.

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



#ifndef __dwi_tractography_mapping_fod_td_map_h__
#define __dwi_tractography_mapping_fod_td_map_h__


#include "image/buffer_scratch.h"
#include "image/nav.h"
#include "image/transform.h"
#include "image/voxel.h"
#include "image/interp/linear.h"

#include "dwi/fod_map.h"
#include "dwi/directions/set.h"
#include "dwi/tractography/mapping/voxel.h"
#include "dwi/tractography/ACT/tissues.h"



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Mapping
      {



        // Tempalted Lobe class MUST provide operator+= (const float) for adding streamline density

      template <class Lobe>
      class FOD_TD_map : public FOD_map<Lobe>
      {

        typedef typename FOD_map<Lobe>::MapVoxel MapVoxel;
        typedef typename FOD_map<Lobe>::VoxelAccessor VoxelAccessor;

        public:

        template <typename Info>
        FOD_TD_map (const Info& info, const DWI::Directions::FastLookupSet& directions) :
        FOD_map<Lobe> (info),
        dirs (directions) { }

        virtual ~FOD_TD_map() { }


        bool operator() (const SetDixel& in);


        protected:
        using FOD_map<Lobe>::accessor;
        using FOD_map<Lobe>::lobes;


        const DWI::Directions::FastLookupSet& dirs;

        size_t dix2lobe (const Dixel&);


        FOD_TD_map (const FOD_TD_map& that) : FOD_map<Lobe> (that), dirs (that.dirs) { assert (0); }

      };





      template <class Lobe>
      bool FOD_TD_map<Lobe>::operator() (const SetDixel& in)
      {
          for (SetDixel::const_iterator i = in.begin(); i != in.end(); ++i) {
            const size_t lobe_index = dix2lobe (*i);
            if (lobe_index)
              lobes[lobe_index] += i->get_value();
          }
          return true;
      }


      template <class Lobe>
      size_t FOD_TD_map<Lobe>::dix2lobe (const Dixel& in)
      {
          if (!Image::Nav::within_bounds (accessor, in))
            return 0;
          VoxelAccessor v (accessor);
          Image::Nav::set_pos (v, in);
          if (!v.value())
            return 0;
          const MapVoxel& map_voxel (*v.value());
          if (map_voxel.empty())
            return 0;
          return map_voxel.dir2lobe (in.get_dir());
      }




      // Templated Lobe class MUST also provide void set_weight (const float), for setting weight of processing mask

      template <class Lobe>
      class FOD_TD_diff_map : public FOD_TD_map<Lobe>
      {

        typedef typename FOD_map<Lobe>::MapVoxel MapVoxel;
        typedef typename FOD_map<Lobe>::VoxelAccessor VoxelAccessor;

        public:
        template <class Set>
        FOD_TD_diff_map (Set& dwi, const DWI::Directions::FastLookupSet& dirs) :
        FOD_TD_map<Lobe> (dwi, dirs),
        proc_mask (FOD_map<Lobe>::info(), "FOD map processing mask"),
        FOD_sum (0.0),
        TD_sum (0.0)
        {
          fill_proc_mask (dwi, proc_mask);
        }

        bool operator() (const FOD_lobes& in);
        bool operator() (const SetDixel& in);

        double mu() const { return FOD_sum / TD_sum; }


        protected:
        using FOD_map<Lobe>::accessor;
        using FOD_map<Lobe>::lobes;
        using FOD_TD_map<Lobe>::dirs;

        Image::BufferScratch<float> proc_mask;
        double FOD_sum, TD_sum;


        private:
        template <class Set>
        void fill_proc_mask (Set&, Image::BufferScratch<float>&);


        protected:
        FOD_TD_diff_map (const FOD_TD_diff_map& that) : FOD_TD_map<Lobe> (that), proc_mask (that.proc_mask.info()) { assert (0); }

      };



      template <class Lobe>
      bool FOD_TD_diff_map<Lobe>::operator() (const FOD_lobes& in)
      {
          if (!FOD_map<Lobe>::operator() (in))
            return false;
          Image::BufferScratch<float>::voxel_type mask (proc_mask);
          const float mask_value = Image::Nav::get_value_at_pos (mask, in.vox);
          VoxelAccessor v (accessor);
          Image::Nav::set_pos (v, in.vox);
          if (v.value()) {
            for (typename FOD_map<Lobe>::Iterator i = begin (v); i; ++i) {
              i().set_weight (mask_value); // Note: Lobe class must provide this (even if it does nothing)
              FOD_sum += i().get_FOD() * mask_value;
            }
          }
          return true;
      }

      template <class Lobe>
      bool FOD_TD_diff_map<Lobe>::operator() (const SetDixel& in)
      {
          Image::BufferScratch<float>::voxel_type v (proc_mask);
          float total_contribution = 0.0;
          for (SetDixel::const_iterator i = in.begin(); i != in.end(); ++i) {
            const size_t lobe_index = FOD_TD_map<Lobe>::dix2lobe (*i);
            if (lobe_index) {
              lobes[lobe_index] += i->get_value(); // Note: Lobe class must provide operator+= (const float)
              Image::Nav::set_pos (v, *i);
              total_contribution += v.value() * i->get_value();
            }
          }
          TD_sum += total_contribution;
          return true;
      }




      template <class Lobe>
      template <class Set>
      void FOD_TD_diff_map<Lobe>::fill_proc_mask (Set& in_dwi, Image::BufferScratch<float>& out_mask)
      {

          Image::Buffer<float>::voxel_type dwi (in_dwi);
          Image::Transform transform_dwi (dwi);
          Image::BufferScratch<float>::voxel_type mask (out_mask);

          App::Options opt = App::get_options ("act");
          if (!opt.size()) {

            Image::LoopInOrder loop (dwi, "Creating processing mask from DWI data...", 0, 3);
            dwi[3] = 0;
            for (loop.start (dwi, mask); loop.ok(); loop.next (dwi, mask))
              mask.value() = (dwi.value() && finite (dwi.value())) ? 1.0 : 0.0;

          } else {

            Image::Buffer<float> in_anat (opt[0][0]);
            Image::Buffer<float>::voxel_type anat (in_anat);
            Image::Interp::Linear< Image::Buffer<float>::voxel_type > interp (anat);

            // How densely to sample each voxel, considering the dwi will be a coarser resolution than the anatomical image
            // Express as a step size in voxel space (1.0 = 8 samples, 0.5 = 27 samples, etc.)
            // In case of asymmetrical voxels, take the safest option == denser sampling
            const float voxel_size_ratio = maxvalue (in_dwi.vox(0), in_dwi.vox(1), in_dwi.vox(2)) / minvalue (in_anat.vox(0), in_anat.vox(1), in_anat.vox(2));
            const int   voxel_oversample_ratio = MAX (2, ceil (voxel_size_ratio));
            const float voxel_oversample_step = 1.0 / (voxel_oversample_ratio - 1);

            Image::LoopInOrder loop (dwi, "Creating processing mask from anatomical image...", 0, 3);
            for (loop.start (dwi, mask); loop.ok(); loop.next (dwi, mask)) {
              dwi[3] = 0;
              if (dwi.value()) {

                // Build a list of points in voxel space of the DWI image to be tested
                std::vector< Point<float> > to_test_voxel_space_dwi;
                for (int iz = 0; iz != voxel_oversample_ratio; ++iz) {
                  const float pz = float(dwi[2]) - 0.5 + (iz * voxel_oversample_step);
                  for (int iy = 0; iy != voxel_oversample_ratio; ++iy) {
                    const float py = float(dwi[1]) - 0.5 + (iy * voxel_oversample_step);
                    for (int ix = 0; ix != voxel_oversample_ratio; ++ix) {
                      const float px = float(dwi[0]) - 0.5 + (ix * voxel_oversample_step);
                      to_test_voxel_space_dwi.push_back (Point<float> (px, py, pz));
                    }
                  }
                }

                // Transform these to real-space
                std::vector< Point<float> > to_test_real_space;
                for (std::vector< Point<float> >::const_iterator p = to_test_voxel_space_dwi.begin(); p != to_test_voxel_space_dwi.end(); ++p)
                  to_test_real_space.push_back (transform_dwi.voxel2scanner (*p));

                // Sample from the anatomical segmentation image
                float cgm = 0.0, sgm = 0.0, wm = 0.0, csf = 0.0;
                size_t wm_count = 0;
                for (std::vector< Point<float> >::const_iterator p = to_test_real_space.begin(); p != to_test_real_space.end(); ++p) {
                  if (!interp.scanner (*p)) {
                    Tractography::ACT::Tissues tissues (interp);
                    if (tissues.valid()) {
                      if (tissues.get_wm() >= tissues.get_gm())
                        ++wm_count;
                      cgm += tissues.get_cgm();
                      sgm += tissues.get_sgm();
                      wm  += tissues.get_wm ();
                      csf += tissues.get_csf();
                    }
                  }
                }

                cgm /= float (to_test_real_space.size());
                sgm /= float (to_test_real_space.size());
                wm  /= float (to_test_real_space.size());
                csf /= float (to_test_real_space.size());

                // In my experience, using the square of the WM fraction is better than the WM fraction alone, and this is what was used in the SIFT paper
                //mask.value() = Math::pow2 (wm);

                // Doing a WM > GM fraction rather than the interpolated values - sharper at the interface
                mask.value() = Math::pow2 (float(wm_count) / float(to_test_real_space.size()));

              } else {
                mask.value() = 0.0;
              }
            }

          }

      }





      }
    }
  }
}


#endif
