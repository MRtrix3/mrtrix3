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

      const OptionGroup SeedMechanismOption = OptionGroup ("Tractography seeding mechanisms; at least one must be provided")

      + Option ("seed_image", "seed streamlines entirely at random within a mask image ").allow_multiple()
        + Argument ("image").type_image_in()

      + Option ("seed_sphere", "spherical seed as four comma-separated values (XYZ position and radius)").allow_multiple()
        + Argument ("spec").type_sequence_float()

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
        + Argument ("fod_image").type_image_in();




      const OptionGroup SeedParameterOption = OptionGroup ("Tractography seeding options and parameters")

      + Option ("seeds", 
          "set the number of seeds that tckgen will attempt to track from. If "
          "this option is NOT provided, the default number of seeds is set to "
          "100× the number of selected streamlines. "
          "If -select is NOT also specified, tckgen will continue tracking until "
          "this number of seeds has been attempted. However, if -select is also "
          "specified, tckgen will stop when the number of seeds attempted "
          "reaches the number specified here, OR when the number of streamlines "
          "selected reaches the number requested with the -select option. This "
          "can be used to prevent the program from running indefinitely when no "
          "or very few streamlines can be found that match the selection "
          "criteria. Setting this to zero will cause tckgen to keep attempting "
          "seeds until the number specified by -select has been reached.") +
      Argument ("number").type_integer (0)

      + Option ("max_attempts_per_seed", 
          "set the maximum number of times that the tracking algorithm should "
          "attempt to find an appropriate tracking direction from a given seed point. "
          "This should be set high enough to ensure that an actual plausible seed point is "
          "not discarded prematurely as being unable to initiate tracking from. "
          "Higher settings may affect performance if many seeds are genuinely impossible "
          "to track from, as many attempts will still be made in vain for such seeds. (default: 1000)")
      + Argument ("number").type_integer (1)

      + Option ("seed_cutoff", "set the minimum FA or FOD amplitude for seeding tracks "
          "(default is the same as the normal -cutoff).")
        + Argument ("value").type_float (0.0)

      + Option ("seed_unidirectional","track from the seed point in one direction only (default is to "
                                      "track in both directions).")

      + Option ("seed_direction","specify a seeding direction for the tracking (this should be "
                                 "supplied as a vector of 3 comma-separated values.")
        + Argument ("dir").type_sequence_float()

      + Option ("output_seeds", "output the seed location of all successful streamlines to a file")
        + Argument ("path").type_file_out();







      void load_seed_mechanisms (Properties& properties)
      {
        List& list (properties.seeds);

        auto opt = get_options ("seed_image");
        for (size_t i = 0; i < opt.size(); ++i) {
          SeedMask* seed = new SeedMask (opt[i][0]);
          list.add (seed);
        }

        opt = get_options ("seed_sphere");
        for (size_t i = 0; i < opt.size(); ++i) {
          Sphere* seed = new Sphere (opt[i][0]);
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
      }



      void load_seed_parameters (Properties& properties)
      {
        auto opt = get_options ("seeds");
        if (opt.size()) properties["max_num_seeds"] = str<unsigned int> (opt[0][0]);

        opt = get_options ("max_attempts_per_seed");
        if (opt.size()) properties["max_seed_attempts"] = str<unsigned int> (opt[0][0]);

        opt = get_options ("seed_cutoff");
        if (opt.size()) properties["init_threshold"] = std::string (opt[0][0]);

        opt = get_options ("seed_unidirectional");
        if (opt.size()) properties["unidirectional"] = "1";

        opt = get_options ("seed_direction");
        if (opt.size()) properties["init_direction"] = std::string (opt[0][0]);

        opt = get_options ("output_seeds");
        if (opt.size()) properties["seed_output"] = std::string (opt[0][0]);
      }



      }
    }
  }
}


