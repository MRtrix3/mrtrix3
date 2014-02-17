#include <map>

#include "command.h"
#include "progressbar.h"
#include "image/buffer.h"
#include "image/voxel.h"
#include "image/loop.h"
#include "image/histogram.h"
#include "image/filter/optimal_threshold.h"


using namespace MR;
using namespace App;

void usage ()
{
  DESCRIPTION
  + "create bitwise image by thresholding image intensity. By default, an "
    "optimal threshold is determined using the parameter free method "
    "described in Ridgway G et al. (2009) NeuroImage.44(1):99-111. "
    "Alternatively the threshold can be defined manually by the user "
    "or using a histogram-based analysis to cut out the background.";

  ARGUMENTS
  + Argument ("input", "the input image to be thresholded.").type_image_in ()
  + Argument ("output", "the output binary image mask.").type_image_out ();


  OPTIONS
  + Option ("abs", "specify threshold value as absolute intensity.")
  + Argument ("value").type_float()

  + Option ("histogram", "define the threshold by a histogram analysis to cut out the background. "
                         "Note that only the first study is used for thresholding.")

  + Option ("percentile", "threshold the image at the ith percentile.")
  + Argument ("value").type_float (0.0, 95.0, 100.0)

  + Option ("top", "provide a mask of the N top-valued voxels")
  + Argument ("N").type_integer (0, 100, std::numeric_limits<int>::max())

  + Option ("bottom", "provide a mask of the N bottom-valued voxels")
  + Argument ("N").type_integer (0, 100, std::numeric_limits<int>::max())

  + Option ("invert", "invert output binary mask.")

  + Option ("toppercent", "provide a mask of the N%% top-valued voxels")
  + Argument ("N").type_integer (0, 100, std::numeric_limits<int>::max())

  + Option ("bottompercent", "provide a mask of the N%% bottom-valued voxels")
  + Argument ("N").type_integer (0, 100, std::numeric_limits<int>::max())

  + Option ("nan", "use NaN as the output zero value.")

  + Option ("ignorezero", "ignore zero-values input voxels.")

  + Option ("mask",
            "compute the optimal threshold based on voxels within a mask.")
  + Argument ("image").type_image_in ();
}


void run ()
{
  float threshold_value (NAN), percentile (NAN), bottomNpercent (NAN), topNpercent (NAN);
  bool use_histogram = false;
  size_t topN (0), bottomN (0), nopt (0);

  Options opt = get_options ("abs");
  if (opt.size()) {
    threshold_value = opt[0][0];
    ++nopt;
  }

  opt = get_options ("histogram");
  if (opt.size()) {
    use_histogram = true;
    ++nopt;
  }

  opt = get_options ("percentile");
  if (opt.size()) {
    percentile = opt[0][0];
    ++nopt;
  }

  opt = get_options ("top");
  if (opt.size()) {
    topN = opt[0][0];
    ++nopt;
  }

  opt = get_options ("bottom");
  if (opt.size()) {
    bottomN = opt[0][0];
    ++nopt;
  }

  opt = get_options ("toppercent");
  if (opt.size()) {
    topNpercent = opt[0][0];
    ++nopt;
  }

  opt = get_options ("bottompercent");
  if (opt.size()) {
    bottomNpercent = opt[0][0];
    ++nopt;
  }

  if (nopt > 1)
    throw Exception ("too many conflicting options");


  bool invert = get_options ("invert").size();
  bool use_NaN = get_options ("nan").size();
  bool ignore_zeroes = get_options ("ignorezero").size();

  Image::Buffer<float> data_in (argument[0]);
  assert (!data_in.datatype().is_complex());

  if (Image::voxel_count (data_in) < topN || Image::voxel_count (data_in) < bottomN)
    throw Exception ("number of voxels at which to threshold exceeds number of voxels in image");

  if (std::isfinite (percentile)) {
    percentile /= 100.0;
    if (percentile < 0.5) {
      bottomN = Math::round (Image::voxel_count (data_in) * percentile);
      invert = !invert;
    }
    else topN = Math::round (Image::voxel_count (data_in) * (1.0 - percentile));
  }

  Image::Header header_out (data_in);
  header_out.datatype() = use_NaN ? DataType::Float32 : DataType::Bit;

  Image::Buffer<float>::voxel_type in (data_in);

  Image::Buffer<float> data_out (argument[1], header_out);
  Image::Buffer<float>::voxel_type out (data_out);

  float zero = use_NaN ? NAN : 0.0;
  float one  = 1.0;
  if (invert) std::swap (zero, one);

  if (std::isfinite (topNpercent) || std::isfinite (bottomNpercent)) {
    Image::LoopInOrder loop (in, "computing voxel count...");
    size_t count = 0;
    for (loop.start (in); loop.ok(); loop.next (in)) {
      float val = in.value();
      if (ignore_zeroes && val == 0.0) continue;
      ++count;
    }

    if (std::isfinite (topNpercent))
      topN = Math::round (0.01 * topNpercent * count);
    else
      bottomN = Math::round (0.01 * bottomNpercent * count);
  }



  if (topN || bottomN) {
    std::multimap<float,std::vector<ssize_t> > list;

    {
      Image::Loop loop ("thresholding \"" + shorten (in.name()) + "\" at " + (
                            isnan (percentile) ?
                            (str (topN ? topN : bottomN) + "th " + (topN ? "top" : "bottom") + " voxel") :
                              (str (percentile*100.0) + "\% percentile")
                            ) + "...");

      if (topN) {
        for (loop.start (in); loop.ok(); loop.next (in)) {
          const float val = in.value();
          if (!std::isfinite (val)) continue;
          if (ignore_zeroes && val == 0.0) continue;
          if (list.size() == topN) {
            if (val < list.begin()->first) continue;
            list.erase (list.begin());
          }
          std::vector<ssize_t> pos (in.ndim());
          for (size_t n = 0; n < in.ndim(); ++n)
            pos[n] = in[n];
          list.insert (std::pair<float,std::vector<ssize_t> > (val, pos));
        }
      }
      else {
        for (loop.start (in); loop.ok(); loop.next (in)) {
          const float val = in.value();
          if (!std::isfinite (val)) continue;
          if (ignore_zeroes && val == 0.0) continue;
          if (list.size() == bottomN) {
            std::multimap<float,std::vector<ssize_t> >::iterator i = list.end();
            --i;
            if (val > i->first) continue;
            list.erase (i);
          }
          std::vector<ssize_t> pos (in.ndim());
          for (size_t n = 0; n < in.ndim(); ++n)
            pos[n] = in[n];
          list.insert (std::pair<float,std::vector<ssize_t> > (val, pos));
        }
      }
    }

    Image::Loop loop;
    for (loop.start (out); loop.ok(); loop.next (out)) {
      out.value() = zero;
    }

    for (std::multimap<float,std::vector<ssize_t> >::const_iterator i = list.begin(); i != list.end(); ++i) {
      for (size_t n = 0; n < out.ndim(); ++n)
        out[n] = i->second[n];
      out.value() = one;
    }
  }
  else {
    if (use_histogram) {
      Image::Histogram<Image::Buffer<float>::voxel_type> hist (in);
      threshold_value = hist.first_min();
    }
    else if (isnan (threshold_value)) {
      Ptr<Image::Buffer<bool> > mask_data;
      Ptr<Image::Buffer<bool>::voxel_type > mask_voxel;
      opt = get_options ("mask");
      if (opt.size()) {
        mask_data = new Image::Buffer<bool> (opt[0][0]);
        mask_voxel = new Image::Buffer<bool>::voxel_type (*mask_data);
      }
      threshold_value = Image::Filter::estimate_optimal_threshold (in, (Image::Buffer<bool>::voxel_type*) mask_voxel);
    }

    Image::Loop loop ("thresholding \"" + shorten (in.name()) + "\" at intensity " + str (threshold_value) + "...");
    for (loop.start (out, in); loop.ok(); loop.next (out, in)) {
      float val = in.value();
      out.value() = ( !std::isfinite (val) || val < threshold_value ) ? zero : one;
    }
  }
}
