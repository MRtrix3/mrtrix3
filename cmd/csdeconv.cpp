#include "app.h"
#include "ptr.h"
#include "progressbar.h"
#include "thread/exec.h"
#include "thread/queue.h"
#include "image/loop.h"
#include "image/data.h"
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
    + DWI::CSD_options;
}







typedef float value_type;

class Item
{
  public:
    Math::Vector<value_type> data;
    ssize_t pos[3];
};





class DataLoader
{
  public:
    DataLoader (Image::Data<value_type>& dwi_data,
                Image::Data<bool>* mask_data,
                const std::vector<int>& vec_bzeros,
                const std::vector<int>& vec_dwis,
                bool normalise_to_b0) :
      dwi (dwi_data),
      bzeros (vec_bzeros),
      dwis (vec_dwis),
      loop ("performing constrained spherical deconvolution...", 0, 3),
      normalise (normalise_to_b0) {
        if (mask_data) {
          Image::check_dimensions (*mask_data, dwi, 0, 3);
          mask = new Image::Data<bool>::voxel_type (*mask_data);
          loop.start (*mask, dwi);
        }
        else
          loop.start (dwi);
      }

    bool operator() (Item& item) {
      if (loop.ok()) {
        if (mask) {
          while (!mask->value()) {
            loop.next (*mask, dwi);
            if (!loop.ok())
              return false;
          }
        }

        load (item);

        if (mask)
          loop.next (*mask, dwi);
        else
          loop.next (dwi);

        return true;
      }
      return false;
    }

  private:
    Image::Data<value_type>::voxel_type dwi;
    Ptr<Image::Data<bool>::voxel_type> mask;
    const std::vector<int>&  bzeros;
    const std::vector<int>&  dwis;
    Image::Loop loop;
    bool  normalise;

    void load (Item& item) {
      value_type norm = 0.0;
      if (normalise) {
        for (size_t n = 0; n < bzeros.size(); n++) {
          dwi[3] = bzeros[n];
          norm += dwi.value ();
        }
        norm /= bzeros.size();
      }

      item.data.resize (dwis.size());
      for (size_t n = 0; n < dwis.size(); n++) {
        dwi[3] = dwis[n];
        item.data[n] = dwi.value();
        if (!finite (item.data[n])) return;
        if (item.data[n] < 0.0) item.data[n] = 0.0;
        if (normalise) item.data[n] /= norm;
      }

      item.pos[0] = dwi[0];
      item.pos[1] = dwi[1];
      item.pos[2] = dwi[2];
    }
};





class Processor
{
  public:
    Processor (Image::Data<value_type>& SH_data,
               const DWI::CSDeconv<value_type>::Shared& shared) :
      SH (SH_data),
      sdeconv (shared) { }

    bool operator() (const Item& item) {
      sdeconv.set (item.data);

      size_t n;
      for (n = 0; n < sdeconv.P.niter; n++)
        if (sdeconv.iterate())
          break;

      if (n == sdeconv.P.niter)
        info ("voxel [ " + str (item.pos[0]) + " " + str (item.pos[1]) + " " + str (item.pos[2]) +
            " ] did not reach full convergence");

      SH[0] = item.pos[0];
      SH[1] = item.pos[1];
      SH[2] = item.pos[2];

      for (SH[3] = 0; SH[3] < SH.dim (3); ++SH[3])
        SH.value() = sdeconv.FOD() [SH[3]];

      return true;
    }

  private:
    Image::Data<value_type>::voxel_type SH;
    DWI::CSDeconv<value_type> sdeconv;
};








void run ()
{
  Image::Data<value_type> dwi_data (argument[0]);

  Options opt = get_options ("mask");
  Ptr<Image::Data<bool> > mask_data;
  if (opt.size()) 
    mask_data = new Image::Data<bool> (opt[0][0]);

  DWI::CSDeconv<value_type>::Shared shared (dwi_data, argument[1]);

  bool normalise = get_options ("normalise").size();

  Image::Header header (dwi_data);
  header.dim(3) = shared.nSH();
  header.datatype() = DataType::Float32;
  header.stride(0) = 2;
  header.stride(1) = 3;
  header.stride(2) = 4;
  header.stride(3) = 1;
  Image::Data<value_type> SH_data (header, argument[2]);

  DataLoader loader (dwi_data, mask_data, shared.bzeros, shared.dwis, normalise);
  Processor processor (SH_data, shared);

  Thread::run_queue (loader, 1, Item(), processor, 0);
}

