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


    02-09-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * handle rare cases where gen_seed() would fail

    29-10-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * fix init_direction handling

    18-03-2009 J-Donald Tournier <d.tournier@brain.org.au>
    * fix serious bug that caused the tracking to be incorrect with obliquely aligned data sets

*/

#ifndef __dwi_tractography_tracker_base_h__
#define __dwi_tractography_tracker_base_h__

#include "image/interp.h"
#include "math/math.h"
#include "math/matrix.h"
#include "math/rng.h"
#include "dwi/tractography/properties.h"

namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Tracker {

        class Base {
          public:
            Base (Image::Object& source_image, Properties& properties);
            virtual ~Base ();

            bool          set (const Point& seed, const Point& seed_dir = Point::Invalid) { pos = seed; num_points = 0; return (init_direction (seed_dir)); }
            void          new_seed (const Point& seed_dir = Point::Invalid);
            const Point&  position () const  { return (pos); }
            const Point&  direction () const { return (dir); }

            bool track_excluded () const { return (excluded); }
            bool track_included () const 
            { 
              for (std::vector<Sphere>::const_iterator i = spheres.include.begin(); i != spheres.include.end(); ++i) 
                if (!i->included) return (false);

              for (std::vector<Mask>::const_iterator i = masks.include.begin(); i != masks.include.end(); ++i) 
                if (!i->included) return (false);

              return (true);
            }


            bool next ();

            void set_rng_seed (uint seed) { return (rng.set_seed (seed)); }

            static float curv2angle (float step_size, float curv)     { return (2.0 * asin (step_size / (2.0 * curv))); }

          protected:
            Image::Interp source;
            Properties& props;
            Math::RNG rng;

            virtual bool  init_direction (const Point& seed_dir = Point::Invalid) = 0;
            virtual bool  next_point () = 0;

            float total_seed_volume, step_size, threshold, init_threshold;
            Point pos, dir;
            int num_points, num_max;

            bool excluded;

            class Sphere {
              public:
                Sphere (const Point& position, float radius) : p (position), r (radius), r2 (Math::pow2 (r)), volume (4.0*M_PI*Math::pow3 (r)/3.0), included (false) { }
                Point p;
                float r, r2, volume;
                bool included;

                bool contains (const Point& pt) const { return (dist2 (p, pt) < r2); }
                Point seed (Math::RNG& rng) const {
                  Point s; 
                  do { s.set (2.0*r*(rng.uniform()-0.5), 2.0*r*(rng.uniform()-0.5), 2.0*r*(rng.uniform()-0.5)); } while (s.norm2() > r2);
                  return (p+s);
                }
            };

            class Mask {
              public:
                Mask (Image::Object& image) : i (image), lower (i.dim(0), i.dim(1), i.dim(2)), upper (0.0, 0.0, 0.0), volume (0.0), included (false) { get_bounds(); }

                Image::Interp i;
                Point lower, upper;
                float volume;
                bool included;

                bool contains (const Point& pt) { 
                  Point y (i.R2P (pt));
                  if (y[0] < lower[0] || y[0] >= upper[0] || y[1] < lower[1] || y[1] >= upper[1] || y[2] < lower[2] || y[2] >= upper[2]) return (false);
                  i.P (y);
                  return (i.value() >= 0.5);
                }
                Point seed (Math::RNG& rng) 
                { 
                  Point p;
                  do {
                    p.set (lower[0]+rng.uniform()*(upper[0]-lower[0]), lower[1]+rng.uniform()*(upper[1]-lower[1]), lower[2]+rng.uniform()*(upper[2]-lower[2]));
                    i.P (p);
                  } while (i.value() < 0.5);
                  return (i.P2R (p)); 
                }

              private:
                void get_bounds ()
                {
                  uint count = 0;
                  for (i.set(2,0); i[2] < i.dim(2); i.inc(2)) {
                    for (i.set(1,0); i[1] < i.dim(1); i.inc(1)) {
                      for (i.set(0,0); i[0] < i.dim(0); i.inc(0)) {
                        if (i.Image::Position::value() >= 0.5) { 
                          count++;
                          if (lower[0] > i[0]) lower[0] = i[0];
                          if (lower[1] > i[1]) lower[1] = i[1];
                          if (lower[2] > i[2]) lower[2] = i[2];
                          if (upper[0] <= i[0]) upper[0] = i[0];
                          if (upper[1] <= i[1]) upper[1] = i[1];
                          if (upper[2] <= i[2]) upper[2] = i[2];
                        }
                      }
                    }
                  }
                  lower[0] -= 0.5; lower[1] -= 0.5; lower[2] -= 0.5;
                  upper[0] += 0.5; upper[1] += 0.5; upper[2] += 0.5;
                  volume = count * i.vox(0) * i.vox(1) * i.vox(2);
                }
            };


            class ROISphere {
              public:
                std::vector<Sphere>  seed, include, exclude, mask;
            } spheres;

            class ROIMask {
              public:
                std::vector<Mask>  seed, include, exclude, mask;
            } masks;

            void inc_pos () { pos += step_size * dir; }

            int get_source_data (const Point& p, float* values)
            {
              source.R (p);
              for (source.set(3,0); source[3] < source.dim(3); source.inc(3)) values[source[3]] = source.value();
              return (isnan (values[0]));
            }

            bool not_in_mask (const Point& pt);

            Point gen_seed () 
            {
              float seed_selection = 0.0;
              float seed_selector = total_seed_volume * rng.uniform();
              for (std::vector<Sphere>::iterator i = spheres.seed.begin(); i != spheres.seed.end(); ++i) { 
                seed_selection += i->volume; 
                if (seed_selector < seed_selection) return (i->seed (rng));
              }
              for (std::vector<Mask>::iterator i = masks.seed.begin(); i != masks.seed.end(); ++i) { 
                seed_selection += i->volume; 
                if (seed_selector < seed_selection) return (i->seed (rng));
              }
              return (gen_seed());
            }
        };








        inline bool Base::not_in_mask (const Point& pt)
        {
          for (std::vector<Sphere>::const_iterator i = spheres.mask.begin(); i != spheres.mask.end(); ++i) 
            if (!i->contains (pt)) return (true);

          for (std::vector<Mask>::iterator i = masks.mask.begin(); i != masks.mask.end(); ++i) 
            if (!i->contains (pt)) return (true);

          return (false);
        }


      }
    }
  }
}

#endif
