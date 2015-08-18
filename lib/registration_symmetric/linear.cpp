//#include "registration_symmetric/linear.h"

//namespace MR
//{
//  namespace Image
//  {
//    namespace RegistrationSymmetric
//    {

//      using namespace App;

//      const char* initialisation_choices[] = { "mass", "geometric", "none", NULL };

//      const OptionGroup rigid_options =
//        OptionGroup ("Rigid registration options")

//        + Option ("rigid_out", "the output text file containing the rigid transformation as a 4x4 matrix")
//          + Argument ("file").type_file_out ()

//        + Option ("rigid_scale", "use a multi-resolution scheme by defining a scale factor for each level "
//                                 "using comma separated values (Default: 0.5,1)")
//          + Argument ("factor").type_sequence_float ()

//        + Option ("rigid_niter", "the maximum number of iterations. This can be specified either as a single number "
//                                 "for all multi-resolution levels, or a single value for each level. (Default: 1000)")
//          + Argument ("num").type_sequence_int ()

//        + Option ("rigid_cc", "metric: use cross correlation. default: least squares");


//      const OptionGroup affine_options =
//          OptionGroup ("Affine registration options")

//        + Option ("affine_out", "the output text file containing the affine transformation as a 4x4 matrix")
//          + Argument ("file").type_file_out ()

//        + Option ("affine_scale", "use a multi-resolution scheme by defining a scale factor for each level "
//                                 "using comma separated values (Default: 0.5,1)")
//          + Argument ("factor").type_sequence_float ()

//        + Option ("affine_niter", "the maximum number of iterations. This can be specified either as a single number "
//                                  "for all multi-resolution levels, or a single value for each level. (Default: 1000)")
//          + Argument ("num").type_sequence_int ()

//        + Option ("affine_cc", "metric: use cross correlation. default: least squares");


//      const OptionGroup syn_options =
//        OptionGroup ("SyN registration options")

//        + Option ("warp", "the output non-linear warp defined as a deformation field")
//          + Argument ("image").type_file_out ()

//        + Option ("syn_scale", "use a multi-resolution scheme by defining a scale factor for each level "
//                                 "using comma separated values (Default: 0.5,1)")
//          + Argument ("factor").type_sequence_float ()

//        + Option ("syn_niter", "the maximum number of iterations. This can be specified either as a single number "
//                                 "for all multi-resolution levels, or a single value for each level. (Default: 1000)")
//          + Argument ("num").type_sequence_int ()

//        + Option ("smooth_grad", "regularise the gradient field with Gaussian smoothing (standard deviation in mm, Default 3 x voxel_size)")
//          + Argument ("stdev").type_float ()

//        + Option ("smooth_disp", "regularise the displacement field with Gaussian smoothing (standard deviation in mm, Default 0.5 x voxel_size)")
//          + Argument ("stdev").type_float ()

//        + Option ("grad_step", "the initial gradient step size for SyN registration (Default: 0.12)") //TODO
//          + Argument ("num").type_float ();


//      const OptionGroup initialisation_options =
//          OptionGroup ("Initialisation options")

//        + Option ("rigid_init", "initialise either the rigid, affine, or syn registration with the supplied rigid transformation (as a 4x4 matrix)")
//          + Argument ("file").type_file_in ()

//        + Option ("affine_init", "initialise either the affine, or syn registration with the supplied affine transformation (as a 4x4 matrix)")
//          + Argument ("file").type_file_in ()

//        + Option ("syn_init", "initialise the syn registration with the supplied warp image (which includes the linear transform)")
//          + Argument ("image").type_image_in ()

//        + Option ("centre", "for rigid and affine registration only: Initialise the centre of rotation and initial translation. "
//                            "Valid choices are: mass (which uses the image center of mass), geometric (geometric image centre) or none. "
//                            "Default: mass (which may not be suited for multi-modality registration).")
//          + Argument ("type").type_choice (initialisation_choices);



//      const OptionGroup fod_options =
//        OptionGroup ("FOD registration options")

//        + Option ("directions", "the directions used for FOD reorienation using apodised point spread functions (Default: 60 directions)")
//          + Argument ("file", "a list of directions [az el] generated using the gendir command.").type_file_in ()

//        + Option ("lmax", "explicitly set the lmax to be used in FOD registration. By default FOD registration will "
//                         "first use lmax 2 until convergence, then add lmax 4 SH coefficients and run till convergence")
//          + Argument ("num").type_integer ()

//        + Option ("noreorientation", "turn off FOD reorientation. Reorientation is on by default if the number "
//                                     "of volumes in the 4th dimension corresponds to the number of coefficients in an "
//                                     "antipodally symmetric spherical harmonic series (i.e. 6, 15, 28, 45, 66 etc");
//    }
//  }
//}

