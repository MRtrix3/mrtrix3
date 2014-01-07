#include "app.h"
#include "ptr.h"
#include "progressbar.h"
#include "thread/exec.h"
#include "thread/queue.h"
#include "image/loop.h"
#include "image/buffer.h"
#include "image/buffer_preload.h"
#include "image/voxel.h"
#include "dwi/sdeconv/constrained.h"

using namespace MR;
using namespace App;

MRTRIX_APPLICATION

void usage ()
{
  DESCRIPTION
    + "perform non-negativity constrained spherical deconvolution."

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
    + Argument ("response",
        "the diffusion-weighted signal response function for a single fibre population.").type_file()
    + Argument ("SH",
        "the output spherical harmonics coefficients image.").type_image_out();

  OPTIONS
    + DWI::GradOption
    + DWI::CSD_options
    + Image::Stride::StrideOption;
}



typedef float value_type;
typedef double cost_value_type;
typedef Image::BufferPreload<value_type> InputBufferType;
typedef Image::Buffer<value_type> OutputBufferType;
typedef Image::Buffer<bool> MaskBufferType;





class Processor
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
        if (!finite (data[n]))
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







void run ()
{

  std::vector<ssize_t> strides (4, 0);
  strides[3] = 1;
  InputBufferType dwi_buffer (argument[0], strides);

  Ptr<MaskBufferType> mask_data;
  Ptr<MaskBufferType::voxel_type> mask_vox;
  Options opt = get_options ("mask");
  if (opt.size()) {
    mask_data = new MaskBufferType (opt[0][0]);
    mask_vox = new MaskBufferType::voxel_type (*mask_data);
  }

  DWI::CSDeconv<value_type>::Shared shared (dwi_buffer);
  shared.parse_cmdline_options();
  shared.set_response (argument[1]);
  shared.init();


  Image::Header header (dwi_buffer);
  header.dim(3) = shared.nSH();
  header.datatype() = DataType::Float32;
  opt = get_options ("stride");
  if (opt.size()) {
    std::vector<int> strides = opt[0][0];
    if (strides.size() > header.ndim())
      throw Exception ("too many axes supplied to -stride option");
    for (size_t n = 0; n < strides.size(); ++n)
      header.stride(n) = strides[n];
  } else {
    header.stride(0) = 2;
    header.stride(1) = 3;
    header.stride(2) = 4;
    header.stride(3) = 1;
  }
  OutputBufferType FOD_buffer (argument[2], header);

  InputBufferType::voxel_type dwi_vox (dwi_buffer);
  OutputBufferType::voxel_type FOD_vox (FOD_buffer);

  Processor processor (dwi_vox, FOD_vox, mask_vox, shared);
  Image::ThreadedLoop loop ("performing constrained spherical deconvolution...", dwi_vox, 1, 0, 3);
  loop.run (processor);
}

