/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
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
        + Argument ("num_per_voxel").type_integer (1)

      + Option ("seed_grid_per_voxel", "seed a fixed number of streamlines per voxel in a mask image; place seeds on a 3D mesh grid "
                                       "(grid_size argument is per axis; so a grid_size of 3 results in 27 seeds per voxel)").allow_multiple()
        + Argument ("image").type_image_in()
        + Argument ("grid_size").type_integer (1)

      + Option ("seed_rejection", "seed from an image using rejection sampling (higher values = more probable to seed from)").allow_multiple()
        + Argument ("image").type_image_in()

      + Option ("seed_gmwmi", "seed from the grey matter - white matter interface (only valid if using ACT framework). "
                              "Input image should be a 3D seeding volume; seeds drawn within this image will be optimised to the "
                              "interface using the 5TT image provided using the -act option.").allow_multiple()
        + Argument ("image").type_image_in()

      + Option ("seed_dynamic", "determine seed points dynamically using the SIFT model (must not provide any other seeding mechanism). "
                                "Note that while this seeding mechanism improves the distribution of reconstructed streamlines density, "
                                "it should NOT be used as a substitute for the SIFT method itself.") // Don't allow multiple
        + Argument ("fod_image").type_image_in()


      + Option ("max_seeds", "set the maximum number of tracks to generate. The program will "
                             "not generate more tracks than this number, even if the desired "
                             "number of tracks hasn't yet been reached (default is 100 x number); "
                             "set to 0 to ignore limit.")
          + Argument ("number").type_integer (0)


      + Option ("num_seeds", "set the number of seeds that tckgen will attempt to track from. "
                             "This overrides both -select and -max_seeds. Use this option if you "
                             "genuinely need a constant number of seeds rather than selected streamlines. "
                             "However, note that in most cases, the -seed_random_per_voxel or "
                             "-seed_grid_per_voxel options are likely to be more appropriate.")
          + Argument ("number").type_integer (0)


      + Option ("max_seed_attempts", "set the maximum number of times that the tracking algorithm should "
                                     "attempt to find an appropriate tracking direction from a given seed point")
        + Argument ("number").type_integer (1)


      + Option ("output_seeds", "output the seed location of all successful streamlines to a file")
        + Argument ("path").type_file_out();







      void load_tracking_seeds (Properties& properties)
      {

        List& list (properties.seeds);

        auto opt = get_options ("seed_sphere");
        for (size_t i = 0; i < opt.size(); ++i) {
          Sphere* seed = new Sphere (opt[i][0]);
          list.add (seed);
        }

        opt = get_options ("seed_image");
        for (size_t i = 0; i < opt.size(); ++i) {
          SeedMask* seed = new SeedMask (opt[i][0]);
          list.add (seed);
        }

        opt = get_options ("seed_random_per_voxel");
        for (size_t i = 0; i < opt.size(); ++i) {
          Random_per_voxel* seed = new Random_per_voxel (opt[i][0], opt[i][1]);
          list.add (seed);
        }

        opt = get_options ("seed_grid_per_voxel");
        for (size_t i = 0; i < opt.size(); ++i) {
          Grid_per_voxel* seed = new Grid_per_voxel (opt[i][0], opt[i][1]);
          list.add (seed);
        }

        opt = get_options ("seed_rejection");
        for (size_t i = 0; i < opt.size(); ++i) {
          Rejection* seed = new Rejection (opt[i][0]);
          list.add (seed);
        }

        opt = get_options ("seed_gmwmi");
        if (opt.size()) {
          auto opt_act = get_options ("act");
          if (!opt_act.size())
            throw Exception ("Cannot perform GM-WM Interface seeding without ACT segmented tissue image");
          for (size_t i = 0; i < opt.size(); ++i) {
            GMWMI* seed = new GMWMI (opt[i][0], str(opt_act[0][0]));
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

        opt = get_options ("max_seeds");
        if (opt.size()) properties["max_num_attempts"] = str<unsigned int> (opt[0][0]);
        
        opt = get_options ("num_seeds");
        if (opt.size()) properties["exact_num_attempts"] = str<unsigned int> (opt[0][0]);
        //TODO: This property is not being used by any algorithm in practice yet.

        opt = get_options ("max_seed_attempts");
        if (opt.size()) properties["max_seed_attempts"] = str<unsigned int> (opt[0][0]);

        opt = get_options ("output_seeds");
        if (opt.size()) properties["seed_output"] = std::string (opt[0][0]);

      }






      }
    }
  }
}


