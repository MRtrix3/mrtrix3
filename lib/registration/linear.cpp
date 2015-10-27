#include "registration/linear.h"

namespace MR
{
  namespace Registration
  {

    using namespace App;

    const char* initialisation_choices[] = { "mass", "geometric", "none", NULL };

    const OptionGroup rigid_options =
      OptionGroup ("Rigid registration options")

      + Option ("rigid", "the output text file containing the rigid transformation as a 4x4 matrix")
        + Argument ("file").type_file_out ()

      + Option ("rigid_scale", "use a multi-resolution scheme by defining a scale factor for each level "
                               "using comma separated values (Default: 0.5,1)")
        + Argument ("factor").type_sequence_float ()

      + Option ("rigid_niter", "the maximum number of iterations. This can be specified either as a single number "
                               "for all multi-resolution levels, or a single value for each level. (Default: 1000)")
        + Argument ("num").type_sequence_int ()

      + Option ("rigid_smooth_factor", "amount of smoothing before registration (Default: 1.0)")
        + Argument ("num").type_float (0.0, 1.0, std::numeric_limits<float>::infinity())

      + Option ("rigid_cc", "metric: use cross correlation. default: least squares");


    const OptionGroup affine_options =
        OptionGroup ("Affine registration options")

      + Option ("affine", "the output text file containing the affine transformation that aligns "
        "input image 1 to input image 2 as a 4x4 matrix")
        + Argument ("file").type_file_out ()

      + Option ("affine_2tomidway", "the output text file containing the affine transformation that aligns "
        "image2 to image1 in their common midway space as a 4x4 matrix")
        + Argument ("file").type_file_out ()

      + Option ("affine_1tomidway", "the output text file containing the affine transformation that "
        "aligns image1 to image2 in their common midway space as a 4x4 matrix")
        + Argument ("file").type_file_out ()

      + Option ("affine_scale", "use a multi-resolution scheme by defining a scale factor for each level "
                               "using comma separated values (Default: 0.5,1)")
        + Argument ("factor").type_sequence_float ()

      + Option ("affine_niter", "the maximum number of iterations. This can be specified either as a single number "
                                "for all multi-resolution levels, or a single value for each level. (Default: 1000)")
        + Argument ("num").type_sequence_int ()

      + Option ("affine_smooth_factor", "amount of smoothing before registration (Default: 1.0)")
        + Argument ("num").type_float (0.0, 1.0, std::numeric_limits<float>::infinity())

      + Option ("affine_sparsity", "sparsity of gradient descent 0 (batch) to 1.0 (max stochastic) (Default: 0.0)")
        + Argument ("num").type_sequence_float ()

      + Option ("affine_cc", "metric: use cross correlation. default: least squares")

      + Option ("affine_robust", "metric: use robust estimator. default: false");


    const OptionGroup syn_options =
      OptionGroup ("SyN registration options")

      + Option ("warp", "the output non-linear warp defined as a deformation field")
        + Argument ("image").type_file_out ()

      + Option ("syn_scale", "use a multi-resolution scheme by defining a scale factor for each level "
                               "using comma separated values (Default: 0.5,1)")
        + Argument ("factor").type_sequence_float ()

      + Option ("syn_niter", "the maximum number of iterations. This can be specified either as a single number "
                               "for all multi-resolution levels, or a single value for each level. (Default: 1000)")
        + Argument ("num").type_sequence_int ()

      + Option ("smooth_grad", "regularise the gradient field with Gaussian smoothing (standard deviation in mm, Default 3 x voxel_size)")
        + Argument ("stdev").type_float ()

      + Option ("smooth_disp", "regularise the displacement field with Gaussian smoothing (standard deviation in mm, Default 0.5 x voxel_size)")
        + Argument ("stdev").type_float ()

      + Option ("grad_step", "the initial gradient step size for SyN registration (Default: 0.12)") //TODO
        + Argument ("num").type_float ();


    const OptionGroup initialisation_options =
        OptionGroup ("Initialisation options")

      + Option ("rigid_init", "initialise either the rigid, affine, or syn registration with the supplied rigid transformation (as a 4x4 matrix)")
        + Argument ("file").type_file_in ()

      + Option ("affine_init", "initialise either the affine, or syn registration with the supplied affine transformation (as a 4x4 matrix)")
        + Argument ("file").type_file_in ()

      + Option ("syn_init", "initialise the syn registration with the supplied warp image (which includes the linear transform)")
        + Argument ("image").type_image_in ()

      + Option ("centre", "for rigid and affine registration only: Initialise the centre of rotation and initial translation. "
                          "Valid choices are: mass (which uses the image center of mass), geometric (geometric image centre) or none. "
                          "Default: mass (which may not be suited for multi-modality registration).")
        + Argument ("type").type_choice (initialisation_choices);



    const OptionGroup fod_options =
      OptionGroup ("FOD registration options")

      + Option ("directions", "the directions used for FOD reorienation using apodised point spread functions (Default: 60 directions)")
        + Argument ("file", "a list of directions [az el] generated using the gendir command.").type_file_in ()

      + Option ("lmax", "explicitly set the lmax to be used in FOD registration. By default FOD registration will "
                       "use lmax 4 SH coefficients")
        + Argument ("num").type_integer ()

      + Option ("noreorientation", "turn off FOD reorientation. Reorientation is on by default if the number "
                                   "of volumes in the 4th dimension corresponds to the number of coefficients in an "
                                   "antipodally symmetric spherical harmonic series (i.e. 6, 15, 28, 45, 66 etc");
  }
}

