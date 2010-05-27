/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 03/12/09.

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
#include "image/voxel.h"
#include "math/rng.h"
#include "dataset/loop.h"
#include "dataset/stride.h"

using namespace MR; 

SET_VERSION_DEFAULT;

DESCRIPTION = {
  "apply generic mathematical operations to images.",
  NULL
};


ARGUMENTS = { 
  Argument ("image", "input image", "the input image.").type_string (),
  Argument ("output", "output image", "the output image.").type_image_out (),
  Argument::End 
}; 

OPTIONS = { 
  Option ("abs", "absolute value", "take absolute value of current voxel value"),
  Option ("neg", "negative value", "take negative value of current voxel value"),
  Option ("sqrt", "square root", "take square root of current voxel value"),
  Option ("exp", "exp", "take e raised to the power of current voxel value"),
  Option ("log", "log", "take natural logarithm of current voxel value"),
  Option ("cos", "cos", "take cosine of current voxel value"),
  Option ("sin", "sin", "take sine of current voxel value"),
  Option ("tan", "tanh", "take tangent of current voxel value"),
  Option ("cosh", "cosh", "take hyperbolic cosine of current voxel value"),
  Option ("sinh", "sinh", "take hyperbolic sine of current voxel value"),
  Option ("tanh", "tanh", "take hyperbolic tangent of current voxel value"),
  Option ("acos", "acos", "take inverse cosine of current voxel value"),
  Option ("asin", "asin", "take inverse sine of current voxel value"),
  Option ("atan", "atan", "take inverse tangent of current voxel value"),
  Option ("acosh", "acosh", "take inverse hyperbolic cosine of current voxel value"),
  Option ("asinh", "asinh", "take inverse hyperbolic sine of current voxel value"),
  Option ("atanh", "atanh", "take inverse hyperbolic tangent of current voxel value"),
  Option ("round", "round", "round current voxel value to nearest integer"),
  Option ("ceil", "ceil", "round current voxel value up to smallest integer not less than current value"),
  Option ("floor", "floor", "round current voxel value down to largest integer not greater than current value"),

  Option ("add", "add", "add to current voxel value the corresponding voxel value of 'source'")
    .append (Argument ("source", "source", "the source image or value").type_string()),

  Option ("subtract", "subtract", "subtract from current voxel value the corresponding voxel value of 'source'")
    .append (Argument ("source", "source", "the source image or value").type_string()),

  Option ("multiply", "multiply", "multiply current voxel value by corresponding voxel value of 'source'")
    .append (Argument ("source", "source", "the source image or value").type_string()),

  Option ("divide", "divide", "divide current voxel value by corresponding voxel value of 'source'")
    .append (Argument ("source", "source", "the source image or value").type_string()),

  Option ("min", "min", "return smallest of current voxel value and corresponding voxel value of 'source'")
    .append (Argument ("source", "source", "the source image or value").type_string()),

  Option ("max", "max", "return greatest of current voxel value and corresponding voxel value of 'source'")
    .append (Argument ("source", "source", "the source image or value").type_string()),

  Option ("lt", "less than", "return 1 if current voxel value is less than corresponding voxel value of 'source', 0 otherwise")
    .append (Argument ("source", "source", "the source image or value").type_string()),

  Option ("gt", "greater than", "return 1 if current voxel value is greater than corresponding voxel value of 'source', 0 otherwise")
    .append (Argument ("source", "source", "the source image or value").type_string()),

  Option ("le", "less than or equal to", "return 1 if current voxel value is less than or equal to corresponding voxel value of 'source', 0 otherwise")
    .append (Argument ("source", "source", "the source image or value").type_string()),

  Option ("ge", "greater than or equal to", "return 1 if current voxel value is greater than or equal to corresponding voxel value of 'source', 0 otherwise")
    .append (Argument ("source", "source", "the source image or value").type_string()),

  Option ("mean", "mean value", "return return the arithmetic mean of the voxel values along the axes specified.")
    .append (Argument ("axis", "axis", "a comma-separated list of axes along which to estimate the mean").type_string()),

  Option ("std", "standard deviation", "return return the standard deviation of the voxel values along the axes specified.")
    .append (Argument ("axis", "axis", "a comma-separated list of axes along which to estimate the standard deviation").type_string()),

  Option ("datatype", "data type", "specify output image data type.") 
    .append (Argument ("spec", "specifier", "the data type specifier.").type_choice (DataType::identifiers)),

  Option::End 
};

typedef double value_type;



// base class for each operation:
class Functor {
  public:
    Functor (Functor* input) : in (input) { }
    virtual ~Functor () { }
    virtual void get (const std::vector<ssize_t>& pos, std::vector<value_type>& values, size_t axis) { }
    virtual size_t ndim () const { return (0); }
    virtual ssize_t dim (size_t axis) const { return (0); }
    virtual const Image::Header* header () const { return (NULL); }

  protected:
    Ptr<Functor> in;
};




// data source: either a broadcastable constant, or an image:
class Source : public Functor {
  public:
    Source (const std::string& text) : Functor (NULL) {
      try {
        H = new Image::Header (Image::Header::open (text));
        V = new Image::Voxel<value_type> (*H);
      }
      catch (Exception) {
        val = to<value_type> (text);
        H = NULL;
      }
    }

    void get (const std::vector<ssize_t>& pos, std::vector<value_type>& values, size_t axis) 
    {
      if (V) { // from image:
        assert (size_t (V->dim(axis)) == values.size());

        for (size_t i = 0; i < V->ndim(); ++i) // set position
          if (i != axis) 
            (*V)[i] = V->dim(i) > 1 ? pos[i] : 0; 

        if (V->dim(axis) > 1) { // straight copy:
          for ((*V)[axis] = 0; (*V)[axis] < V->dim(axis); ++(*V)[axis]) 
            values[(*V)[axis]] = V->value();
        }
        else { // broadcast:
          (*V)[axis] = 0;
          value_type x = V->value();
          for (size_t i = 0; i < values.size(); ++i)
            values[i] = x;
        }
      }
      else // from constant:
        for (size_t i = 0; i < values.size(); ++i)
          values[i] = val;
    }

    virtual size_t ndim () const { return (V ? V->ndim() : 0); }
    virtual ssize_t dim (size_t axis) const { return (V ? V->dim (axis) : 0); }
    virtual const Image::Header* header () const { return (H.get()); }
  private:
    value_type val;
    Ptr<Image::Header> H;
    Ptr<Image::Voxel<value_type> > V;
};






// straight voxel-by-voxel operations from a single input:
class Unary : public Functor {
  public:
    Unary (Functor* input) : Functor (input) { }
    virtual size_t ndim () const { return (in->ndim()); }
    virtual ssize_t dim (size_t axis) const { return (in->dim (axis)); }
    virtual const Image::Header* header () const { return (in->header()); }
};



// straight voxel-by-voxel operations from two broadcast-matchable inputs:
class Binary : public Functor {
  public:
    Binary (Functor* input1, Functor* input2) :
      Functor (input1), in2 (input2) { 
      size_t max_dim = std::max (in->ndim(), in2->ndim());
      for (size_t i = 0; i < max_dim; ++i) {
        size_t d1 = i < in->ndim() ? in->dim(i) : 1;
        size_t d2 = i < in2->ndim() ? in2->dim(i) : 1;
        if (d1 != d2) 
          if (d1 != 1 && d2 != 1) 
            throw Exception ("dimension mismatch between inputs");
      }
    }
    virtual size_t ndim () const { return (std::max (in->ndim(), in2->ndim())); }
    virtual ssize_t dim (size_t axis) const 
    { 
      ssize_t d1 = axis < in->ndim() ? in->dim(axis) : 1;
      ssize_t d2 = axis < in2->ndim() ? in2->dim(axis) : 1;
      return (std::max (d1, d2));
    }
    virtual const Image::Header* header () const { return (in->header() ? in->header() : in2->header()); }
  protected:
    Ptr<Functor> in2;
    std::vector<value_type> values2;
};



// DataSet interface to be looped over - calls the functor chain as needed:
class Counter {
  public:
    template <class Set> 
      Counter (Functor& input, Set& output, size_t inner_axis) : 
        in (input), N_ (output.ndim()), x_ (output.ndim()), axis (inner_axis) { 
        for (size_t n = 0; n < N_.size(); ++n) {
          x_[n] = 0;
          N_[n] = output.dim(n);
        }
        values.resize (N_[axis]);
      }
    size_t ndim () const { return (N_.size()); }
    ssize_t dim (size_t i) const { return (N_[i]); }
    ssize_t& operator[] (size_t i) { return (x_[i]); }
    template <class Set> 
      void get (Set& out) {
        in.get (x_, values, axis);
        for (out[axis] = 0; out[axis] < out.dim(axis); ++out[axis]) 
          out.value() = values[out[axis]];
      }
  private:
    Functor& in;
    std::vector<ssize_t> N_, x_;
    std::vector<value_type> values;
    const size_t axis;
};




#define DEF_UNARY(name, operation) class name : public Unary { \
  public: \
    name (Functor* input) : Unary (input) { } \
    virtual void get (const std::vector<ssize_t>& pos, std::vector<value_type>& values, size_t axis) { \
      in->get (pos, values, axis); \
      for (size_t n = 0; n < values.size(); ++n) { \
        value_type& val = values[n]; \
        values[n] = operation; \
      } \
    } \
}

#define DEF_BINARY(name, operation) class name : public Binary { \
  public: \
    name (Functor* input1, Functor* input2) : Binary (input1, input2) { } \
    virtual void get (const std::vector<ssize_t>& pos, std::vector<value_type>& values, size_t axis) { \
      values2.resize (values.size()); \
      in->get (pos, values, axis); \
      in2->get (pos, values2, axis); \
      for (size_t n = 0; n < values.size(); ++n) { \
        value_type& val1 = values[n]; \
        value_type& val2 = values2[n]; \
        values[n] = operation; \
      } \
    } \
}

DEF_UNARY (Abs, Math::abs(val));
DEF_UNARY (Neg, -val);
DEF_UNARY (Sqrt, Math::sqrt (val));
DEF_UNARY (Exp, Math::exp (val));
DEF_UNARY (Log, Math::log (val));
DEF_UNARY (Cos, Math::cos (val));
DEF_UNARY (Sin, Math::sin (val));
DEF_UNARY (Tan, Math::tan (val));
DEF_UNARY (Cosh, Math::cosh (val));
DEF_UNARY (Sinh, Math::sinh (val));
DEF_UNARY (Tanh, Math::tanh (val));
DEF_UNARY (Acos, Math::acos (val));
DEF_UNARY (Asin, Math::asin (val));
DEF_UNARY (Atan, Math::atan (val));
DEF_UNARY (Acosh, Math::acosh (val));
DEF_UNARY (Asinh, Math::asinh (val));
DEF_UNARY (Atanh, Math::atanh (val));
DEF_UNARY (Round, Math::round (val));
DEF_UNARY (Ceil, Math::ceil (val));
DEF_UNARY (Floor, Math::floor (val));

DEF_BINARY (Add, val1+val2);
DEF_BINARY (Subtract, val1-val2);
DEF_BINARY (Mult, val1*val2);
DEF_BINARY (Divide, val1/val2);
DEF_BINARY (Min, std::min (val1, val2));
DEF_BINARY (Max, std::max (val1, val2));

DEF_BINARY (LessThan, val1 < val2);
DEF_BINARY (GreaterThan, val1 > val2);
DEF_BINARY (LessThanOrEqualTo, val1 <= val2);
DEF_BINARY (GreaterThanOrEqualTo, val1 >= val2);




// other less straightforward operations:

class Mean : public Unary {
  public:
    Mean (Functor* input, const std::string& axes_specifier) : 
      Unary (input), ax (in->ndim(), false) {
        norm = 1.0;
        try {
          axes_involved = parse_ints (axes_specifier);
          if (axes_involved.size() > in->ndim()) throw 0;
          for (size_t i = 0; i < axes_involved.size(); ++i) {
            if (axes_involved[i] < 0 || axes_involved[i] >= int (in->ndim())) throw 0;
            for (size_t j = 0; j < i; ++j)
              if (axes_involved[i] == axes_involved[j]) throw 0;
            ax[axes_involved[i]] = true;
            norm *= in->dim (axes_involved[i]);
          }
        }
        catch (...) {
          throw Exception ("axes specification \"" + axes_specifier + "\" does not match image");
        }
        buf.resize (in->dim(axes_involved[0]));
        norm = 1.0/norm;
      }
    virtual ssize_t dim (size_t axis) const { return (ax[axis] ? 1 : in->dim (axis)); }

    virtual void get (const std::vector<ssize_t>& pos, std::vector<value_type>& values, size_t axis) 
    { 
      assert (values.size() == (ax[axis] ? 1 : size_t(in->dim(axis))));
      std::vector<ssize_t> x = pos;

      for (x[axis] = 0; x[axis] < ssize_t(values.size()); ++x[axis]) {
        value_type sum = 0.0;
        do {
          in->get (x, buf, axes_involved[0]);
          for (size_t m = 0; m < buf.size(); ++m)
            sum += buf[m];
        } while (next (x));
        values[x[axis]] = norm * sum;
      }
    } 
  protected:
    std::vector<bool> ax;
    std::vector<int> axes_involved;
    std::vector<value_type> buf;
    value_type norm;

    bool next (std::vector<ssize_t>& x, size_t n = 1) 
    {
      if (n >= axes_involved.size()) return (false);
      x[axes_involved[n]]++;
      if (x[axes_involved[n]] >= in->dim (axes_involved[n])) {
        x[axes_involved[n]] = 0;
        return (next (x, n+1));
      }
      return (true);
    }
};



class Std : public Mean {
  public:
    Std (Functor* input, const std::string& axes_specifier) : 
      Mean (input, axes_specifier) { }

    virtual void get (const std::vector<ssize_t>& pos, std::vector<value_type>& values, size_t axis) 
    { 
      assert (values.size() == (ax[axis] ? 1 : size_t(in->dim(axis))));
      std::vector<ssize_t> x = pos;

      for (x[axis] = 0; x[axis] < ssize_t(values.size()); ++x[axis]) {
        value_type sum = 0.0, sum2 = 0.0;
        do {
          in->get (x, buf, axes_involved[0]);
          for (size_t m = 0; m < buf.size(); ++m) {
            sum += buf[m];
            sum2 += Math::pow2 (buf[m]);
          }
        } while (next (x));
        values[x[axis]] = Math::sqrt (norm*sum2 - Math::pow2 (norm*sum));
      }
    } 
};





EXECUTE {
  Functor* last = new Source (argument[0].get_string());

  for (size_t n = 0; n < option.size(); ++n) {
    if (option[n].is ("datatype")) continue;
    if (option[n].is ("abs")) { last = new Abs (last); continue; }
    if (option[n].is ("neg")) { last = new Neg (last); continue; }
    if (option[n].is ("sqrt")) { last = new Sqrt (last); continue; }
    if (option[n].is ("exp")) { last = new Exp (last); continue; }
    if (option[n].is ("log")) { last = new Log (last); continue; }
    if (option[n].is ("cos")) { last = new Cos (last); continue; }
    if (option[n].is ("sin")) { last = new Sin (last); continue; }
    if (option[n].is ("tan")) { last = new Tan (last); continue; }
    if (option[n].is ("cosh")) { last = new Cosh (last); continue; }
    if (option[n].is ("sinh")) { last = new Sinh (last); continue; }
    if (option[n].is ("tanh")) { last = new Tanh (last); continue; }
    if (option[n].is ("acos")) { last = new Acos (last); continue; }
    if (option[n].is ("asin")) { last = new Asin (last); continue; }
    if (option[n].is ("atan")) { last = new Atan (last); continue; }
    if (option[n].is ("acosh")) { last = new Acosh (last); continue; }
    if (option[n].is ("asinh")) { last = new Asinh (last); continue; }
    if (option[n].is ("atanh")) { last = new Atanh (last); continue; }
    if (option[n].is ("round")) { last = new Round (last); continue; }
    if (option[n].is ("ceil")) { last = new Ceil (last); continue; }
    if (option[n].is ("floor")) { last = new Floor (last); continue; }
    if (option[n].is ("add")) { last = new Add (last, new Source (option[n][0].get_string())); continue; }
    if (option[n].is ("subtract")) { last = new Subtract (last, new Source (option[n][0].get_string())); continue; }
    if (option[n].is ("multiply")) { last = new Mult (last, new Source (option[n][0].get_string())); continue; }
    if (option[n].is ("divide")) { last = new Divide (last, new Source (option[n][0].get_string())); continue; }
    if (option[n].is ("min")) { last = new Min (last, new Source (option[n][0].get_string())); continue; }
    if (option[n].is ("max")) { last = new Max (last, new Source (option[n][0].get_string())); continue; }
    if (option[n].is ("lt")) { last = new LessThan (last, new Source (option[n][0].get_string())); continue; }
    if (option[n].is ("gt")) { last = new GreaterThan (last, new Source (option[n][0].get_string())); continue; }
    if (option[n].is ("le")) { last = new LessThanOrEqualTo (last, new Source (option[n][0].get_string())); continue; }
    if (option[n].is ("ge")) { last = new GreaterThanOrEqualTo (last, new Source (option[n][0].get_string())); continue; }
    if (option[n].is ("mean")) { last = new Mean (last, option[n][0].get_string()); continue; }
    if (option[n].is ("std")) { last = new Std (last, option[n][0].get_string()); continue; }

    assert (0);
  }

  const Image::Header* source_header = last->header();
  if (!source_header) throw Exception ("no source images found - aborting");
  Image::Header header = *source_header;
  header.reset_scaling();

  header.axes.ndim() = last->ndim();
  for (size_t i = 0; i < last->ndim(); ++i)
    header.axes.dim(i) = last->dim(i);

  header.datatype() = DataType::Float32;
  OptionList opt = get_options ("datatype");
  if (opt.size()) 
    header.datatype().parse (DataType::identifiers[opt[0][0].get_int()]);


  const Image::Header destination_header (argument[1].get_image (header));

  Image::Voxel<value_type> out (destination_header);
  std::vector<size_t> axes = DataSet::Stride::order (out);

  Counter count (*last, out, axes[0]);

  axes.erase (axes.begin(), axes.begin()+1);
  DataSet::LoopInOrder loop (axes, "performing mathematical operations...");
  for (loop.start (count, out); loop.ok(); loop.next (count, out)) 
    count.get (out);

  delete last;
}


