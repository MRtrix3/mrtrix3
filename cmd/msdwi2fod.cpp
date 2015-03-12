#include "command.h"
#include "ptr.h"
#include "timer.h"
#include "progressbar.h"
#include "image/copy.h"
#include "image/threaded_loop.h"
#include "image/adapter/subset.h"
#include "image/buffer.h"
#include "image/buffer_preload.h"
#include "image/buffer_scratch.h"
#include "image/voxel.h"
#include "dwi/gradient.h"
#include "math/SH.h"
#include "dwi/directions/predefined.h"
#include "math/constrained_least_squares.h"


using namespace MR;
using namespace App;

void usage () {
  AUTHOR = "Ben Jeurissen (ben.jeurissen@gmail.com)";

  DESCRIPTION
    + "Multi-shell, multi-tissue CSD";

  REFERENCES 
    + "Jeurissen, B; Tournier, J-D; Dhollander, T; Connelly, A; Sijbers, J "
    "Multi-tissue constrained spherical deconvolution for improved analysis of multi-shell diffusion MRI data "
    "NeuroImage, in press, DOI: 10.1016/j.neuroimage.2014.07.061";

  ARGUMENTS
    + Argument ("dwi",
      "the input diffusion-weighted image.").type_image_in()
    + Argument ("response odf",
      "the input tissue response and the output ODF image.").allow_multiple();

  OPTIONS
    + Option ("mask",
        "only perform computation within the specified binary brain mask image.")
    + Argument ("image").type_image_in ()

    + Option ("lmax","")
    + Argument ("order").type_sequence_int()

    + Option ("directions",
                "specify the directions over which to apply the non-negativity constraint "
                "(by default, the built-in 300 direction set is used). These should be "
                "supplied as a text file containing the [ az el ] pairs for the directions.")
    + Argument ("file").type_file_in()

    + DWI::GradImportOptions;
}

typedef double value_type;
typedef Image::BufferPreload<value_type> InputBufferType;
typedef Image::Buffer<bool> MaskBufferType;
typedef Image::Buffer<value_type> OutputBufferType;
typedef Image::BufferScratch<value_type> BufferScratchType;


class Shared {
  public:
    Shared (std::vector<int>& lmax_, std::vector<Math::Matrix<value_type> >& response_, Math::Matrix<value_type>& grad_, Math::Matrix<value_type>& HR_dirs) :
      lmax(lmax_),
      response(response_),
      grad(grad_)
  {
    /* create forward convolution matrix */
    DWI::Shells shells (grad);
    size_t nbvals = shells.count();
    size_t nsamples = grad.rows();
    size_t ntissues = lmax.size();
    size_t nparams = 0;
    int maxlmax = 0;
    for(size_t i = 0; i < lmax.size(); i++) {
      nparams+=Math::SH::NforL(lmax[i]);
      if (lmax[i] > maxlmax)
        maxlmax = lmax[i];
    }
    Math::Matrix<value_type> C (nsamples,nparams);

    std::vector<size_t> dwilist;
    for (size_t i = 0; i < nsamples; i++)
      dwilist.push_back(i);
    Math::Matrix<value_type> directions; DWI::gen_direction_matrix (directions, grad, dwilist);
    Math::Matrix<value_type> SHT; Math::SH::init_transform (SHT, directions, maxlmax);
    for (size_t i = 0; i < SHT.rows(); i++)
      for (size_t j = 0; j < SHT.columns(); j++)
        if (isnan(SHT(i,j)))
          SHT(i,j) = 0;

    Math::Matrix<value_type> delta(1,2);
    Math::Matrix<value_type> DSH__; Math::SH::init_transform (DSH__, delta, maxlmax);
    Math::Vector<value_type> DSH_ = DSH__.row(0);
    Math::Vector<value_type> DSH(maxlmax/2+1);
    size_t j = 0;
    for (size_t i = 0; i < DSH_.size(); i++)
      if (DSH_[i] != 0) {
        DSH[j] = DSH_[i];
        j++;
      }

    size_t pbegin = 0;
    for (size_t tissue_idx = 0; tissue_idx < ntissues; tissue_idx++) {
      size_t tissue_lmax = lmax[tissue_idx];
      size_t tissue_n = Math::SH::NforL(tissue_lmax);
      size_t tissue_nmzero = tissue_lmax/2+1;
      for (size_t shell_idx = 0; shell_idx < nbvals; shell_idx++) {
        Math::Vector<value_type> response_ = response[tissue_idx].row(shell_idx);
        Math::Vector<value_type> response__(response_);
        response__/=DSH.sub(0,tissue_nmzero);
        Math::Vector<value_type> fconv(tissue_n);
        int li = 0; int mi = 0;
        for (int l = 0; l <= int(tissue_lmax); l+=2) {
          for (int m = -l; m <= l; m++) {
            fconv[mi] = response__[li];
            mi++;
          }
          li++;
        }
        std::vector<size_t> vols = shells[shell_idx].get_volumes();
        for (size_t idx = 0; idx < vols.size(); idx++) {
          Math::Vector<value_type> SHT_(SHT.row(vols[idx]).sub(0,tissue_n));
          Math::Vector<value_type> SHT__(SHT_);
          SHT__*=fconv;
          C.row(vols[idx]).sub(pbegin,pbegin+tissue_n) = SHT__;
        }
      }
      pbegin+=tissue_n;
    }

    /* create constraint matrix */
    std::vector<size_t> m(lmax.size());
    std::vector<size_t> n(lmax.size());
    size_t M = 0;
    size_t N = 0;

    //Math::Matrix<value_type> HR_dirs;
    //DWI::Directions::electrostatic_repulsion_300(HR_dirs);
    Math::Matrix<value_type> SHT300; Math::SH::init_transform (SHT300, HR_dirs, maxlmax);

    for(size_t i = 0; i < lmax.size(); i++) {
      if (lmax[i] > 0)
        m[i] = HR_dirs.rows();
      else
        m[i] = 1;
      M+=m[i];
      n[i] = Math::SH::NforL(lmax[i]);
      N+=n[i];
    }

    Math::Matrix<value_type> A (M,N);
    size_t b_m = 0; size_t b_n = 0;
    for(size_t i = 0; i < lmax.size(); i++) {
      A.sub(b_m,b_m+m[i],b_n,b_n+n[i]) = SHT300.sub(0,m[i],0,n[i]);
      b_m+=m[i]; b_n+=n[i];
    }
    A.save ("A.txt",16);
    C.save ("H.txt",16);
    problem = Math::ICLS::Problem<value_type> (C, A, 1.0e-10, 1.0e-6);
  };

  public:
    std::vector<int> lmax;
    std::vector<Math::Matrix<value_type> > response;
    Math::Matrix<value_type>& grad;
    Math::ICLS::Problem<value_type> problem;
};




class Processor {
  public:
    Processor (
        Shared& shared,
        InputBufferType::voxel_type& dwi_in_vox,
        Ptr<MaskBufferType::voxel_type>& mask_in_vox,
        BufferScratchType::voxel_type& fodf_out_vox)
      :
        shared (shared),
        solver (shared.problem),
        dwi_in(dwi_in_vox),
        mask_in(mask_in_vox),
        fodf_out(fodf_out_vox),
        dwi(dwi_in.dim(3)),
        fodf(fodf_out.dim(3))
  { }

    void operator () (const Image::Iterator& pos) {
      if (!load_data(pos))
        return;
      size_t niter = solver (fodf, dwi);
      if (niter >= shared.problem.max_niter) {
        INFO ("failed to converge for voxel " + str(pos));
        dwi.save("d" + str(pos) + ".txt",16);
      }
      write_back (pos);
    }

  private:
    const Shared& shared;
    Math::ICLS::Solver<value_type> solver;
    InputBufferType::voxel_type dwi_in;
    Ptr<MaskBufferType::voxel_type> mask_in;
    BufferScratchType::voxel_type fodf_out;
    Math::Vector<value_type> dwi;
    Math::Vector<value_type> fodf;

    bool load_data (const Image::Iterator& pos) {
      if (mask_in) {
        Image::assign_pos(pos) (*mask_in);
        if (!mask_in->value())
          return false;
      }
      Image::assign_pos(pos) (dwi_in);

      for (auto l = Image::Loop(3) (dwi_in); l; ++l) {
        dwi[dwi_in[3]] = dwi_in.value();
        if (!std::isfinite (dwi[dwi_in[3]]))
          return false;
        if (dwi[dwi_in[3]] < 0.0)
          dwi[dwi_in[3]] = 0.0;
      }
      return true;
    }

    void write_back (const Image::Iterator& pos) {
      Image::assign_pos(pos) (fodf_out);
      for (auto l = Image::Loop(3) (fodf_out); l; ++l) 
        fodf_out.value() = fodf[fodf_out[3]];
    }

};



void run () {
  /* input DWI image */
  InputBufferType dwi_in_buffer (argument[0], Image::Stride::contiguous_along_axis(3));
  InputBufferType::voxel_type dwi_in_vox (dwi_in_buffer);
  
  /* input gradient directions */
  auto grad = DWI::get_valid_DW_scheme<value_type> (dwi_in_buffer);
  
  DWI::Shells shells (grad);
  size_t nbvals = shells.count();
  
  /* input response functions */
  std::vector<int> lmax;
  std::vector<Math::Matrix<value_type> > response;
  for (size_t i = 0; i < (argument.size()-1)/2; i++) {
    Math::Matrix<value_type> r(argument[i*2+1]);
    if (r.rows() != nbvals)
      throw Exception ("number of rows in response function text file should match number of shells in dwi");
    response.push_back(r);
    lmax.push_back((r.columns()-1)*2);
  }
  
  /* input mask image */
  Ptr<MaskBufferType> mask_in_buffer;
  Ptr<MaskBufferType::voxel_type> mask_in_vox;
  Options opt = get_options ("mask");
  if (opt.size()) {
    mask_in_buffer = new MaskBufferType (opt[0][0]);
    Image::check_dimensions (*mask_in_buffer, dwi_in_buffer, 0, 3);
    mask_in_vox = new MaskBufferType::voxel_type (*mask_in_buffer);
  }


  /* lmax */
  opt = get_options ("lmax");
  if (opt.size()) {
    std::vector<int> lmax_in = opt[0][0];
    if (lmax_in.size() == lmax.size()) {
      for (size_t i = 0; i<lmax_in.size(); i++) {
        if (lmax_in[i] < 0 || lmax_in[i] > 30) {
          throw Exception ("lmaxes should be even and between 0 and 30");
        }
        lmax[i] = lmax_in[i];
      }
    } else {
      throw Exception ("number of lmaxes does not match number of response functions");
    }
  }

  /* make sure responses abide to the lmaxes */
  size_t sumnparams = 0;
  std::vector<size_t> nparams;
  for(size_t i = 0; i < lmax.size(); i++) {
    nparams.push_back(Math::SH::NforL(lmax[i]));
    sumnparams+=Math::SH::NforL(lmax[i]);
    response[i].resize(response[i].rows(),lmax[i]/2+1);
  }
  
  /* HR dirs option */
  Math::Matrix<value_type> HR_dirs;
  DWI::Directions::electrostatic_repulsion_300(HR_dirs);
  opt = get_options ("directions");
  if (opt.size())
    HR_dirs.load (opt[0][0]);
  
  /* precalculate everything */
  Shared shared (lmax,response,grad,HR_dirs);
  
  /* create scratch buffer for output */  
  Image::Header header (dwi_in_buffer); 
  header.set_ndim (4); 
  header.dim (3) = sumnparams;
  header.datatype() = DataType::Float32;
  BufferScratchType scratch_buffer (header);
  auto scratch_vox = scratch_buffer.voxel();
  
  /* do the actual work */
  Image::ThreadedLoop loop ("working...", dwi_in_vox, 0, 3);
  Processor processor (shared, dwi_in_vox, mask_in_vox, scratch_vox);
  Timer timer;
  loop.run (processor);
  VAR (timer.elapsed());
 
  /* copy from scratch buffer to output buffers */
  std::vector<ssize_t> from (4, 0);
  std::vector<ssize_t> to = { scratch_vox.dim(0), scratch_vox.dim(1), scratch_vox.dim(2), 0 };

   for (size_t i = 0; i < (argument.size()-1)/2; ++i) {
    header.dim (3) = nparams[i];
    OutputBufferType out_buffer (argument[(i+1)*2], header);
    to[3] = nparams[i];
    Image::copy (Image::Adapter::Subset<decltype(scratch_vox)> (scratch_vox, from, to), out_buffer.voxel());
    from[3]+= nparams[i];
  }
  
}
