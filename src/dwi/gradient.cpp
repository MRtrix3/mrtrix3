#include "dwi/gradient.h"

namespace MR
{
  namespace DWI
  {

    using namespace App;

    OptionGroup GradImportOptions (bool include_bvalue_scaling)
    {
      OptionGroup group ("DW gradient table import options");

      group 
        + Option ("grad",
            "specify the diffusion-weighted gradient scheme used in the acquisition. "
            "The program will normally attempt to use the encoding stored in the image "
            "header. This should be supplied as a 4xN text file with each line is in "
            "the format [ X Y Z b ], where [ X Y Z ] describe the direction of the "
            "applied gradient, and b gives the b-value in units of s/mm^2.")
        +   Argument ("encoding").type_file_in()

        + Option ("fslgrad",
            "specify the diffusion-weighted gradient scheme used in the acquisition in FSL bvecs/bvals format.")
        +   Argument ("bvecs").type_file_in()
        +   Argument ("bvals").type_file_in();

      if (include_bvalue_scaling) 
        group 
          + Option ("bvalue_scaling",
              "specifies whether the b-values should be scaled by the square of "
              "the corresponding DW gradient norm, as often required for "
              "multi-shell or DSI DW acquisition schemes. The default action can "
              "also be set in the MRtrix config file, under the BValueScaling entry. "
              "Valid choices are yes/no, true/false, 0/1 (default: true).")
          +   Argument ("mode").type_bool (true);

      return group;
    }


    OptionGroup GradExportOptions() 
    {
      return OptionGroup ("DW gradient table export options")

        + Option ("export_grad_mrtrix", "export the diffusion-weighted gradient table to file in MRtrix format")
        +   Argument ("path").type_file_out()

        + Option ("export_grad_fsl", "export the diffusion-weighted gradient table to files in FSL (bvecs / bvals) format")
        +   Argument ("bvecs_path").type_file_out()
        +   Argument ("bvals_path").type_file_out();
    }






    void save_bvecs_bvals (const Image::Header& header, const std::string& bvecs_path, const std::string& bvals_path)
    {

      const Math::Matrix<float>& grad (header.DW_scheme());
      Math::Matrix<float> G (grad.rows(), 3);

      // rotate vectors from scanner space to image space
      Math::Matrix<float> grad_G = grad.sub (0, grad.rows(), 0, 3);
      Math::Matrix<float> rotation = header.transform().sub (0,3,0,3);
      Math::mult (G, 0.0f, 1.0f, CblasNoTrans, grad_G, CblasNoTrans, rotation);

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




    void export_grad_commandline (const Image::Header& header) 
    {
      auto check = [](const Image::Header& h) -> const Image::Header& {
        if (!h.DW_scheme().is_set())
          throw Exception ("no gradient information found within image \"" + h.name() + "\"");
        if (h.DW_scheme().rows() == 0 || h.DW_scheme().columns() < 4)
          throw Exception ("DW scheme in header \"" + h.name() + "\" has unexpected dimensions");
        return h;
      };

      App::Options opt = get_options ("export_grad_mrtrix");
      if (opt.size()) 
        check (header).DW_scheme().save (opt[0][0]);

      opt = get_options ("export_grad_fsl");
      if (opt.size()) 
        save_bvecs_bvals (check (header), opt[0][0], opt[0][1]);
    }




  }
}


