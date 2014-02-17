/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


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


      + Option ("output_seeds", "output the seed location of all successful streamlines to a file")
        + Argument ("path").type_text();




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
          Default* seed = new Default (opt[i][0], list.get_rng());
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

        opt = get_options ("output_seeds");
        if (opt.size()) properties["seed_output"] = std::string (opt[0][0]);

      }






      }
    }
  }
}


