#include "dwi/tractography/tractography.h"

#define MAX_TRIALS 1000

namespace MR
{
  namespace DWI
  {

    namespace Tractography
    {

      using namespace App;

      const OptionGroup TrackOption = OptionGroup ("Streamlines tractography options")

      + Option ("seed",
            "specify the seed region of interest. This should be either the path "
            "to a binary mask image, or a comma-separated list of 4 floating-point "
            "values, specifying the [x,y,z] coordinates of the centre and radius "
            "of a spherical ROI.")
          .allow_multiple()
          + Argument ("spec")

      + Option ("include",
            "specify an inclusion region of interest, in the same format as the "
            "seed region. Only tracks that enter all such inclusion ROI will be "
            "produced.")
          .allow_multiple()
          + Argument ("spec")

      + Option ("exclude",
            "specify an exclusion region of interest, in the same format as the "
            "seed region. Only tracks that enter any such exclusion ROI will be "
            "discarded.")
          .allow_multiple()
          + Argument ("spec")

      + Option ("mask",
            "specify a mask region of interest, in the same format as the seed "
            "region. Tracks will be terminated when they leave any such ROI.")
          .allow_multiple()
          + Argument ("spec")

      + Option ("grad",
            "specify the diffusion encoding scheme (may be required for FACT "
            "and RSFACT, ignored otherwise)")
          + Argument ("file")

      + Option ("step",
            "set the step size of the algorithm in mm (default for iFOD1: 0.1 x voxelsize; for iFOD2: 0.5 x voxelsize).")
          + Argument ("size").type_float (0.0, 0.0, INFINITY)

      + Option ("angle",
            "set the maximum angle between successive steps (default is 90° x stepsize / voxelsize).")
          + Argument ("theta").type_float (0.0, 90.0, 90.0)

      + Option ("number",
            "set the desired number of tracks. The program will continue to "
            "generate tracks until this number of tracks have been selected "
            "and written to the output file (default is 100 for *_STREAM methods, "
            "1000 for *_PROB methods).")
          + Argument ("tracks").type_integer (1, 1, std::numeric_limits<int>::max())

      + Option ("maxnum",
            "set the maximum number of tracks to generate. The program will "
            "not generate more tracks than this number, even if the desired "
            "number of tracks hasn't yet been reached (default is 100 x number).")
          + Argument ("tracks").type_integer (1, 1, INT_MAX)

      + Option ("maxlength",
            "set the maximum length of any track in mm (default is 100 x voxelsize).")
          + Argument ("value").type_float (0.0, 0.0, INFINITY)

      + Option ("minlength",
            "set the minimum length of any track in mm (default is 5 x voxelsize).")
          + Argument ("value").type_float (0.0, 0.0, INFINITY)

      + Option ("cutoff",
            "set the FA or FOD amplitude cutoff for terminating tracks "
            "(default is 0.1).")
          + Argument ("value").type_float (0.0, 0.1, INFINITY)

      + Option ("initcutoff",
            "set the minimum FA or FOD amplitude for initiating tracks (default "
            "is twice the normal cutoff).")
          + Argument ("value").type_float (0.0, 0.1, INFINITY)

      + Option ("trials",
            "set the maximum number of sampling trials at each point (only "
            "used for probabilistic tracking).")
          + Argument ("number").type_integer (1, MAX_TRIALS, std::numeric_limits<int>::max())

      + Option ("unidirectional",
            "track from the seed point in one direction only (default is to "
            "track in both directions).")

      + Option ("initdirection",
            "specify an initial direction for the tracking (this should be "
            "supplied as a vector of 3 comma-separated values.")
          + Argument ("dir").type_sequence_float()

      + Option ("noprecomputed",
            "do NOT pre-compute legendre polynomial values. Warning: "
            "this will slow down the algorithm by a factor of approximately 4.")

      + Option ("power",
            "raise the FOD to the power specified (default is 1/nsamples).")
          + Argument ("value").type_float (1e-6, 1.0, 1e6)

      + Option ("samples",
            "set the number of FOD samples to take per step for the 2nd order "
            "(iFOD2) method (Default: 4).")
          + Argument ("number").type_integer (2, 4, 100)

      + Option ("rk4", "use 4th-order Runge-Kutta integration");



      void load_streamline_properties (Properties& properties)
      {

        using namespace MR::App;

        Options opt = get_options ("seed");
        for (size_t i = 0; i < opt.size(); ++i)
          properties.seed.add (ROI (opt[i][0]));

        opt = get_options ("include");
        for (size_t i = 0; i < opt.size(); ++i)
          properties.include.add (ROI (opt[i][0]));

        opt = get_options ("exclude");
        for (size_t i = 0; i < opt.size(); ++i)
          properties.exclude.add (ROI (opt[i][0]));

        opt = get_options ("mask");
        for (size_t i = 0; i < opt.size(); ++i)
          properties.mask.add (ROI (opt[i][0]));

        opt = get_options ("grad");
        if (opt.size()) properties["DW_scheme"] = std::string (opt[0][0]);

        opt = get_options ("step");
        if (opt.size()) properties["step_size"] = std::string (opt[0][0]);

        opt = get_options ("angle");
        if (opt.size()) properties["max_angle"] = std::string (opt[0][0]);

        opt = get_options ("number");
        if (opt.size()) properties["max_num_tracks"] = std::string (opt[0][0]);

        opt = get_options ("maxnum");
        if (opt.size()) properties["max_num_attempts"] = std::string (opt[0][0]);

        opt = get_options ("maxlength");
        if (opt.size()) properties["max_dist"] = std::string (opt[0][0]);

        opt = get_options ("minlength");
        if (opt.size()) properties["min_dist"] = std::string (opt[0][0]);

        opt = get_options ("cutoff");
        if (opt.size()) properties["threshold"] = std::string (opt[0][0]);

        opt = get_options ("initcutoff");
        if (opt.size()) properties["init_threshold"] = std::string (opt[0][0]);

        opt = get_options ("trials");
        if (opt.size()) properties["max_trials"] = std::string (opt[0][0]);

        opt = get_options ("unidirectional");
        if (opt.size()) properties["unidirectional"] = "1";

        opt = get_options ("initdirection");
        if (opt.size()) properties["init_direction"] = std::string (opt[0][0]);

        opt = get_options ("noprecomputed");
        if (opt.size()) properties["sh_precomputed"] = "0";

        opt = get_options ("power");
        if (opt.size()) properties["fod_power"] = std::string (opt[0][0]);

        opt = get_options ("samples");
        if (opt.size()) properties["samples_per_step"] = std::string (opt[0][0]);

        opt = get_options ("rk4");
        if (opt.size()) properties["rk4"] = "1";

      }



    }
  }
}


