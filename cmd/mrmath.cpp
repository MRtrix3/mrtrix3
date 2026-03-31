/* Copyright (c) 2008-2026 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#include <limits>

#include "command.h"
#include "image.h"
#include "image_helpers.h"
#include "memory.h"
#include "progressbar.h"
#include "algo/threaded_loop.h"
#include "dwi/gradient.h"
#include "fixel/helpers.h"
#include "math/math.h"
#include "math/median.h"
#include "metadata/phase_encoding.h"
#include <array>

#include <limits>


using namespace MR;
using namespace App;

const char* operations[] = {
  "mean",
  "median",
  "sum",
  "product",
  "rms",
  "norm",
  "var",
  "std",
  "min",
  "max",
  "absmax", // Maximum of absolute values
  "magmax", // Value for which the magnitude is the maximum (i.e. preserves signed-ness)
  NULL
};

void usage ()
{
  AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

  SYNOPSIS = "Compute summary statistic on image intensities either across images, "
             "or along a specified axis of a single image";

  DESCRIPTION
    + "Supported operations are:"

    + "mean, median, sum, product, rms (root-mean-square value), norm (vector 2-norm), var (unbiased variance), "
      "std (unbiased standard deviation), min, max, absmax (maximum absolute value), "
      "magmax (value with maximum absolute value, preserving its sign)."

    + "This command is used to traverse either along an image axis, or across a "
      "set of input images, calculating some statistic from the values along each "
      "traversal. If you are seeking to instead perform mathematical calculations "
      "that are done independently for each voxel, pleaase see the 'mrcalc' command.";

  EXAMPLES
  + Example ("Calculate a 3D volume representing the mean intensity across a 4D image series",
             "mrmath 4D.mif mean 3D_mean.mif -axis 3",
             "This is a common operation for calculating e.g. the mean value within a "
             "specific DWI b-value. Note that axis indices start from 0; thus, axes 0, 1 & 2 "
             "are the three spatial axes, and axis 3 operates across volumes.")

  + Example ("Generate a Maximum Intensity Projection (MIP) along the inferior-superior direction",
             "mrmath input.mif max MIP.mif -axis 2",
             "Since a MIP is literally the maximal value along a specific projection direction, "
             "axis-aligned MIPs can be generated easily using mrmath with the \'max\' operation.");

  ARGUMENTS
  + Argument ("input", "the input image(s).").type_image_in ().allow_multiple()
  + Argument ("operation", "the operation to apply, one of: " + join(operations, ", ") + ".").type_choice (operations)
  + Argument ("output", "the output image.").type_image_out ();

  OPTIONS
  + Option ("axis", "perform operation along a specified axis of a single input image")
    + Argument ("index").type_integer (0)

  + Option ("keep_unary_axes", "Keep unary axes in input images prior to calculating the stats. "
    "The default is to wipe axes with single elements.")

  + DataType::options();
}


using value_type = float;


class Mean { NOMEMALIGN
  public:
    Mean () : sum (0.0), count (0) { }
    void operator() (value_type val) {
      if (std::isfinite (val)) {
        sum += val;
        ++count;
      }
    }
    value_type result () const {
      if (!count)
        return NAN;
      return sum / count;
    }
    double sum;
    size_t count;
};

class Median { NOMEMALIGN
  public:
    Median () { }
    void operator() (value_type val) {
      if (!std::isnan (val))
        values.push_back(val);
    }
    value_type result () {
      return Math::median(values);
    }
    vector<value_type> values;
};

class Sum { NOMEMALIGN
  public:
    Sum () : sum (0.0) { }
    void operator() (value_type val) {
      if (std::isfinite (val))
        sum += val;
    }
    value_type result () const {
      return sum;
    }
    double sum;
};


class Product { NOMEMALIGN
  public:
    Product () : product (NAN) { }
    void operator() (value_type val) {
      if (std::isfinite (val))
        product = std::isfinite (product) ? product * val : val;
    }
    value_type result () const {
      return product;
    }
    double product;
};


class RMS { NOMEMALIGN
  public:
    RMS() : sum (0.0), count (0) { }
    void operator() (value_type val) {
      if (std::isfinite (val)) {
        sum += Math::pow2 (val);
        ++count;
      }
    }
    value_type result() const {
      if (!count)
        return NAN;
      return std::sqrt(sum / count);
    }
    double sum;
    size_t count;
};

class NORM2 { NOMEMALIGN
  public:
    NORM2() : sum (0.0), count (0) { }
    void operator() (value_type val) {
      if (std::isfinite (val)) {
        sum += Math::pow2 (val);
        ++count;
      }
    }
    value_type result() const {
      if (!count)
        return NAN;
      return std::sqrt(sum);
    }
    double sum;
    size_t count;
};


// Welford's algorithm to avoid catastrophic cancellation
class Var { NOMEMALIGN
  public:
    Var () : delta (0.0), delta2 (0.0), mean (0.0), m2 (0.0), count (0) { }
    void operator() (value_type val) {
      if (std::isfinite (val)) {
        ++count;
        delta = val - mean;
        mean += delta / count;
        delta2 = val - mean;
        m2 += delta * delta2;
      }
    }
    value_type result () const {
      if (count < 2)
        return NAN;
      return m2 / (static_cast<double> (count) - 1.0);
    }
    double delta, delta2, mean, m2;
    size_t count;
};


class Std : public Var { NOMEMALIGN
  public:
    Std() : Var() { }
    value_type result () const { return std::sqrt (Var::result()); }
};


class Min { NOMEMALIGN
  public:
    Min () : min (std::numeric_limits<value_type>::infinity()) { }
    void operator() (value_type val) {
      if (std::isfinite (val) && val < min)
        min = val;
    }
    value_type result () const { return std::isfinite (min) ? min : NAN; }
    value_type min;
};


class Max { NOMEMALIGN
  public:
    Max () : max (-std::numeric_limits<value_type>::infinity()) { }
    void operator() (value_type val) {
      if (std::isfinite (val) && val > max)
        max = val;
    }
    value_type result () const { return std::isfinite (max) ? max : NAN; }
    value_type max;
};


class AbsMax { NOMEMALIGN
  public:
    AbsMax () : max (-std::numeric_limits<value_type>::infinity()) { }
    void operator() (value_type val) {
      if (std::isfinite (val) && abs(val) > max)
        max = abs(val);
    }
    value_type result () const { return std::isfinite (max) ? max : NAN; }
    value_type max;
};

class MagMax { NOMEMALIGN
  public:
    MagMax () : max (-std::numeric_limits<value_type>::infinity()) { }
    MagMax (const int i) : max (-std::numeric_limits<value_type>::infinity()) { }
    void operator() (value_type val) {
      if (std::isfinite (val) && (!std::isfinite (max) || abs(val) > abs (max)))
        max = val;
    }
    value_type result () const { return std::isfinite (max) ? max : NAN; }
    value_type max;
};





template <class Operation>
class AxisKernel { NOMEMALIGN
  public:
    AxisKernel (size_t axis) : axis (axis) { }

    template <class InputImageType, class OutputImageType>
      void operator() (InputImageType& in, OutputImageType& out) {
        Operation op;
        for (auto l = Loop (axis) (in); l; ++l)
          op (in.value());
        out.value() = op.result();
      }
  protected:
    const size_t axis;
};





class ImageKernelBase { NOMEMALIGN
  public:
    virtual ~ImageKernelBase () { }
    virtual void process (Header& image_in) = 0;
    virtual void write_back (Image<value_type>& out) = 0;
};



template <class Operation>
class ImageKernel : public ImageKernelBase { NOMEMALIGN
  protected:
    class InitFunctor { NOMEMALIGN
      public:
        template <class ImageType>
          void operator() (ImageType& out) const { out.value() = Operation(); }
    };
    class ProcessFunctor { NOMEMALIGN
      public:
        template <class ImageType1, class ImageType2>
          void operator() (ImageType1& out, ImageType2& in) const {
            Operation op = out.value();
            op (in.value());
            out.value() = op;
          }
    };
    class ResultFunctor { NOMEMALIGN
      public:
        template <class ImageType1, class ImageType2>
          void operator() (ImageType1& out, ImageType2& in) const {
            Operation op = in.value();
            out.value() = op.result();
          }
    };

  public:
    ImageKernel (const Header& header) :
      image (Header::scratch (header).get_image<Operation>()) {
        ThreadedLoop (image).run (InitFunctor(), image);
      }

    void write_back (Image<value_type>& out)
    {
      ThreadedLoop (image).run (ResultFunctor(), out, image);
    }

    void process (Header& header_in)
    {
      auto in = header_in.get_image<value_type>();
      ThreadedLoop (image).run (ProcessFunctor(), image, in);
    }

  protected:
    Image<Operation> image;
};

void run ()
{
  const size_t num_inputs = argument.size() - 2;
  const int op = argument[num_inputs];
  const std::string& output_path = argument.back();

  auto opt = get_options ("axis");
  if (opt.size()) {

    if (num_inputs != 1)
      throw Exception ("Option -axis only applies if a single input image is used");

    const size_t axis = opt[0][0];

    auto image_in = Header::open (argument[0]).get_image<value_type>().with_direct_io (axis);

    if (axis >= image_in.ndim())
      throw Exception ("Cannot perform operation along axis " + str (axis) + "; image only has " + str(image_in.ndim()) + " axes");

    Header header_out (image_in);

    if (axis == 3) {
      try {
        const auto DW_scheme = DWI::parse_DW_scheme (header_out);
        DWI::stash_DW_scheme (header_out, DW_scheme);
      } catch (...) { }
      DWI::clear_DW_scheme (header_out);
      Metadata::PhaseEncoding::clear_scheme (header_out.keyval());
    }

    header_out.datatype() = DataType::from_command_line (DataType::Float32);
    header_out.size(axis) = 1;
    squeeze_dim (header_out);

    auto image_out = Header::create (output_path, header_out).get_image<float>();

    auto loop = ThreadedLoop (std::string("computing ") + operations[op] + " along axis " + str(axis) + "...", image_out);

    switch (op) {
      case 0: loop.run  (AxisKernel<Mean>   (axis), image_in, image_out); return;
      case 1: loop.run  (AxisKernel<Median> (axis), image_in, image_out); return;
      case 2: loop.run  (AxisKernel<Sum>    (axis), image_in, image_out); return;
      case 3: loop.run  (AxisKernel<Product>(axis), image_in, image_out); return;
      case 4: loop.run  (AxisKernel<RMS>    (axis), image_in, image_out); return;
      case 5: loop.run  (AxisKernel<NORM2>  (axis), image_in, image_out); return;
      case 6: loop.run  (AxisKernel<Var>    (axis), image_in, image_out); return;
      case 7: loop.run  (AxisKernel<Std>    (axis), image_in, image_out); return;
      case 8: loop.run  (AxisKernel<Min>    (axis), image_in, image_out); return;
      case 9: loop.run  (AxisKernel<Max>    (axis), image_in, image_out); return;
      case 10: loop.run  (AxisKernel<AbsMax> (axis), image_in, image_out); return;
      case 11: loop.run (AxisKernel<MagMax> (axis), image_in, image_out); return;
      default: assert (0);
    }

  } else {

    if (num_inputs < 2)
      throw Exception ("mrmath requires either multiple input images, or the -axis option to be provided");

    vector<Header> headers_in (num_inputs);

    // Header of first input image is the template to which all other input images are compared
    headers_in[0] = Header::open (argument[0]);
    Header header_out (headers_in[0]);
    header_out.datatype() = DataType::from_command_line (DataType::Float32);

    // find dimensionality of output image once unary-dimensional axes are optionally removed
    size_t ndim_out = header_out.ndim();
    if (!get_options ("keep_unary_axes").size() ) {
      while (header_out.size (ndim_out - 1) == 1)
        --ndim_out;
    }

    // load all image headers,
    //   and verify that dimensions of all input images adequately match
    for (size_t i = 1; i < num_inputs; ++i) {
      headers_in[i] = Header::open(argument[i]);
      if (headers_in[i].ndim() < header_out.ndim())
        throw Exception ("Image " + headers_in[i].name() + " has fewer axes" + //
                         " than first input image " + headers_in[i].name());   //
      if (header_out.ndim() >= 3 &&                                       //
          !Fixel::is_data_file(header_out) &&                             //
          !Fixel::is_data_file(headers_in[i]) &&                          //
          !voxel_grids_match_in_scanner_space(headers_in[i], header_out)) //
        throw Exception("Header transform of image " + headers_in[i].name() +         //
                        " differs from that of first image " + headers_in[i].name()); //
      if (!dimensions_match(headers_in[i], header_out, 0, ndim_out))
        throw Exception ("Dimensions of image " + headers_in[i].name() +                      //
                         " do not match those of first input image " + headers_in[0].name()); //
      for (size_t axis = ndim_out; axis != headers_in[i].ndim(); ++axis) {
        if (headers_in[i].size(axis) != 1)
          throw Exception ("Image " + headers_in[i].name() + " has axis with non-unary dimension"
                           " beyond first input image " + headers_in[0].name());
      }
      header_out.merge_keyval(headers_in[i].keyval());
    }

    // Wipe any excess unary-dimensional axes
    header_out.ndim() = ndim_out;

    // Instantiate a kernel depending on the operation requested
    std::unique_ptr<ImageKernelBase> kernel;
    switch (op) {
      case 0:  kernel.reset (new ImageKernel<Mean>    (header_out)); break;
      case 1:  kernel.reset (new ImageKernel<Median>  (header_out)); break;
      case 2:  kernel.reset (new ImageKernel<Sum>     (header_out)); break;
      case 3:  kernel.reset (new ImageKernel<Product> (header_out)); break;
      case 4:  kernel.reset (new ImageKernel<RMS>     (header_out)); break;
      case 5:  kernel.reset (new ImageKernel<NORM2>   (header_out)); break;
      case 6:  kernel.reset (new ImageKernel<Var>     (header_out)); break;
      case 7:  kernel.reset (new ImageKernel<Std>     (header_out)); break;
      case 8:  kernel.reset (new ImageKernel<Min>     (header_out)); break;
      case 9:  kernel.reset (new ImageKernel<Max>     (header_out)); break;
      case 10:  kernel.reset (new ImageKernel<AbsMax>  (header_out)); break;
      case 11: kernel.reset (new ImageKernel<MagMax>  (header_out)); break;
      default: assert (0);
    }

    // Feed the input images to the kernel one at a time
    {
      ProgressBar progress (std::string("computing ") + operations[op] + " across "
          + str(headers_in.size()) + " images", num_inputs);
      for (size_t i = 0; i != headers_in.size(); ++i) {
        assert (headers_in[i].valid());
        assert (headers_in[i].is_file_backed());
        kernel->process (headers_in[i]);
        ++progress;
      }
    }

    auto out = Header::create (output_path, header_out).get_image<value_type>();
    kernel->write_back (out);
  }

}

