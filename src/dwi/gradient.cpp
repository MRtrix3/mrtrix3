#include "dwi/gradient.h"

namespace MR
{
  namespace DWI
  {

    const char* bvalue_scaling_options[] = { "none", "auto", "always", NULL };

    using namespace App;

    const OptionGroup GradOption = OptionGroup ("DW gradient encoding options")
      + Option ("grad",
          "specify the diffusion-weighted gradient scheme used in the acquisition. "
          "The program will normally attempt to use the encoding stored in the image "
          "header. This should be supplied as a 4xN text file with each line is in "
          "the format [ X Y Z b ], where [ X Y Z ] describe the direction of the "
          "applied gradient, and b gives the b-value in units of s/mm^2.")
      + Argument ("encoding").type_file()

      + Option ("fslgrad",
          "specify the diffusion-weighted gradient scheme used in the acquisition in FSL bvecs/bvals format.")
      + Argument ("bvecs").type_file()
      + Argument ("bvals").type_file()

      + Option ("bvalue_scaling",
          "specifies whether the b-values should be scaled according to the "
          "gradient amplitudes. Due to the way different MR scanners operate, "
          "multi-shell or DSI DW acquisition schemes are often stored as a "
          "constant nominal b-value, using the norm of the gradient amplitude "
          "to modulate the actual b-value. By default ('auto'), MRtrix will try to "
          "detect the presence of a multi-shell scheme, and in this case it will "
          "scale the b-value by the square of the corresponding gradient norm. "
          "Use 'none' to disable all scaling, and 'always' to force the scaling. "
          "The default action can also be set in the MRtrix config file, as a "
          "BValueScaling entry.")
      + Argument ("mode").type_choice (bvalue_scaling_options, 1);




    //CONF option: BValueScaling
    //CONF default: auto
    //CONF specifies whether b-values should be scaled according the DW gradient
    //CONF amplitudes - see the -bvalue_scaling option for details.

    int get_bvalue_scaling_mode() 
    {
      App::Options opt = App::get_options ("bvalue_scaling");
      if (opt.size()) 
        return opt[0][0];

      std::string scaling_config = File::Config::get ("BValueScaling");
      if (scaling_config.empty())
        return 1;

      for (int n = 0; bvalue_scaling_options[n]; ++n)
        if (scaling_config == bvalue_scaling_options[n])
          return n;

      throw Exception ("unknown value for config file entry 'BValueScaling': " + scaling_config 
          + " (expected one of " + join (bvalue_scaling_options, ", ") + ")");
    }








    void save_bvecs_bvals (const Image::Header& header, const std::string& path)
    {

      std::string bvecs_path, bvals_path;
      if (path.size() >= 5 && path.substr (path.size() - 5, path.size()) == "bvecs") {
        bvecs_path = path;
        bvals_path = path.substr (0, path.size() - 5) + "bvals";
      } else if (path.size() >= 5 && path.substr (path.size() - 5, path.size()) == "bvals") {
        bvecs_path = path.substr (0, path.size() - 5) + "bvecs";
        bvals_path = path;
      } else {
        bvecs_path = path + "bvecs";
        bvals_path = path + "bvals";
      }

      const Math::Matrix<float>& grad (header.DW_scheme());
      Math::Matrix<float> G (grad.rows(), 3);

      // rotate vectors from scanner space to image space
      Math::Matrix<float> D (header.transform());
      Math::Permutation p (4);
      int signum;
      Math::LU::decomp (D, p, signum);
      Math::Matrix<float> image2scanner (4,4);
      Math::LU::inv (image2scanner, D, p);
      Math::Matrix<float> rotation = image2scanner.sub (0,3,0,3);
      Math::Matrix<float> grad_G = grad.sub (0, grad.rows(), 0, 3);
      Math::mult (G, float(0.0), float(1.0), CblasNoTrans, grad_G, CblasTrans, rotation);

      // deal with FSL requiring gradient directions to coincide with data strides
      // also transpose matrices in preparation for file output
      std::vector<size_t> order = Image::Stride::order (header, 0, 3);
      Math::Matrix<float> bvecs (3, grad.rows());
      Math::Matrix<float> bvals (1, grad.rows());
      for (size_t n = 0; n < G.rows(); ++n) {
        bvecs(0,n) = header.stride(order[0]) > 0 ? G(n,order[0]) : -G(n,order[0]);
        bvecs(1,n) = header.stride(order[1]) > 0 ? G(n,order[1]) : -G(n,order[1]);
        bvecs(2,n) = header.stride(order[2]) > 0 ? G(n,order[2]) : -G(n,order[2]);
        bvals(0,n) = grad(n,3);
      }

      bvecs.save (bvecs_path);
      bvals.save (bvals_path);

    }





  }
}


