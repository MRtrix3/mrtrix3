/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */


#include "command.h"
#include "progressbar.h"

#include "image.h"
#include "algo/loop.h"
#include "algo/threaded_loop.h"

#include "sparse/fixel_metric.h"
#include "sparse/image.h"
#include "sparse/keys.h"

#include "math/SH.h"


using namespace MR;
using namespace App;


using Sparse::FixelMetric;

const char* operations[] = {
  "mean",
  "sum",
  "product",
  "rms",
  "var",
  "std",
  "min",
  "max",
  "absmax",
  "magmax",
  "count",
  "complexity",
  "sf",
  "dec_unit",
  "dec_scaled",
  "split_size",
  "split_value",
  "split_dir",
  NULL
};


void usage ()
{
  AUTHOR = "Robert E. Smith (robert.smith@florey.edu.au)";

  DESCRIPTION
  + "convert a fixel-based sparse-data image into some form of scalar image. "
    "This could be: \n"
    "- Some statistic computed across all fixel values within a voxel: mean, sum, product, rms, var, std, min, max, absmax, magmax\n"
    "- The number of fixels in each voxel: count\n"
    "- Some measure of crossing-fibre organisation: complexity, sf ('single-fibre')\n"
    "- A 4D directionally-encoded colour image: dec_unit, dec_scaled\n"
    "- A 4D scalar image with one 3D volume per fixel: split_size, split_value\n"
    "- A 4D image with three 3D volumes per fixel direction: split_dir";

  REFERENCES 
    + "* Reference for 'complexity' operation:\n"
    "Riffert, T. W.; Schreiber, J.; Anwander, A. & Knosche, T. R. "
    "Beyond Fractional Anisotropy: Extraction of bundle-specific structural metrics from crossing fibre models. "
    "NeuroImage, 2014 (in press)";

  ARGUMENTS
  + Argument ("fixel_in",  "the input sparse fixel image.").type_image_in ()
  + Argument ("operation", "the operation to apply, one of: " + join(operations, ", ") + ".").type_choice (operations)
  + Argument ("image_out", "the output scalar image.").type_image_out ();

  OPTIONS
  + Option ("weighted", "weight the contribution of each fixel to the per-voxel result according to its volume "
                        "(note that this option is not applicable for all operations, and should be avoided if the "
                        "value stored in the fixel image is itself the estimated fibre volume)");

}



class OpBase
{
  protected:
    typedef Sparse::Image<FixelMetric> in_type;
    typedef Image<float> out_type;

  public:
    OpBase (const bool weighted) :
        weighted (weighted) { }
    OpBase (const OpBase& that) :
        weighted (that.weighted) { }
    virtual ~OpBase() { }
    virtual bool operator() (in_type&, out_type&) = 0;
  protected:
    const bool weighted;
};


class Mean : public OpBase
{
  public:
    Mean (const bool weighted) :
        OpBase (weighted),
        sum (0.0f),
        sum_volumes (0.0f) { }
    Mean (const Mean& that) :
        OpBase (that),
        sum (0.0f),
        sum_volumes (0.0f) { }
    bool operator() (in_type& in, out_type& out)
    {
      sum = sum_volumes = 0.0;
      if (weighted) {
        for (size_t i = 0; i != in.value().size(); ++i) {
          sum += in.value()[i].size * in.value()[i].value;
          sum_volumes += in.value()[i].size;
        }
        out.value() = sum_volumes ? (sum / sum_volumes) : 0.0;
      } else {
        for (size_t i = 0; i != in.value().size(); ++i)
          sum += in.value()[i].value;
        out.value() = in.value().size() ? (sum / default_type(in.value().size())) : 0.0;
      }
      return true;
    }
  private:
    default_type sum, sum_volumes;
};

class Sum : public OpBase
{
  public:
    Sum (const bool weighted) :
        OpBase (weighted),
        sum (0.0) { }
    Sum (const Sum& that) :
        OpBase (that),
        sum (0.0) { }
    bool operator() (in_type& in, out_type& out)
    {
      sum = 0.0;
      if (weighted) {
        for (size_t i = 0; i != in.value().size(); ++i)
          sum += in.value()[i].size * in.value()[i].value;
        out.value() = sum;
      } else {
        for (size_t i = 0; i != in.value().size(); ++i)
          sum += in.value()[i].value;
        out.value() = sum;
      }
      return true;
    }
  private:
    default_type sum;
};

class Product : public OpBase
{
  public:
    Product (const bool weighted) :
        OpBase (weighted),
        product (0.0)
    {
      if (weighted)
        WARN ("Option -weighted has no meaningful interpretation for product operation; ignoring");
    }
    Product (const Product& that) :
        OpBase (that),
        product (0.0) { }
    bool operator() (in_type& in, out_type& out)
    {
      if (!in.value().size()) {
        out.value() = 0.0;
        return true;
      }
      product = in.value()[0].value;
      for (size_t i = 1; i != in.value().size(); ++i)
        product *= in.value()[i].value;
      out.value() = product;
      return true;
    }
  private:
    default_type product;
};

class RMS : public OpBase
{
  public:
    RMS (const bool weighted) :
        OpBase (weighted),
        sum (0.0),
        sum_volumes (0.0) { }
    RMS (const RMS& that) :
        OpBase (that),
        sum (0.0),
        sum_volumes (0.0) { }
    bool operator() (in_type& in, out_type& out)
    {
      sum = sum_volumes = 0.0;
      if (weighted) {
        for (size_t i = 0; i != in.value().size(); ++i) {
          sum *= in.value()[i].size * Math::pow2 (in.value()[i].value);
          sum_volumes += in.value()[i].size;
        }
        out.value() = std::sqrt (sum / sum_volumes);
      } else {
        for (size_t i = 0; i != in.value().size(); ++i)
          sum += Math::pow2 (in.value()[i].value);
        out.value() = std::sqrt (sum / default_type(in.value().size()));
      }
      return true;
    }
  private:
    default_type sum, sum_volumes;
};

class Var : public OpBase
{
  public:
    Var (const bool weighted) :
        OpBase (weighted),
        sum (0.0f),
        weighted_mean (0.0),
        sum_volumes (0.0),
        sum_volumes_sqr (0.0),
        sum_sqr (0.0) { }
    Var (const Var& that) :
        OpBase (that),
        sum (0.0),
        weighted_mean (0.0),
        sum_volumes (0.0),
        sum_volumes_sqr (0.0),
        sum_sqr (0.0) { }
    bool operator() (in_type& in, out_type& out)
    {
      if (!in.value().size()) {
        out.value() = NAN;
        return true;
      } else if (in.value().size() == 1) {
        out.value() = 0.0;
        return true;
      }
      if (weighted) {
        sum = sum_volumes = 0.0;
        for (size_t i = 0; i != in.value().size(); ++i) {
          sum += in.value()[i].size * in.value()[i].value;
          sum_volumes += in.value()[i].size;
        }
        weighted_mean = sum / sum_volumes;
        sum = sum_volumes_sqr = 0.0;
        for (size_t i = 0; i != in.value().size(); ++i) {
          sum += in.value()[i].size * Math::pow2 (weighted_mean - in.value()[i].value);
          sum_volumes_sqr += Math::pow2 (in.value()[i].size);
        }
        // Unbiased variance estimator in the presence of weighting
        out.value() = sum / (sum_volumes - (sum_volumes_sqr / sum_volumes));
      } else {
        sum = sum_sqr = 0.0;
        for (size_t i = 0; i != in.value().size(); ++i) {
          sum += in.value()[i].value;
          sum_sqr += Math::pow2 (in.value()[i].value);
        }
        out.value() = (sum_sqr - (Math::pow2 (sum) / default_type(in.value().size()))) / default_type(in.value().size() - 1);
      }
      return true;
    }
  private:
    // Used for both calculations
    default_type sum;
    // Used for weighted calculations
    default_type weighted_mean, sum_volumes, sum_volumes_sqr;
    // Used for unweighted calculations
    default_type sum_sqr;
};

class Std : public Var
{
  public:
    Std (const bool weighted) :
        Var (weighted) { }
    Std (const Std& that) :
        Var (that) { }
    bool operator() (in_type& in, out_type& out)
    {
      Var::operator() (in, out);
      out.value() = std::sqrt (out.value());
      return true;
    }
};

class Min : public OpBase
{
  public:
    Min (const bool weighted) :
        OpBase (weighted),
        min (std::numeric_limits<default_type>::infinity())
    {
      if (weighted)
        WARN ("Option -weighted has no meaningful interpretation for min operation; ignoring");
    }
    Min (const Min& that) :
        OpBase (that),
        min (std::numeric_limits<default_type>::infinity()) { }
    bool operator() (in_type& in, out_type& out)
    {
      min = std::numeric_limits<default_type>::infinity();
      for (size_t i = 0; i != in.value().size(); ++i) {
        if (in.value()[i].value < min)
          min = in.value()[i].value;
      }
      out.value() = std::isfinite (min) ? min : NAN;
      return true;
    }
  private:
    default_type min;
};

class Max : public OpBase
{
  public:
    Max (const bool weighted) :
        OpBase (weighted),
        max (-std::numeric_limits<default_type>::infinity())
    {
      if (weighted)
        WARN ("Option -weighted has no meaningful interpretation for max operation; ignoring");
    }
    Max (const Max& that) :
        OpBase (that),
        max (-std::numeric_limits<default_type>::infinity()) { }
    bool operator() (in_type& in, out_type& out)
    {
      max = -std::numeric_limits<default_type>::infinity();
      for (size_t i = 0; i != in.value().size(); ++i) {
        if (in.value()[i].value > max)
          max = in.value()[i].value;
      }
      out.value() = std::isfinite (max) ? max : NAN;
      return true;
    }
  private:
    default_type max;
};

class AbsMax : public OpBase
{
  public:
    AbsMax (const bool weighted) :
        OpBase (weighted),
        max (0.0f)
    {
      if (weighted)
        WARN ("Option -weighted has no meaningful interpretation for absmax operation; ignoring");
    }
    AbsMax (const AbsMax& that) :
        OpBase (that),
        max (0.0f) { }
    bool operator() (in_type& in, out_type& out)
    {
      max = 0.0f;
      for (size_t i = 0; i != in.value().size(); ++i) {
        if (std::abs (in.value()[i].value) > max)
          max = std::abs (in.value()[i].value);
      }
      out.value() = max;
      return true;
    }
  private:
    default_type max;
};

class MagMax : public OpBase
{
  public:
    MagMax (const bool weighted) :
        OpBase (weighted),
        max (0.0)
    {
      if (weighted)
        WARN ("Option -weighted has no meaningful interpretation for magmax operation; ignoring");
    }
    MagMax (const AbsMax& that) :
        OpBase (that),
        max (0.0) { }
    bool operator() (in_type& in, out_type& out)
    {
      max = 0.0;
      for (size_t i = 0; i != in.value().size(); ++i) {
        if (std::abs (in.value()[i].value) > std::abs (max))
          max = in.value()[i].value;
      }
      out.value() = max;
      return true;
    }
  private:
    default_type max;
};

class Count : public OpBase
{
  public:
    Count (const bool weighted) :
        OpBase (weighted)
    {
      if (weighted)
        WARN ("Option -weighted has no meaningful interpretation for count operation; ignoring");
    }
    Count (const Count& that) :
        OpBase (that) { }
    bool operator() (in_type& in, out_type& out)
    {
      out.value() = in.value().size();
      return true;
    }
};

class Complexity : public OpBase
{
  public:
    Complexity (const bool weighted) :
        OpBase (weighted),
        max (0.0),
        sum (0.0)
    {
      if (weighted)
        WARN ("Option -weighted has no meaningful interpretation for complexity operation; ignoring");
    }
    Complexity (const Complexity& that) :
        OpBase (that),
        max (0.0f),
        sum (0.0f) { }
    bool operator() (in_type& in, out_type& out)
    {
      if (in.value().size() <= 1) {
        out.value() = 0.0;
        return true;
      }
      max = sum = 0.0;
      for (size_t i = 0; i != in.value().size(); ++i) {
        max = std::max (max, default_type(in.value()[i].value));
        sum += in.value()[i].value;
      }
      out.value() = (default_type(in.value().size()) / default_type(in.value().size()-1.0)) * (1.0 - (max / sum));
      return true;
    }
  private:
    default_type max, sum;
};

class SF : public OpBase
{
  public:
    SF (const bool weighted) :
        OpBase (weighted),
        max (0.0),
        sum (0.0)
    {
      if (weighted)
        WARN ("Option -weighted has no meaningful interpretation for sf operation; ignoring");
    }
    SF (const Complexity& that) :
        OpBase (that),
        max (0.0),
        sum (0.0) { }
    bool operator() (in_type& in, out_type& out)
    {
      max = sum = 0.0f;
      for (size_t i = 0; i != in.value().size(); ++i) {
        max = std::max (max, default_type(in.value()[i].value));
        sum += in.value()[i].value;
      }
      out.value() = sum ? (max / sum) : 0.0f;
      return true;
    }
  private:
    default_type max, sum;
};

class DEC_unit : public OpBase
{
  public:
    DEC_unit (const bool weighted) :
        OpBase (weighted),
        sum_dec {0.0, 0.0, 0.0} { }
    DEC_unit (const DEC_unit& that) :
        OpBase (that),
        sum_dec {0.0, 0.0, 0.0} { }
    bool operator() (in_type& in, out_type& out)
    {
      sum_dec = {0.0, 0.0, 0.0};
      if (weighted) {
        for (size_t i = 0; i != in.value().size(); ++i)
          sum_dec += Eigen::Vector3 (std::abs (in.value()[i].dir[0]), std::abs (in.value()[i].dir[1]), std::abs (in.value()[i].dir[2])) * in.value()[i].value * in.value()[i].size;
      } else {
        for (size_t i = 0; i != in.value().size(); ++i)
          sum_dec += Eigen::Vector3 (std::abs (in.value()[i].dir[0]), std::abs (in.value()[i].dir[1]), std::abs (in.value()[i].dir[2])) * in.value()[i].value;
      }
      sum_dec.normalize();
      for (out.index(3) = 0; out.index(3) != 3; ++out.index(3))
        out.value() = sum_dec[size_t(out.index(3))];
      return true;
    }
  private:
    Eigen::Vector3 sum_dec;
};

class DEC_scaled : public OpBase
{
  public:
    DEC_scaled (const bool weighted) :
        OpBase (weighted),
        sum_dec {0.0, 0.0, 0.0},
        sum_volume (0.0),
        sum_value (0.0) { }
    DEC_scaled (const DEC_scaled& that) :
        OpBase (that),
        sum_dec {0.0, 0.0, 0.0},
        sum_volume (0.0),
        sum_value (0.0) { }
    bool operator() (in_type& in, out_type& out)
    {
      sum_dec = {0.0, 0.0, 0.0};
      sum_value = 0.0;
      if (weighted) {
        sum_volume = 0.0;
        for (size_t i = 0; i != in.value().size(); ++i) {
          sum_dec += Eigen::Vector3 (std::abs (in.value()[i].dir[0]), std::abs (in.value()[i].dir[1]), std::abs (in.value()[i].dir[2])) * in.value()[i].value * in.value()[i].size;
          sum_volume += in.value()[i].size;
          sum_value += in.value()[i].size * in.value()[i].value;
        }
        sum_dec.normalize();
        sum_dec *= (sum_value / sum_volume);
      } else {
        for (size_t i = 0; i != in.value().size(); ++i) {
          sum_dec += Eigen::Vector3 (std::abs (in.value()[i].dir[0]), std::abs (in.value()[i].dir[1]), std::abs (in.value()[i].dir[2])) * in.value()[i].value;
          sum_value += in.value()[i].value;
        }
        sum_dec.normalize();
        sum_dec *= sum_value;
      }
      for (out.index(3) = 0; out.index(3) != 3; ++out.index(3))
        out.value() = sum_dec[size_t(out.index(3))];
      return true;
    }
  private:
    Eigen::Vector3 sum_dec;
    default_type sum_volume, sum_value;
};

class SplitSize : public OpBase
{
  public:
    SplitSize (const bool weighted) :
        OpBase (weighted)
    {
      if (weighted)
        WARN ("Option -weighted has no meaningful interpretation for split_amp operation; ignoring");
    }
    SplitSize (const SplitSize& that) :
        OpBase (that) { }
    bool operator() (in_type& in, out_type& out)
    {
      for (out.index(3) = 0; out.index(3) < out.size(3); ++out.index(3)) {
        if (out.index(3) < in.value().size())
          out.value() = in.value()[out.index(3)].size;
        else
          out.value() = 0.0;
      }
      return true;
    }
};

class SplitValue : public OpBase
{
  public:
    SplitValue (const bool weighted) :
        OpBase (weighted)
    {
      if (weighted)
        WARN ("Option -weighted has no meaningful interpretation for split_value operation; ignoring");
    }
    SplitValue (const SplitValue& that) :
        OpBase (that) { }
    bool operator() (in_type& in, out_type& out)
    {
      for (out.index(3) = 0; out.index(3) < out.size(3); ++out.index(3)) {
        if (out.index(3) < in.value().size())
          out.value() = in.value()[out.index(3)].value;
        else
          out.value() = 0.0;
      }
      return true;
    }
};

class SplitDir : public OpBase
{
  public:
    SplitDir (const bool weighted) :
        OpBase (weighted)
    {
      if (weighted)
        WARN ("Option -weighted has no meaningful interpretation for split_dir operation; ignoring");
    }
    SplitDir (const SplitDir& that) :
        OpBase (that) { }
    bool operator() (in_type& in, out_type& out)
    {
      size_t index;
      out.index(3) = 0;
      for (index = 0; index != in.value().size(); ++index) {
        for (size_t axis = 0; axis != 3; ++axis) {
          out.value() = in.value()[index].dir[axis];
          ++out.index(3);
        }
      }
      for (; out.index(3) != out.size(3); ++out.index(3))
        out.value() = NAN;
      return true;
    }
};








void run ()
{
  Header H_in = Header::open (argument[0]);
  Sparse::Image<FixelMetric> in (H_in);

  const int op = argument[1];

  Header H_out (H_in);
  H_out.datatype() = DataType::Float32;
  H_out.datatype().set_byte_order_native();
  H_out.keyval().erase (Sparse::name_key);
  H_out.keyval().erase (Sparse::size_key);
  if (op == 10) { // count
    H_out.datatype() = DataType::UInt8;
  } else if (op == 13 || op == 14) { // dec
    H_out.ndim() = 4;
    H_out.size (3) = 3;
  } else if (op == 15 || op == 16 || op == 17) { // split_*
    H_out.ndim() = 4;
    uint32_t max_count = 0;
    for (auto l = Loop ("determining largest fixel count", in) (in); l; ++l)
      max_count = std::max (max_count, in.value().size());
    if (max_count == 0)
      throw Exception ("fixel image is empty");
    // 3 volumes per fixel if performing split_dir
    H_out.size(3) = (op == 17) ? (3 * max_count) : max_count;
  }

  auto out = Image<float>::create (argument[2], H_out);

  auto opt = get_options ("weighted");
  const bool weighted = opt.size();

  auto loop = ThreadedLoop ("converting sparse fixel data to scalar image", in);

  switch (op) {
    case 0:  loop.run (Mean       (weighted), in, out); break;
    case 1:  loop.run (Sum        (weighted), in, out); break;
    case 2:  loop.run (Product    (weighted), in, out); break;
    case 3:  loop.run (RMS        (weighted), in, out); break;
    case 4:  loop.run (Var        (weighted), in, out); break;
    case 5:  loop.run (Std        (weighted), in, out); break;
    case 6:  loop.run (Min        (weighted), in, out); break;
    case 7:  loop.run (Max        (weighted), in, out); break;
    case 8:  loop.run (AbsMax     (weighted), in, out); break;
    case 9:  loop.run (MagMax     (weighted), in, out); break;
    case 10: loop.run (Count      (weighted), in, out); break;
    case 11: loop.run (Complexity (weighted), in, out); break;
    case 12: loop.run (SF         (weighted), in, out); break;
    case 13: loop.run (DEC_unit   (weighted), in, out); break;
    case 14: loop.run (DEC_scaled (weighted), in, out); break;
    case 15: loop.run (SplitSize  (weighted), in, out); break;
    case 16: loop.run (SplitValue (weighted), in, out); break;
    case 17: loop.run (SplitDir   (weighted), in, out); break;
  }

}

