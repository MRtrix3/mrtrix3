/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */

#ifndef __dwi_tractography_seeding_basic_h__
#define __dwi_tractography_seeding_basic_h__

#include "dwi/tractography/roi.h"
#include "dwi/tractography/seeding/base.h"


// By default, the rejection sampler will perform its sampling based on image intensity values,
//   and then randomly select a position within that voxel
// Use this flag to instead perform rejection sampling on the trilinear-interpolated value
//   at each trial seed point
//#define REJECTION_SAMPLING_USE_INTERPOLATION



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Seeding
      {


        class Sphere : public Base
        {

          public:
            Sphere (const std::string& in) :
              Base (in, "sphere", MAX_TRACKING_SEED_ATTEMPTS_RANDOM) {
                auto F = parse_floats (in);
                if (F.size() != 4)
                  throw Exception ("Could not parse seed \"" + in + "\" as a spherical seed point; needs to be 4 comma-separated values (XYZ position, then radius)");
                pos = { float(F[0]), float(F[1]), float(F[2]) };
                rad = F[3];
                volume = 4.0*Math::pi*Math::pow3(rad)/3.0;
              }

            virtual bool get_seed (Eigen::Vector3f& p) const override;

          private:
            Eigen::Vector3f pos;
            float rad;

        };


        class SeedMask : public Base
        {

          public:
            SeedMask (const std::string& in) :
              Base (in, "random seeding mask", MAX_TRACKING_SEED_ATTEMPTS_RANDOM),
              mask (in) {
                volume = get_count (mask) * mask.spacing(0) * mask.spacing(1) * mask.spacing(2);
              }

            virtual bool get_seed (Eigen::Vector3f& p) const override;

          private:
            Mask mask;

        };



        class Random_per_voxel : public Base
        {

          public:
            Random_per_voxel (const std::string& in, const size_t num_per_voxel) :
              Base (in, "random per voxel", MAX_TRACKING_SEED_ATTEMPTS_FIXED),
              mask (in),
              num (num_per_voxel),
              inc (0),
              expired (false) {
                count = get_count (mask) * num_per_voxel;
                mask.index(0) = 0; mask.index(1) = 0; mask.index(2) = -1;
              }

            virtual bool get_seed (Eigen::Vector3f& p) const override;
            virtual ~Random_per_voxel() { }

          private:
            mutable Mask mask;
            const size_t num;

            mutable uint32_t inc;
            mutable bool expired;
        };



        class Grid_per_voxel : public Base
        {

          public:
            Grid_per_voxel (const std::string& in, const size_t os_factor) :
              Base (in, "grid per voxel", MAX_TRACKING_SEED_ATTEMPTS_FIXED),
              mask (in),
              os (os_factor),
              pos (os, os, os),
              offset (-0.5 + (1.0 / (2*os))),
              step (1.0 / os),
              expired (false) {
                count = get_count (mask) * Math::pow3 (os_factor);
              }

            virtual ~Grid_per_voxel() { }
            virtual bool get_seed (Eigen::Vector3f& p) const override;


          private:
            mutable Mask mask;
            const int os;
            mutable Eigen::Vector3i pos;
            const float offset, step;
            mutable bool expired;

        };



        class Rejection : public Base
        {
          public:
            typedef Eigen::Transform<float, 3, Eigen::AffineCompact> transform_type;
            Rejection (const std::string&);


            virtual bool get_seed (Eigen::Vector3f& p) const override;

          private:
#ifdef REJECTION_SAMPLING_USE_INTERPOLATION
            Interp::Linear<Image<float>> interp;
#else
            Image<float> image;
            transform_type voxel2scanner;
#endif
            float max;

        };






      }
    }
  }
}

#endif

