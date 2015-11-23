#include "command.h"
#include "ptr.h"
#include "progressbar.h"
#include "image/threaded_loop.h"
#include "image/buffer.h"
#include "image/buffer_scratch.h"
#include "image/adapter/extract.h"
#include "math/hermite.h"
#include "image/voxel.h"
#include "dwi/gradient.h"
#include "dwi/sdeconv/transform.h"

/*
  InputBufferType::voxel_type dwi_vox (dwi_buffer);
  OutputBufferType::voxel_type FOD_vox (FOD_buffer);
  Ptr<MaskBufferType::voxel_type> mask_vox;
    mask_vox = new MaskBufferType::voxel_type (*mask_buffer);
*/


namespace MR {
  namespace DWI {
    namespace MTCSD {

      using namespace App;

      const OptionGroup Options = OptionGroup ("multi-tissue CSD options")
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
        + Argument ("file").type_file_in()

        + Option ("norm_lambda",
            "the regularisation parameter lambda that controls the strength of the "
            "constraint on the norm of the solution (default = 1.0).")
        + Argument ("value").type_float (0.0, 1.0, 1.0e12);



      template <typename ValueType>
        class Shared {
          public:
            Shared (const Image::Header& header, const std::vector< Response<ValueType> >& response) 
            {
              std::vector<size_t> bzeros;
              Math::Matrix<ValueType> grad, directions;
              M = DWI::get_SH_to_DWI_mapping (header, grad, directions, dwis, bzeros, response, ncoefs);
              Minv = Math::pinv (M);
            }

            bool is_multi_shell () const { 
              return dwis.empty();
            }

            size_t num_dwi () const {
              return is_multi_shell() ? M.rows() : dwis.size();
            }

            std::vector<size_t> dwis;
            std::vector<size_t> ncoefs;
            Math::Matrix<ValueType> M, Minv;
        };




      template <typename ValueType>
        class Processor {
          public:
            Processor (const Shared<ValueType>& shared) :
              shared (shared),
              dwi (shared.M.rows()),
              fod (shared.M.columns()) { }

            template <class VoxelTypeDWI, class VoxelTypeFOD>
              void operator() (VoxelTypeDWI& dwi_vox, VoxelTypeFOD& fod_vox) {
                for (dwi_vox[3] = 0; dwi_vox[3] < dwi_vox.dim(3); ++dwi_vox[3]) 
                  dwi[dwi_vox[3]] = dwi_vox.value();

                Math::mult (fod, shared.Minv, dwi);

                for (fod_vox[3] = 0; fod_vox[3] < fod_vox.dim(3); ++fod_vox[3]) 
                  fod_vox.value() = fod[fod_vox[3]];
              }

            template <class VoxelTypeDWI, class VoxelTypeFOD, class VoxelTypeMask>
              void operator() (VoxelTypeDWI& dwi_vox, VoxelTypeFOD& fod_vox, VoxelTypeMask& mask_vox) {
                if (mask_vox.value()) 
                  operator() (dwi_vox, fod_vox);
              }

          protected:
            const Shared<ValueType>& shared;
            Math::Vector<ValueType> dwi, fod;
        };




    }
  }
}




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
        "the output spherical harmonics coefficients image.").type_image_out().allow_multiple();

  OPTIONS
    + DWI::MTCSD::Options
    + DWI::GradImportOptions()
    + DWI::ShellOption
    + Image::Stride::StrideOption;
}



typedef float value_type;
typedef double compute_type;
typedef Image::Buffer<value_type> SourceBufferType;
typedef Image::BufferScratch<value_type> DWIBufferType;
typedef Image::Buffer<value_type> OutputBufferType;
typedef Image::Buffer<bool> MaskBufferType;






void run ()
{
  const Image::Header dwi_header (argument[0]);

  // load response(s):
  std::vector< DWI::Response<compute_type> > response (1);
  response[0].init (DWI::default_WM_response());

  // initialise MTCSD:
  DWI::MTCSD::Shared<compute_type> shared (dwi_header, response);

  // load mask if needed:
  Ptr<MaskBufferType> mask_buffer;
  Ptr<MaskBufferType::voxel_type> mask_vox;
  Options opt = get_options ("mask");
  if (opt.size()) {
    mask_buffer = new MaskBufferType (opt[0][0]);
    Image::check_dimensions (dwi_header, *mask_buffer, 0, 3);
    mask_vox = new MaskBufferType::voxel_type (*mask_buffer);
  }

  // set up output FOD buffer:
  // this will need to be changed to use a BufferScratch
  // and a write-back to separate files implemented at the end of processing
  Image::Header header (dwi_header);
  header.set_ndim (4);
  header.dim(3) = shared.M.columns();
  header.datatype() = DataType::from_command_line (DataType::Float32);
  Image::Stride::set_from_command_line (header, Image::Stride::contiguous_along_axis(3));
  OutputBufferType fod_buffer (argument[1], header);
  OutputBufferType::voxel_type fod_vox (fod_buffer);

  // set up DWI scratch buffer:
  Image::Info info (header);
  info.set_ndim (4);
  info.dim(3) = shared.num_dwi();
  Image::Stride::set (info, Image::Stride::contiguous_along_axis(3));
  DWIBufferType dwi_buffer (info, "DWI");
  DWIBufferType::voxel_type dwi_vox (dwi_buffer);

  { // preload dwi data:
    SourceBufferType dwi_source_buffer (dwi_header);
    SourceBufferType::voxel_type dwi_source (dwi_source_buffer);
    if (shared.is_multi_shell()) 
      Image::threaded_copy_with_progress_message ("loading DWI data... ", dwi_source, dwi_vox);
    else {
      Image::Adapter::Extract1D<SourceBufferType::voxel_type> dwi_only (dwi_source, 3, container_cast< std::vector<int> > (shared.dwis));
      Image::threaded_copy_with_progress_message ("loading DWI data... ", dwi_only, dwi_vox);
    }
  }


  // launch the processing:
  if (mask_vox) 
    Image::ThreadedLoop (std::string("performing ") + (shared.is_multi_shell() ? "multi-shell " : "") + (shared.ncoefs.size() > 1 ? "multi-tissue " : "" ) + "CSD...", 
        dwi_vox, 0, 3).run (DWI::MTCSD::Processor<compute_type> (shared), dwi_vox, fod_vox, *mask_vox);
  else
    Image::ThreadedLoop (std::string("performing ") + (shared.is_multi_shell() ? "multi-shell " : "") + (shared.ncoefs.size() > 1 ? "multi-tissue " : "" ) + "CSD...", 
        dwi_vox, 0, 3).run (DWI::MTCSD::Processor<compute_type> (shared), dwi_vox, fod_vox);
}

