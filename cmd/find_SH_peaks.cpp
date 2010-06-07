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
#include "progressbar.h"
#include "image/thread_voxelwise.h"
#include "image/voxel.h"
#include "math/SH.h"

#define DOT_THRESHOLD 0.99

using namespace std; 
using namespace MR; 

SET_VERSION_DEFAULT;

DESCRIPTION = {
  "identify the orientations of the N largest peaks of a SH profile",
  NULL
};

ARGUMENTS = {
  Argument ("SH", "SH coefficients image", "the input image of SH coefficients.").type_image_in (),
  Argument ("ouput", "output image", "the output image. Each volume corresponds to the x, y & z component of each peak direction vector in turn.").type_image_out (),
  Argument::End
};

OPTIONS = {
  Option ("num", "number of peaks", "the number of peaks to extract (default is 3).")
    .append (Argument ("peaks", "number", "the number of peaks").type_integer (0, INT_MAX, 3)),

  Option ("direction", "specify direction", "the direction of a peak to estimate. The algorithm will attempt to find the same number of peaks as have been specified using this option.", Optional | AllowMultiple)
    .append (Argument ("phi", "azimuthal angle", "the azimuthal angle of the direction (in degrees).").type_float (-INFINITY, INFINITY, 0.0))
    .append (Argument ("theta", "elevation angle", "the elevation angle of the direction (in degrees, from the vertical z-axis).").type_float (-INFINITY, INFINITY, 0.0)),

  Option ("peaks", "true peaks image", "the program will try to find the peaks that most closely match those in the image provided.")
    .append (Argument ("image", "peaks image", "an image containing the true peaks to be estimated.").type_image_in ()),

  Option ("threshold", "amplitude threshold", "only peak amplitudes greater than the threshold will be considered.")
    .append (Argument ("value", "value", "the threshold value").type_float (-INFINITY, INFINITY, 0.0)),

  Option ("seeds", "seed direction set for multiple restarts", "specify a set of directions from which to start the multiple restarts of the optimisation (by default, the built-in 60 direction set is used)")
    .append (Argument ("file", "file", "a text file containing the [ el az ] pairs for the directions.").type_file ()),

  Option ("mask", "brain mask", "only perform computation within the specified binary brain mask image.")
    .append (Argument ("image", "image", "the mask image to use.").type_image_in ()),

  Option::End
};



class Direction {
  public:
    Direction () : a (NAN) { }
    Direction (const Direction& d) : a (d.a), v (d.v) { }
    Direction (float phi, float theta) : a (1.0), v (cos(phi)*sin(theta), sin(phi)*sin(theta), cos(theta)) { }
    float a;
    Point v;
    bool operator<(const Direction& d) const { return (a > d.a); }
};




template <typename T> class OrientationEstimator : public Image::ThreadVoxelWise
{
  public:
    OrientationEstimator (Image::Object& SH_object, RefPtr<Image::Voxel> mask_voxel) : 
      Image::ThreadVoxelWise (SH_object, mask_voxel),
      lmax (Math::SH::LforN (SH_object.dim(3))) { }
    ~OrientationEstimator () { }

    RefPtr<Image::Object> dirs_obj;
    Ptr<Image::Voxel> ipeaks;
    Math::Matrix<T> dirs;
    int   current[3];
    int npeaks, lmax;
    std::vector<Direction> true_peaks;
    T threshold;



    void execute (int thread_ID)
    {
      assert (dirs_obj);
      assert (dirs_obj->is_mapped());
      std::vector<Direction> peaks_out (npeaks);
      Image::Voxel SH (source);
      Image::Voxel out (*dirs_obj);
      T val [source.dim(3)];

      do {
        if (get_next (SH)) return;

        out[0] = SH[0];
        out[1] = SH[1];
        out[2] = SH[2];

        if (prepare_values (val, SH)) {
          for (out[3] = 0; out[3] < out.dim(3); out[3]++) 
            out.value() = NAN;
          continue;
        }


        std::vector<Direction> all_peaks;

        for (size_t i = 0; i < dirs.rows(); i++) {
          Direction p (dirs(i,0), dirs(i,1)); 
          p.a = Math::SH::get_peak (val, lmax, p.v);

          if (finite (p.a)) {
            for (size_t j = 0; j < all_peaks.size(); j++) {
              if (Math::abs (p.v.dot (all_peaks[j].v)) > DOT_THRESHOLD) {
                p.a = NAN;
                break;
              }
            }
          }
          if (finite (p.a) && p.a >= threshold) all_peaks.push_back (p);
        }

        if (ipeaks) {
          for (int i = 0; i < npeaks; i++) {
            Point p;
            (*ipeaks)[3] = 3*i;
            for (int n = 0; n < 3; n++) { p[n] = ipeaks->value(); (*ipeaks)[3]++; }
            p.normalise();

            float mdot = 0.0;
            for (size_t n = 0; n < all_peaks.size(); n++) {
              float f = Math::abs (p.dot (all_peaks[n].v));
              if (f > mdot) { 
                mdot = f; 
                peaks_out[i] = all_peaks[n];
              }
            }
          }
        }
        else if (true_peaks.size()) {
          for (int i = 0; i < npeaks; i++) {
            float mdot = 0.0;
            for (size_t n = 0; n < all_peaks.size(); n++) {
              float f = Math::abs (all_peaks[n].v.dot (true_peaks[i].v));
              if (f > mdot) { 
                mdot = f; 
                peaks_out[i] = all_peaks[n];
              }
            }
          }
        }
        else std::partial_sort_copy (all_peaks.begin(), all_peaks.end(), peaks_out.begin(), peaks_out.end());

        int actual_npeaks = MIN (npeaks, (int) all_peaks.size());
        out[3] = 0;
        for (int n = 0; n < actual_npeaks; n++) {
          out.value() = peaks_out[n].a*peaks_out[n].v[0]; out[3]++; 
          out.value() = peaks_out[n].a*peaks_out[n].v[1]; out[3]++; 
          out.value() = peaks_out[n].a*peaks_out[n].v[2]; out[3]++;
        }
        for (; out[3] < 3*npeaks; out[3]++) out.value() = NAN;


      } while (true);
    }



  protected:
    Mutex mutex;
    bool done;


    bool prepare_values (T* val, Image::Voxel& SH)
    {
      for (SH[3] = 0; SH[3] < source.dim(3); SH[3]++) 
        val[SH[3]] = SH.value();

      if (ipeaks) {
        (*ipeaks)[0] = SH[0];
        (*ipeaks)[1] = SH[1];
        (*ipeaks)[2] = SH[2];
        (*ipeaks)[3] = 0;
        if (isnan (ipeaks->value())) return (true);
      }

      bool no_peaks = true;
      for (int i = 0; i < source.dim(3); i++) {
        if (isnan (val[i])) return (true);
        if (no_peaks) 
          if (i && val[i] != 0.0) no_peaks = false;
      }

      return (no_peaks);
    }

};




extern float default_directions [];


EXECUTE {
  Image::Object& SH (*argument[0].get_image());

  std::vector<OptBase> opt = get_options (5); // mask
  RefPtr<Image::Voxel> mask;
  if (opt.size()) mask = new Image::Voxel (*opt[0][0].get_image());

  OrientationEstimator<float> estimator (SH, mask);

  // Load seed direction set:
  opt = get_options (4); // seeds
  if (opt.size()) estimator.dirs.load (opt[0][0].get_string());
  else {
    estimator.dirs.allocate (60,2);
    estimator.dirs.view() = Math::MatrixView<float> (default_directions, 60, 2);
  }
  if (estimator.dirs.columns() != 2) 
    throw Exception ("expecting 2 columns for search directions matrix");
  
  opt = get_options (0); // num
  estimator.npeaks = opt.size() ? opt[0][0].get_int() : 3;

  opt = get_options (1); // individual peaks 
  for (size_t n = 0; n < opt.size(); n++) {
    Direction p (M_PI*opt[n][0].get_float()/180.0, M_PI*opt[n][1].get_float()/180.0);
    estimator.true_peaks.push_back (p);
  }
  if (estimator.true_peaks.size()) estimator.npeaks = estimator.true_peaks.size();

  opt = get_options (3); // threshold
  estimator.threshold = -INFINITY;
  if (opt.size()) estimator.threshold = opt[0][0].get_float();

  Image::Header header (SH);

  header.data_type = DataType::Float32;
  header.axes.resize (4);

  opt = get_options (2); // peaks image
  if (opt.size()) {
    if (estimator.true_peaks.size()) throw Exception ("you can't specify both a peaks file and orientations to be estimated at the same time");
    estimator.ipeaks = new Image::Voxel (*opt[0][0].get_image());
    if (estimator.ipeaks->dim(0) != header.axes[0].dim || 
        estimator.ipeaks->dim(1) != header.axes[1].dim ||
        estimator.ipeaks->dim(2) != header.axes[2].dim)
      throw Exception ("dimensions of peaks image \"" + estimator.ipeaks->name() + "\" do not match that of SH coefficients image \"" + header.name + "\"");
    estimator.npeaks = estimator.ipeaks->dim(3) / 3;
    estimator.ipeaks->image.map();
  }

  header.axes[3].dim = 3 * estimator.npeaks;

  estimator.dirs_obj = argument[1].get_image (header);
  estimator.dirs_obj->map();

  info ("using lmax = " + str (estimator.lmax));

  estimator.run("finding orientations of largest peaks...");
}





float default_directions [] = {
  0, 0,
  -3.14159, 1.3254,
  -2.58185, 1.50789,
  2.23616, 1.46585,
  0.035637, 0.411961,
  2.65836, 0.913741,
  0.780743, 1.23955,
  -0.240253, 1.58088,
  -0.955334, 1.08447,
  1.12534, 1.78765,
  1.12689, 1.30126,
  0.88512, 1.55615,
  2.08019, 1.16222,
  0.191423, 1.06076,
  1.29453, 0.707568,
  2.794, 1.24245,
  2.02138, 0.337172,
  1.59186, 1.30164,
  -2.83601, 0.910221,
  0.569095, 0.96362,
  3.05336, 1.00206,
  2.4406, 1.19129,
  0.437969, 1.30795,
  0.247623, 0.728643,
  -0.193887, 1.0467,
  -1.34638, 1.14233,
  1.35977, 1.54693,
  1.82433, 0.660035,
  -0.766769, 1.3685,
  -2.02757, 1.02063,
  -0.78071, 0.667313,
  -1.47543, 1.45516,
  -1.10765, 1.38916,
  -1.65789, 0.871848,
  1.89902, 1.44647,
  3.08122, 0.336433,
  -2.35317, 1.25244,
  2.54757, 0.586206,
  -2.14697, 0.338323,
  3.10764, 0.670594,
  1.75238, 0.991972,
  -1.21593, 0.82585,
  -0.259942, 0.71572,
  -1.51829, 0.549286,
  2.22968, 0.851973,
  0.979108, 0.954864,
  1.36274, 1.04186,
  -0.0104792, 1.33716,
  -0.891568, 0.33526,
  -2.0635, 0.68273,
  -2.41353, 0.917031,
  2.57199, 1.50166,
  0.965936, 0.33624,
  0.763244, 0.657346,
  -2.61583, 0.606725,
  -0.429332, 1.30226,
  -2.91118, 1.56901,
  -2.79822, 1.24559,
  -1.70453, 1.20406,
  -0.582782, 0.975235
};

