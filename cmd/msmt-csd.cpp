#include "command.h"
#include "ptr.h"
#include "progressbar.h"
#include "image/threaded_loop.h"
#include "image/buffer.h"
#include "image/buffer_preload.h"
#include "math/hermite.h"
#include "image/voxel.h"
#include "dwi/gradient.h"
#include "dwi/sdeconv/transform.h"

/*
  InputBufferType::voxel_type dwi_vox (dwi_buffer);
  OutputBufferType::voxel_type FOD_vox (FOD_buffer);
  Ptr<MaskBufferType::voxel_type> mask_vox;
    mask_vox = new MaskBufferType::voxel_type (*mask_data);
*/


namespace MR {
  namespace DWI {

    using namespace App;

    const OptionGroup MT_CSDOptions = OptionGroup ("multi-tissue CSD options")
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
                "supplied as a text file containing the [ az el ] pairs for the directions.")
      + Argument ("file").type_file()

      + Option ("norm_lambda",
                "the regularisation parameter lambda that controls the strength of the "
                "constraint on the norm of the solution (default = 1.0).")
      + Argument ("value").type_float (0.0, 1.0, 1.0e12)

      + Option ("niter",
                "the maximum number of iterations to perform for each voxel (default = 50).")
      + Argument ("number").type_integer (1, 50, 1000);

  }
}


    /*

      template <typename ValueType>
        class MSMT_CSD
        {
          public:
            Processor (InputBufferType::voxel_type& DWI_vox,
                OutputBufferType::voxel_type& FOD_vox,
                Ptr<MaskBufferType::voxel_type>& mask_vox,
        const DWI::CSDeconv<value_type>::Shared& shared) :
      dwi (DWI_vox),
      fod (FOD_vox),
      mask (mask_vox),
      sdeconv (shared),
      data (shared.dwis.size()) {
        if (mask)
          Image::check_dimensions (*mask, dwi, 0, 3);
      }



    void operator () (const Image::Iterator& pos) {
      if (!load_data (pos))
        return;

      sdeconv.set (data);

      size_t n;
      for (n = 0; n < sdeconv.shared.niter; n++)
        if (sdeconv.iterate())
          break;

      if (n >= sdeconv.shared.niter)
        INFO ("voxel [ " + str (pos[0]) + " " + str (pos[1]) + " " + str (pos[2]) +
            " ] did not reach full convergence");

      write_back (pos);
    }


    void run () {
      Image::ThreadedLoop loop ("performing multi-shell multi-tissue constrained spherical deconvolution...", dwi_vox, 1, 0, 3);
      loop.run (processor);
    }

  private:
    InputBufferType::voxel_type dwi;
    OutputBufferType::voxel_type fod;
    Ptr<MaskBufferType::voxel_type> mask;
    DWI::CSDeconv<value_type> sdeconv;
    Math::Vector<value_type> data;



    bool load_data (const Image::Iterator& pos) {
      if (mask) {
        Image::voxel_assign (*mask, pos);
        if (!mask->value())
          return false;
      }

      Image::voxel_assign (dwi, pos);

      for (size_t n = 0; n < sdeconv.shared.dwis.size(); n++) {
        dwi[3] = sdeconv.shared.dwis[n];
        data[n] = dwi.value();
        if (!std::isfinite (data[n]))
          return false;
        if (data[n] < 0.0)
          data[n] = 0.0;
      }

      return true;
    }



    void write_back (const Image::Iterator& pos) {
      Image::voxel_assign (fod, pos);
      for (fod[3] = 0; fod[3] < fod.dim (3); ++fod[3])
        fod.value() = sdeconv.FOD() [fod[3]];
    }

};


*/



using namespace MR;
using namespace App;


void usage ()
{
  DESCRIPTION
    + "perform multi-shell, multi-tissue constrained spherical deconvolution."

    + "Note that this program makes use of implied symmetries in the diffusion "
    "profile. First, the fact the signal attenuation profile is real implies "
    "that it has conjugate symmetry, i.e. Y(l,-m) = Y(l,m)* (where * denotes "
    "the complex conjugate). Second, the diffusion profile should be "
    "antipodally symmetric (i.e. S(x) = S(-x)), implying that all odd l "
    "components should be zero. Therefore, this program only computes the even "
    "elements."

    + "Note that the spherical harmonics equations used here differ slightly "
    "from those conventionally used, in that the (-1)^m factor has been "
    "omitted. This should be taken into account in all subsequent calculations."

    + Math::SH::encoding_description;

  ARGUMENTS
    + Argument ("dwi",
        "the input diffusion-weighted image.").type_image_in()
    + Argument ("SH",
        "the output spherical harmonics coefficients image.").type_image_out();

  OPTIONS
    + DWI::MT_CSDOptions
    + DWI::GradOption
    + DWI::ShellOption
    + Image::Stride::StrideOption;
}



typedef float value_type;
typedef double cost_value_type;
typedef Image::BufferPreload<value_type> InputBufferType;
typedef Image::Buffer<value_type> OutputBufferType;
typedef Image::Buffer<bool> MaskBufferType;






void run ()
{
  Image::Header dwi_header (argument[0]);

  std::vector< DWI::Response<float> > response (1);
  response[0].init (DWI::default_WM_response());

  Math::Matrix<float> M = DWI::get_SH_to_DWI_mapping (dwi_header, response);
  M.save ("M.txt");

  /*
  InputBufferType dwi_buffer (argument[0], Image::Stride::contiguous_along_axis (3));

  Ptr<MaskBufferType> mask_data;
  Options opt = get_options ("mask");
  if (opt.size()) 
    mask_data = new MaskBufferType (opt[0][0]);

  DWI::MSMT_CSD<value_type>::Shared shared (dwi_buffer);
  shared.parse_cmdline_options();
  shared.init();


  Image::Header header (dwi_buffer);
  header.set_ndim (4);
  header.dim(3) = shared.nSH();
  header.datatype() = DataType::from_command_line (DataType::Float32);
  Image::Stride::set_from_command_line (header, Image::Stride::contiguous_along_axis(3));
  OutputBufferType FOD_buffer (argument[2], header);


  Processor processor (shared).run();
  */
}

