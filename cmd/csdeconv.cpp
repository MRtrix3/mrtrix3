/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "app.h"
#include "ptr.h"
#include "progressbar.h"
#include "thread/exec.h"
#include "thread/queue.h"
#include "dataset/loop.h"
#include "image/voxel.h"
#include "dwi/gradient.h"
#include "dwi/sdeconv/constrained.h"

using namespace MR; 

SET_VERSION_DEFAULT;

DESCRIPTION = {
  "perform non-negativity constrained spherical deconvolution.",

  "Note that this program makes use of implied symmetries in the diffusion profile. First, the fact the signal attenuation profile is real implies that it has conjugate symmetry, i.e. Y(l,-m) = Y(l,m)* (where * denotes the complex conjugate). Second, the diffusion profile should be antipodally symmetric (i.e. S(x) = S(-x)), implying that all odd l components should be zero. Therefore, this program only computes the even elements.",

  "Note that the spherical harmonics equations used here differ slightly from those conventionally used, in that the (-1)^m factor has been omitted. This should be taken into account in all subsequent calculations.",

  "Each volume in the output image corresponds to a different spherical harmonic component, according to the following convention:\n"
    "[0]     Y(0,0)\n"
    "\n"
    "[1] Im {Y(2,2)}\n"
    "[2] Im {Y(2,1)}\n"
    "[3]     Y(2,0)\n"
    "[4] Re {Y(2,1)}\n"
    "[5] Re {Y(2,2)}\n"
    "\n"
    "[6] Im {Y(4,4)}\n"
    "[7] Im {Y(4,3)}\n"
    "etc...\n",

  NULL
};

ARGUMENTS = {
  Argument ("dwi", "input DW image", "the input diffusion-weighted image.").type_image_in (),
  Argument ("response", "response function", "the diffusion-weighted signal response function for a single fibre population.").type_file (),
  Argument ("SH", "output SH image", "the output spherical harmonics coefficients image.").type_image_out (),
  Argument::End
};


OPTIONS = { 
  Option ("grad", "supply gradient encoding", "specify the diffusion-weighted gradient scheme used in the acquisition. The program will normally attempt to use the encoding stored in image header.")
    .append (Argument ("encoding", "gradient encoding", "the gradient encoding, supplied as a 4xN text file with each line is in the format [ X Y Z b ], where [ X Y Z ] describe the direction of the applied gradient, and b gives the b-value in units (1000 s/mm^2).").type_file ()),

  Option ("lmax", "maximum harmonic order", "set the maximum harmonic order for the output series. By default, the program will use the highest possible lmax given the number of diffusion-weighted images.")
    .append (Argument ("order", "order", "the maximum harmonic order to use.").type_integer (2, 30, 8)),

  Option ("mask", "brain mask", "only perform computation within the specified binary brain mask image.")
    .append (Argument ("image", "image", "the mask image to use.").type_image_in ()),

  Option ("directions", "direction set for constraint", "specify the directions over which to apply the non-negativity constraint (by default, the built-in 300 direction set is used)")
    .append (Argument ("file", "file", "a text file containing the [ el az ] pairs for the directions.").type_file ()),

  Option ("filter", "initial angular frequency filter", "the linear frequency filtering parameters used for the initial linear spherical deconvolution step (default = [ 1 1 1 0 0 ]).")
    .append (Argument ("spec", "specification", "a text file containing the filtering coefficients for each even harmonic order.").type_file ()),

  Option ("normalise", "normalise to b=0", "normalise the DW signal to the b=0 image"),

  Option ("neg_lambda", "neg_lambda", "the regularisation parameter lambda that controls the strength of the non-negativity constraint (default = 1.0).")
    .append (Argument ("value", "value", "the value of lambda to use.").type_float (0.0, 1.0e12, 1.0)),

  Option ("norm_lambda", "norm_lambda", "the regularisation parameter lambda that controls the strength of the constraint on the norm of the solution (default = 1.0).")
    .append (Argument ("value", "value", "the value of lambda to use.").type_float (0.0, 1.0e12, 1.0)),

  Option ("threshold", "amplitude threshold", "the threshold below which the amplitude of the FOD is assumed to be zero, expressed as a fraction of the mean value of the initial FOD (default = 0.1).")
    .append (Argument ("value", "value", "the value of lambda to use.").type_float (-1.0, 2.0, 0.1)),

  Option ("niter", "maximum number of iterations", "the maximum number of iterations to perform for each voxel (default = 50).")
    .append (Argument ("number", "number", "the maximum number of iterations to use.").type_integer (1, 1000, 50)),

  Option::End 
};


typedef float value_type;

class Item {
  public:
    Math::Vector<value_type> data;
    ssize_t pos[3];
};

class Allocator {
  public:
    Allocator (size_t data_size) : N (data_size) { }
    Item* alloc () { Item* item = new Item; item->data.allocate (N); return (item); }
    void reset (Item* item) { }
    void dealloc (Item* item) { delete item; }
  private:
    size_t N;
};


typedef Thread::Queue<Item,Allocator> Queue;





class DataLoader {
  public:
    DataLoader (Queue& queue, 
        const Image::Header& dwi_header, 
        const Image::Header* mask_header,
        const std::vector<int>& vec_bzeros,
        const std::vector<int>& vec_dwis,
        bool normalise_to_b0) : 
      writer (queue), 
      dwi (dwi_header), 
      mask (mask_header),
      bzeros (vec_bzeros),
      dwis (vec_dwis),
      normalise (normalise_to_b0) { }

    void execute () {
      Exec exec (*this);
      if (mask) {
        Image::Voxel<value_type> mask_vox (*mask);
        DataSet::loop1_mask ("performing constrained spherical deconvolution...", exec, mask_vox, dwi);
      }
      else DataSet::loop1 ("performing constrained spherical deconvolution...", exec, dwi);
    }

  private:
    Queue::Writer writer;
    Image::Voxel<value_type>  dwi;
    const Image::Header* mask;
    const std::vector<int>&  bzeros;
    const std::vector<int>&  dwis;
    bool  normalise;

    class Exec {
      public:
        Exec (DataLoader& parent) : P (parent), item (P.writer) { }
        void operator() (Image::Voxel<value_type>& D) { 
          value_type norm = 0.0;
          if (P.normalise) {
            for (size_t n = 0; n < P.bzeros.size(); n++) {
              D[3] = P.bzeros[n];
              norm += D.value ();
            }
            norm /= P.bzeros.size();
          }

          for (size_t n = 0; n < P.dwis.size(); n++) {
            D[3] = P.dwis[n];
            item->data[n] = D.value(); 
            if (!finite (item->data[n])) return;
            if (item->data[n] < 0.0) item->data[n] = 0.0;
            if (P.normalise) item->data[n] /= norm;
          }

          item->pos[0] = D[0];
          item->pos[1] = D[1];
          item->pos[2] = D[2];

          if (!item.write()) throw Exception ("error writing to work queue");
        }
      private:
        DataLoader& P;
        Queue::Writer::Item item;
    };
    friend class Exec;
};





class Processor {
  public:
    Processor (Queue& queue, 
        const Image::Header& header, 
        const DWI::CSDeconv<value_type>::Shared& shared, 
        int max_num_iterations) :
      reader (queue), SH (header), sdeconv (shared), niter (max_num_iterations) { }

    void execute () {
      Queue::Reader::Item item (reader);
      while (item.read()) {
        sdeconv.set (item->data);

        int n;
        for (n = 0; n < niter; n++) if (sdeconv.iterate()) break;
        if (n == niter) error ("voxel [ " + 
            str(item->pos[0]) + " " + str(item->pos[1]) + " " + str(item->pos[2]) +
            " ] failed to converge"); 

        SH[0] = item->pos[0];
        SH[1] = item->pos[1];
        SH[2] = item->pos[2];

        for (SH[3] = 0; SH[3] < SH.dim(3); ++SH[3])
          SH.value() = sdeconv.FOD()[SH[3]];
      }
    }

  private:
    Queue::Reader reader;
    Image::Voxel<value_type> SH;
    DWI::CSDeconv<value_type> sdeconv;
    int niter;
};







extern value_type default_directions [];

EXECUTE {
  const Image::Header dwi_header = argument[0].get_image();
  Image::Header header (dwi_header);

  if (header.ndim() != 4) 
    throw Exception ("dwi image should contain 4 dimensions");

  Math::Matrix<float> grad;

  std::vector<OptBase> opt = get_options (0);
  if (opt.size()) grad.load (opt[0][0].get_string());
  else {
    if (!header.DW_scheme.is_set()) 
      throw Exception ("no diffusion encoding found in image \"" + header.name() + "\"");
    grad.copy (header.DW_scheme);
  }

  if (grad.rows() < 7 || grad.columns() != 4) 
    throw Exception ("unexpected diffusion encoding matrix dimensions");

  info ("found " + str(grad.rows()) + "x" + str(grad.columns()) + " diffusion-weighted encoding");

  if (header.dim(3) != (int) grad.rows()) 
    throw Exception ("number of studies in base image does not match that in encoding file");

  DWI::normalise_grad (grad);
  if (grad.rows() < 7 || grad.columns() != 4) 
    throw Exception ("unexpected diffusion encoding matrix dimensions");

  std::vector<int> bzeros, dwis;
  DWI::guess_DW_directions (dwis, bzeros, grad);
  info ("found " + str(dwis.size()) + " diffusion-weighted directions");

  Math::Matrix<float> DW_dirs;
  DWI::gen_direction_matrix (DW_dirs, grad, dwis);

  opt = get_options (1);
  int lmax = opt.size() ? opt[0][0].get_int() : Math::SH::LforN (dwis.size());
  info ("calculating even spherical harmonic components up to order " + str(lmax));


  info (std::string ("setting response function from file \"") + argument[1].get_string() + "\"");
  Math::Vector<float> response;
  response.load (argument[1].get_string());
  info ("setting response function using even SH coefficients: " + str(response));


  opt = get_options (4); // filter
  Math::Vector<float> filter;
  if (opt.size()) filter.load (opt[0][0].get_string());
  else {
    filter.allocate (response.size());
    filter.zero();
    filter[0] = filter[1] = filter[2] = 1.0;
  }
  info ("using initial filter coefficients: " + str(filter));



  opt = get_options (3); // directions
  Math::Matrix<float> HR_dirs;
  if (opt.size()) HR_dirs.load (opt[0][0].get_string());
  else {
    HR_dirs.allocate (300,2);
    HR_dirs = Math::Matrix<float> (default_directions, 300, 2);
  }

  header.axes.dim(3) = Math::SH::NforL (lmax);
  header.datatype() = DataType::Float32;
  header.axes.stride(0) = 2;
  header.axes.stride(1) = 3;
  header.axes.stride(2) = 4;
  header.axes.stride(3) = 1;

  const Image::Header* mask_header = NULL;
  opt = get_options (2); // mask
  if (opt.size())
    mask_header = new Image::Header (opt[0][0].get_image());

  bool normalise = get_options(5).size();

  opt = get_options(6); // neg_lambda
  float neg_lambda = 1.0;
  if (opt.size()) neg_lambda = opt[0][0].get_float();

  opt = get_options(7); // norm_lambda
  float norm_lambda = 1.0;
  if (opt.size()) norm_lambda = opt[0][0].get_float();

  opt = get_options(8); // threshold
  float threshold = 0.1;
  if (opt.size()) threshold = opt[0][0].get_float();

  opt = get_options(9); // niter
  int niter = 50;
  if (opt.size()) niter = opt[0][0].get_int();


  const Image::Header SH_header (argument[2].get_image (header));

  Queue queue ("work queue", 100, Allocator (dwis.size()));
  DataLoader loader (queue, dwi_header, mask_header, bzeros, dwis, normalise);

  DWI::CSDeconv<value_type>::Shared shared (response, filter, DW_dirs, HR_dirs, lmax);
  shared.neg_lambda = neg_lambda;
  shared.norm_lambda = norm_lambda;

  Processor processor (queue, SH_header, shared, niter);

  Thread::Exec loader_thread (loader, "loader");
  Thread::Array<Processor> processor_list (processor);
  Thread::Exec processor_threads (processor_list, "processor");
}




value_type default_directions [] = {
  0, 0,
  -3.14159, 1.58657,
  2.36695, 1.25853,
  3.08672, 0.558239,
  -1.67401, 0.706981,
  0.114035, 1.45549,
  2.78305, 1.27687,
  3.03488, 1.47933,
  2.46335, 0.305006,
  1.64402, 1.09622,
  -1.34645, 1.2481,
  1.79761, 1.62693,
  -0.88414, 1.51834,
  1.64358, 1.36022,
  -2.7066, 1.01956,
  0.329298, 0.454666,
  -0.920565, 1.13991,
  -1.33257, 0.556134,
  0.892596, 1.23279,
  -0.787592, 0.303143,
  -2.09259, 1.13147,
  -0.457719, 1.03869,
  0.371139, 1.41966,
  2.20967, 1.22547,
  2.53978, 0.606163,
  -0.412995, 0.388528,
  0.725018, 1.49152,
  2.70237, 1.7158,
  -0.808986, 1.0254,
  -1.59766, 1.12722,
  2.50935, 0.455242,
  0.731963, 1.23016,
  0.655017, 0.959021,
  -2.24111, 0.85497,
  2.10638, 1.36019,
  -0.846961, 1.27488,
  3.06335, 1.31095,
  1.44333, 0.758957,
  1.54644, 0.890203,
  -0.514666, 1.29491,
  1.567, 1.49187,
  1.69346, 0.552006,
  0.209201, 1.20641,
  0.487422, 1.03363,
  2.63899, 1.21178,
  -0.632554, 1.01363,
  1.77086, 0.405347,
  -2.76517, 1.1624,
  1.23671, 1.24714,
  1.93834, 0.264452,
  2.9235, 1.37573,
  -1.11188, 1.38166,
  2.04833, 1.21874,
  -2.91953, 0.490257,
  -1.19098, 1.51361,
  1.08017, 0.809038,
  -0.289967, 1.46451,
  -3.07471, 1.25229,
  0.635727, 1.10896,
  -0.867147, 0.882687,
  1.05149, 1.49091,
  -2.59911, 1.1376,
  -1.46509, 0.845243,
  -1.84833, 0.914075,
  2.44675, 1.00828,
  -1.31564, 0.279639,
  -0.784998, 0.604968,
  2.44502, 1.60297,
  2.9443, 0.770919,
  1.27673, 0.848176,
  2.77642, 1.12699,
  -2.50265, 1.25937,
  1.3869, 0.971908,
  1.41163, 0.305817,
  -1.65286, 1.51973,
  -1.66588, 0.857793,
  1.98611, 0.541404,
  -2.96427, 1.36126,
  -0.398669, 1.57005,
  -1.17166, 0.423278,
  2.3223, 1.11328,
  -2.22828, 0.421751,
  1.41185, 1.49157,
  -2.34795, 0.988783,
  -0.012617, 0.821608,
  -1.26243, 0.838351,
  -0.0307006, 1.40395,
  1.78894, 0.818245,
  1.02549, 0.954998,
  -2.74372, 1.57328,
  -1.98986, 1.00949,
  -0.331703, 1.31812,
  -1.54712, 0.41071,
  2.40483, 0.857164,
  0.318639, 0.606137,
  -2.26067, 1.12127,
  0.158211, 0.899318,
  -0.59185, 1.42922,
  1.64999, 0.700746,
  1.56437, 1.22914,
  0.343295, 1.11648,
  -1.09466, 1.11597,
  -1.5252, 0.989022,
  -1.26681, 1.38045,
  -2.97413, 0.803613,
  2.35281, 0.707799,
  2.27139, 0.966953,
  -2.5672, 1.39744,
  -1.92736, 1.63394,
  3.0206, 0.410937,
  2.15582, 1.0832,
  1.0738, 0.405277,
  1.99792, 0.809703,
  -2.55141, 0.726527,
  -3.10848, 1.41658,
  1.14688, 1.37171,
  2.15735, 0.412301,
  1.41304, 0.154255,
  0.492675, 0.713364,
  2.79673, 0.525514,
  0.317456, 0.806968,
  -2.02289, 1.51574,
  -2.34343, 1.51722,
  2.17796, 1.49481,
  0.14538, 0.714472,
  2.45229, 0.151727,
  -2.18285, 1.25715,
  -2.85991, 0.340472,
  0.0221302, 1.00201,
  -2.58492, 0.449267,
  -2.39608, 0.281522,
  -1.92714, 1.14914,
  1.05243, 1.2507,
  2.20621, 0.822358,
  -1.02782, 1.2532,
  -2.81677, 1.31031,
  2.08749, 0.944097,
  -2.52015, 1.54051,
  -0.777557, 1.41041,
  0.664196, 0.807484,
  2.76519, 0.676192,
  -0.98491, 0.999267,
  -0.0815636, 0.675896,
  -0.613748, 0.863828,
  0.872396, 0.794462,
  2.77992, 1.42651,
  -1.81878, 1.52819,
  -1.83993, 1.27827,
  2.99836, 1.63509,
  -3.0493, 1.10033,
  -1.57592, 1.38923,
  -1.66501, 1.26585,
  1.98475, 1.07821,
  -0.758846, 0.755449,
  -0.322937, 0.662737,
  -0.219022, 1.21251,
  -0.972796, 0.708713,
  -0.570821, 1.15401,
  -1.91417, 1.41057,
  2.62874, 1.41065,
  -1.03783, 1.51449,
  0.974077, 1.10049,
  0.346323, 0.303114,
  -1.99892, 1.28331,
  2.12168, 0.681107,
  -2.76977, 0.758155,
  2.7664, 0.977251,
  -0.820578, 0.454358,
  0.839928, 0.94433,
  -2.8299, 0.907575,
  -2.86063, 1.47417,
  -2.88221, 1.05282,
  -2.10758, 1.38992,
  -2.66213, 1.27831,
  1.88682, 1.22292,
  -2.67459, 0.594643,
  1.23323, 0.69871,
  -0.56359, 0.677916,
  0.498283, 1.3351,
  3.09525, 1.00711,
  -0.185631, 0.255631,
  2.59167, 1.56346,
  -2.2613, 1.38896,
  2.98466, 0.261101,
  3.07908, 1.15872,
  0.804431, 1.09332,
  2.5158, 1.30725,
  -2.88762, 1.62499,
  -1.04846, 0.563671,
  -0.144761, 0.940433,
  0.805734, 1.36272,
  -2.16743, 0.993861,
  -1.76262, 1.14622,
  2.26017, 1.36782,
  1.7209, 1.49258,
  2.02607, 1.49512,
  -0.179351, 1.35909,
  -2.4419, 0.856827,
  2.48601, 1.15624,
  1.71559, 0.958417,
  0.328795, 0.962895,
  1.79828, 1.36176,
  -2.43004, 1.12329,
  0.578222, 0.549641,
  -1.44111, 0.694471,
  0.492112, 1.18461,
  2.93617, 1.07324,
  -1.20507, 0.690519,
  -2.34227, 1.25185,
  1.20477, 0.986449,
  -0.218792, 0.798903,
  1.47524, 1.10228,
  0.376913, 0.151281,
  -0.0144004, 0.398387,
  -2.04103, 0.86384,
  2.89068, 1.5293,
  -0.286741, 1.07479,
  -0.115302, 1.09485,
  -0.955036, 1.38575,
  0.524931, 1.48422,
  0.996816, 0.666425,
  1.14017, 1.12514,
  -1.43053, 1.11697,
  1.88304, 0.680225,
  2.56481, 0.757686,
  -1.88134, 0.766008,
  -2.52854, 0.998159,
  -2.63927, 0.875863,
  -3.00297, 1.52622,
  1.42464, 0.456914,
  0.18689, 1.05545,
  2.93223, 1.22441,
  0.0620309, 0.54712,
  2.27644, 0.557988,
  -1.76897, 0.162275,
  -1.16344, 0.975942,
  -0.064867, 1.25454,
  -1.06028, 0.847707,
  0.23085, 1.35764,
  0.882646, 0.265951,
  -2.92083, 0.641964,
  2.58797, 0.911683,
  0.49056, 0.883188,
  -2.9243, 1.19959,
  -1.87051, 0.615755,
  0.875184, 0.529991,
  -1.61814, 0.558647,
  -2.09273, 0.715817,
  -1.26264, 1.11158,
  1.17269, 0.550658,
  0.0546909, 1.1517,
  1.30702, 1.10955,
  -2.32322, 0.715657,
  -1.34511, 0.976629,
  -1.5059, 1.25321,
  2.94014, 0.922056,
  -0.538119, 0.52844,
  -2.4151, 1.38489,
  1.48798, 1.36106,
  1.43478, 0.607961,
  -2.17608, 1.52403,
  -1.18696, 1.24717,
  1.81389, 1.08593,
  -1.42162, 1.38294,
  0.680987, 0.404254,
  2.75128, 0.82776,
  0.357011, 1.26778,
  -1.74274, 1.39703,
  0.650615, 1.35971,
  -0.405324, 1.18319,
  0.0853527, 1.30274,
  1.39741, 1.23707,
  -1.88539, 0.464434,
  0.749078, 0.666178,
  -0.75793, 0.151623,
  2.48357, 1.45565,
  -2.77281, 0.151251,
  -2.12502, 0.565067,
  1.90056, 0.943973,
  -2.71678, 1.42485,
  2.61809, 1.06125,
  -0.73739, 1.16199,
  -1.70291, 1.00579,
  -1.88046, 0.312315,
  2.33806, 1.49648,
  3.11653, 0.85627,
  -3.13943, 0.704778,
  -0.232477, 0.519107,
  -0.429576, 0.798159,
  -0.332043, 0.928957,
  0.959616, 1.37061,
  1.3044, 1.38372,
  -2.40422, 0.572587,
  1.87378, 1.49472,
  -3.01602, 0.951485,
  1.6447, 1.62331,
  -0.671971, 1.29962,
  1.7256, 1.22764,
  0.876701, 1.49621,
  1.95232, 1.36154
};
