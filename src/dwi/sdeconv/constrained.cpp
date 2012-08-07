#include "dwi/sdeconv/constrained.h"

namespace MR
{
  namespace DWI
  {

    using namespace App;

    const OptionGroup CSD_options =
      OptionGroup ("Spherical deconvolution options")
      + Option ("lmax",
                "set the maximum harmonic order for the output series. By default, the "
                "program will use the highest possible lmax given the number of "
                "diffusion-weighted images.")
      + Argument ("order").type_integer (2, 8, 30)

      + Option ("mask",
                "only perform computation within the specified binary brain mask image.")
      + Argument ("image").type_image_in()

      + Option ("directions",
                "specify the directions over which to apply the non-negativity constraint "
                "(by default, the built-in 300 direction set is used). These should be "
                "supplied as a text file containing the [ el az ] pairs for the directions.")
      + Argument ("file").type_file()

      + Option ("filter",
                "the linear frequency filtering parameters used for the initial linear "
                "spherical deconvolution step (default = [ 1 1 1 0 0 ]). These should be "
                " supplied as a text file containing the filtering coefficients for each "
                "even harmonic order.")
      + Argument ("spec").type_file()

      + Option ("neg_lambda",
                "the regularisation parameter lambda that controls the strength of the "
                "non-negativity constraint (default = 1.0).")
      + Argument ("value").type_float (0.0, 1.0, 1.0e12)

      + Option ("norm_lambda",
                "the regularisation parameter lambda that controls the strength of the "
                "constraint on the norm of the solution (default = 1.0).")
      + Argument ("value").type_float (0.0, 1.0, 1.0e12)

      + Option ("threshold",
                "the threshold below which the amplitude of the FOD is assumed to be zero, "
                "expressed as an absolute amplitude (default = 0.0).")
      + Argument ("value").type_float (-1.0, 0.0, 10.0)

      + Option ("niter",
                "the maximum number of iterations to perform for each voxel (default = 50).")
      + Argument ("number").type_integer (1, 50, 1000);


  }
}

