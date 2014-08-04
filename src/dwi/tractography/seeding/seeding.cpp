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

#include "dwi/tractography/properties.h"
#include "dwi/tractography/seeding/seeding.h"




namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace Seeding
      {



      using namespace App;

      const OptionGroup SeedOption = OptionGroup ("Tractography seeding options")

      + Option ("seed_sphere", "spherical seed as four comma-separated values (XYZ position and radius)").allow_multiple()
        + Argument ("spec").type_sequence_float()

      + Option ("seed_image", "seed streamlines entirely at random within a mask image "
                              "(this is the same behaviour as the streamline seeding in MRtrix 0.2)").allow_multiple()
        + Argument ("image").type_image_in()

      + Option ("seed_random_per_voxel", "seed a fixed number of streamlines per voxel in a mask image; random placement of seeds in each voxel").allow_multiple()
        + Argument ("image").type_image_in()
        + Argument ("num_per_voxel").type_integer (1, 1, std::numeric_limits<int>::max())

      + Option ("seed_grid_per_voxel", "seed a fixed number of streamlines per voxel in a mask image; place seeds on a 3D mesh grid "
                                       "(grid_size argument is per axis; so a grid_size of 3 results in 27 seeds per voxel)").allow_multiple()
        + Argument ("image").type_image_in()
        + Argument ("grid_size").type_integer (1, 1, std::numeric_limits<int>::max())

      + Option ("seed_rejection", "seed from an image using rejection sampling (higher values = more probable to seed from)").allow_multiple()
        + Argument ("image").type_image_in()

      + Option ("seed_gmwmi", "seed from the grey matter - white matter interface (only valid if using ACT framework)").allow_multiple()
        + Argument ("seed_image").type_image_in()

      + Option ("seed_dynamic", "determine seed points dynamically using the SIFT model (must NOT provide any other seeding mechanism)") // Don't allow multiple
        + Argument ("fod_image").type_image_in()


      + Option ("max_seed_attempts", "set the maximum number of times that the tracking algorithm should "
                                     "attempt to find an appropriate tracking direction from a given seed point")
        + Argument ("count").type_integer (1, 1, 1e6)


      + Option ("output_seeds", "output the seed location of all successful streamlines to a file")
        + Argument ("path").type_file_out();







      void load_tracking_seeds (Properties& properties)
      {

        List& list (properties.seeds);

        App::Options opt = get_options ("seed_sphere");
        for (size_t i = 0; i < opt.size(); ++i) {
          Sphere* seed = new Sphere (opt[i][0], list.get_rng());
          list.add (seed);
        }

        opt = get_options ("seed_image");
        for (size_t i = 0; i < opt.size(); ++i) {
          SeedMask* seed = new SeedMask (opt[i][0], list.get_rng());
          list.add (seed);
        }

        opt = get_options ("seed_random_per_voxel");
        for (size_t i = 0; i < opt.size(); ++i) {
          Random_per_voxel* seed = new Random_per_voxel (opt[i][0], list.get_rng(), opt[i][1]);
          list.add (seed);
        }

        opt = get_options ("seed_grid_per_voxel");
        for (size_t i = 0; i < opt.size(); ++i) {
          Grid_per_voxel* seed = new Grid_per_voxel (opt[i][0], list.get_rng(), opt[i][1]);
          list.add (seed);
        }

        opt = get_options ("seed_rejection");
        for (size_t i = 0; i < opt.size(); ++i) {
          Rejection* seed = new Rejection (opt[i][0], list.get_rng());
          list.add (seed);
        }

        opt = get_options ("seed_gmwmi");
        if (opt.size()) {
          App::Options opt_act = get_options ("act");
          if (!opt_act.size())
            throw Exception ("Cannot perform GM-WM Interface seeding without ACT segmented tissue image");
          for (size_t i = 0; i < opt.size(); ++i) {
            GMWMI* seed = new GMWMI (opt[i][0], list.get_rng(), str(opt_act[0][0]));
            list.add (seed);
          }
        }

        // Can't instantiate the dynamic seeder here; internal FMLS segmenter has to use the same Directions::Set as TrackMapperDixel
        opt = get_options ("seed_dynamic");
        if (opt.size()) {
          if (list.num_seeds())
            throw Exception ("If performing dynamic streamline seeding, cannot specify any other type of seed!");
          properties["seed_dynamic"] = str(opt[0][0]);
        } else if (!list.num_seeds()) {
          throw Exception ("Must provide at least one source of streamline seeds!");
        }

        opt = get_options ("max_seed_attempts");
        if (opt.size()) properties["max_seed_attempts"] = std::string (opt[0][0]);

        opt = get_options ("output_seeds");
        if (opt.size()) properties["seed_output"] = std::string (opt[0][0]);

      }






      }
    }
  }
}


