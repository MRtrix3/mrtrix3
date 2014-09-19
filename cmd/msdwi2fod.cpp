#include "command.h"
#include "ptr.h"
#include "progressbar.h"
#include "thread/exec.h"
#include "thread/queue.h"
#include "image/copy.h"
#include "image/loop.h"
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

  REFERENCES = "Jeurissen, B.; Tournier, J.-D.; Dhollander, T.; Connelly, A.; Sijbers, J."
    "Multi-tissue constrained spherical deconvolution for improved analysis of multi-shell diffusion MRI data"
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
    + Argument ("order").type_integer (0, 8, 30).allow_multiple();
}

typedef double value_type;
typedef Image::BufferPreload<value_type> InputBufferType;
typedef Image::Buffer<bool> MaskBufferType;
typedef Image::Buffer<value_type> OutputBufferType;
typedef Image::BufferScratch<value_type> BufferScratchType;


class Shared {
  public:
    Shared (std::vector<size_t>& lmax_, std::vector<Math::Matrix<value_type> >& response_, Math::Matrix<value_type>& grad_) :
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
    size_t maxlmax = 0;
    for(std::vector<size_t>::iterator it = lmax.begin(); it != lmax.end(); ++it) {
      nparams+=Math::SH::NforL(*it);
      if (*it > maxlmax)
        maxlmax = *it;
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

    Math::Matrix<value_type> HR_dirs;
    DWI::Directions::electrostatic_repulsion_300(HR_dirs);
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
    //A.save("A.txt",16);
    //C.save("C.txt",16);
    problem = Math::ICLS::Problem<value_type> (C, A);
  };

  public:
    std::vector<size_t> lmax;
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
      try {
        solver (fodf, dwi);
      } 
      catch (Exception& E) {
        INFO ("failed to converge for voxel " + str(pos));
        //dwi.save("d" + str(pos) + ".txt",16);
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
  std::cout << argument.size() << std::endl;
  std::cout << (argument.size()-1)/2 << std::endl;
  
  /* input DWI image */
  InputBufferType dwi_in_buffer (argument[0], Image::Stride::contiguous_along_axis(3));
  InputBufferType::voxel_type dwi_in_vox (dwi_in_buffer);
  
  /* input response functions */
  std::vector<size_t> lmax_response;
  std::vector<Math::Matrix<value_type> > response;
  for (size_t i = 0; i < (argument.size()-1)/2; i++) {
    Math::Matrix<value_type> r(argument[i*2+1]);
    response.push_back(r);
    lmax_response.push_back((r.columns()-1)*2);
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

  /* gradient directions from header */
  auto grad = DWI::get_valid_DW_scheme<value_type> (dwi_in_buffer);

  /* for now,  lmaxes are taken from the response files, still take into account lmax option */
  /*int lmax_ = 8;
  opt = get_options ("lmax");
  if (opt.size())
    lmax_ = opt[0][0];
  std::vector<size_t> lmax;
  lmax.push_back(0);
  lmax.push_back(0);
  lmax.push_back(lmax_);*/
  std::vector<size_t> lmax = lmax_response;

  /* make sure responses abide to the lmaxes */
  size_t sumnparams = 0;
  std::vector<size_t> nparams;
  for(size_t i = 0; i < lmax.size(); i++) {
    nparams.push_back(Math::SH::NforL(lmax[i]));
    sumnparams+=Math::SH::NforL(lmax[i]);
    response[i].resize(response[i].rows(),lmax[i]/2+1);
  }

  /* precalculate everything */
  Shared shared (lmax,response,grad);
  
  /* create scratch buffer for output */  
  Image::Header header (dwi_in_buffer); 
  header.set_ndim (4); 
  header.dim (3) = sumnparams;
  header.datatype() = DataType::Float32;
  BufferScratchType scratch_buffer (header);
  auto scratch_vox = scratch_buffer.voxel();
  
  /* do the actual work */
  Image::ThreadedLoop loop ("working...", dwi_in_vox, 1, 0, 3);
  Processor processor (shared, dwi_in_vox, mask_in_vox, scratch_vox);
  loop.run (processor);
 
  /* copy from scratch buffer to output files */
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
