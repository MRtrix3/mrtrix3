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
  Option ("datatype", "data type", "specify output image data type.") // 0
    .append (Argument ("spec", "specifier", "the data type specifier.").type_choice (DataType::identifiers)),

  Option ("abs", "absolute value", "take absolute value of current voxel value"), // 1
  Option ("neg", "negative value", "take negative value of current voxel value"), // 2
  Option ("sqrt", "square root", "take square root of current voxel value"), // 3
  Option ("exp", "exp", "take e raised to the power of current voxel value"), // 4
  Option ("log", "log", "take natural logarithm of current voxel value"), // 5
  Option ("cos", "cos", "take cosine of current voxel value"), // 6
  Option ("sin", "sin", "take sine of current voxel value"), // 7
  Option ("tan", "tanh", "take tangent of current voxel value"), // 8
  Option ("cosh", "cosh", "take hyperbolic cosine of current voxel value"), // 9
  Option ("sinh", "sinh", "take hyperbolic sine of current voxel value"), // 10
  Option ("tanh", "tanh", "take hyperbolic tangent of current voxel value"), // 11
  Option ("acos", "acos", "take inverse cosine of current voxel value"), // 12
  Option ("asin", "asin", "take inverse sine of current voxel value"), // 13
  Option ("atan", "atan", "take inverse tangent of current voxel value"), // 14
  Option ("acosh", "acosh", "take inverse hyperbolic cosine of current voxel value"), // 15
  Option ("asinh", "asinh", "take inverse hyperbolic sine of current voxel value"), // 16
  Option ("atanh", "atanh", "take inverse hyperbolic tangent of current voxel value"), // 17
  Option ("round", "round", "round current voxel value to nearest integer"), // 18
  Option ("ceil", "ceil", "round current voxel value up to smallest integer not less than current value"), // 19
  Option ("floor", "floor", "round current voxel value down to largest integer not greater than current value"), // 20

  Option ("add", "add", "add to current voxel value the corresponding voxel value of 'source'") // 21
    .append (Argument ("source", "source", "the source image or value").type_string()),

  Option ("subtract", "subtract", "subtract from current voxel value the corresponding voxel value of 'source'") // 22
    .append (Argument ("source", "source", "the source image or value").type_string()),

  Option ("multiply", "multiply", "multiply current voxel value by corresponding voxel value of 'source'") // 23
    .append (Argument ("source", "source", "the source image or value").type_string()),

  Option ("divide", "divide", "divide current voxel value by corresponding voxel value of 'source'") // 24
    .append (Argument ("source", "source", "the source image or value").type_string()),

  Option ("min", "min", "return smallest of current voxel value and corresponding voxel value of 'source'") // 25
    .append (Argument ("source", "source", "the source image or value").type_string()),

  Option ("max", "max", "return greatest of current voxel value and corresponding voxel value of 'source'") // 26
    .append (Argument ("source", "source", "the source image or value").type_string()),

  Option ("lt", "less than", "return 1 if current voxel value is less than corresponding voxel value of 'source', 0 otherwise") // 27
    .append (Argument ("source", "source", "the source image or value").type_string()),

  Option ("gt", "greater than", "return 1 if current voxel value is greater than corresponding voxel value of 'source', 0 otherwise") // 28
    .append (Argument ("source", "source", "the source image or value").type_string()),

  Option ("le", "less than or equal to", "return 1 if current voxel value is less than or equal to corresponding voxel value of 'source', 0 otherwise") // 29
    .append (Argument ("source", "source", "the source image or value").type_string()),

  Option ("ge", "greater than or equal to", "return 1 if current voxel value is greater than or equal to corresponding voxel value of 'source', 0 otherwise") // 30
    .append (Argument ("source", "source", "the source image or value").type_string()),

  Option::End 
};

typedef double value_type;

class Functor {
  public:
    Functor (Functor* input) : in (input) { }
    virtual ~Functor () { }
    virtual void get (const std::vector<ssize_t>& pos, std::vector<value_type>& values) { }
    virtual size_t ndim () const { return (0); }
    virtual ssize_t dim (size_t axis) const { return (0); }
    virtual const Image::Header* header () const { return (NULL); }

    static size_t inner_axis;
  protected:
    Ptr<Functor> in;
};
size_t Functor::inner_axis = 0;





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
    void get (const std::vector<ssize_t>& pos, std::vector<value_type>& values) {
      if (V) {
        assert (size_t (V->dim(inner_axis)) == values.size());
        for (size_t i = 0; i < V->ndim(); ++i) 
          if (i != inner_axis && V->dim(i) > 1) (*V)[i] = pos[i]; 
        for ((*V)[inner_axis] = 0; (*V)[inner_axis] < V->dim(inner_axis); ++(*V)[inner_axis]) 
          values[(*V)[inner_axis]] = V->value();
      }
      else 
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






class Unary : public Functor {
  public:
    Unary (Functor* input) : Functor (input) { }
    virtual size_t ndim () const { return (in->ndim()); }
    virtual ssize_t dim (size_t axis) const { return (in->dim (axis)); }
    virtual const Image::Header* header () const { return (in->header()); }
};




class Binary : public Functor {
  public:
    Binary (Functor* input1, Functor* input2) : Functor (input1), in2 (input2) { 
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
    virtual ssize_t dim (size_t axis) const { 
      ssize_t d1 = axis < in->ndim() ? in->dim(axis) : 1;
      ssize_t d2 = axis < in2->ndim() ? in2->dim(axis) : 1;
      return (std::max (d1, d2));
    }
    virtual const Image::Header* header () const { return (in->header() ? in->header() : in2->header()); }
  protected:
    Ptr<Functor> in2;
    std::vector<value_type> values2;
};




class Counter {
  public:
    template <class Set> 
      Counter (Functor& input, Set& output) : in (input), N_ (output.ndim()), x_ (output.ndim()) { 
        for (size_t n = 0; n < N_.size(); ++n) {
          x_[n] = 0;
          N_[n] = output.dim(n);
        }
        values.resize (N_[Functor::inner_axis]);
      }
    size_t ndim () const { return (N_.size()); }
    ssize_t dim (size_t axis) const { return (axis == Functor::inner_axis ? 1 : N_[axis]); }
    ssize_t& operator[] (size_t axis) { return (x_[axis]); }
    template <class Set> 
      void get (Set& out) {
        in.get (x_, values);
        for (out[Functor::inner_axis] = 0; out[Functor::inner_axis] < out.dim(Functor::inner_axis); ++out[Functor::inner_axis]) 
          out.value() = values[out[Functor::inner_axis]];
      }
  private:
    Functor& in;
    std::vector<ssize_t> N_, x_;
    std::vector<value_type> values;
};




#define DEF_UNARY(name, operation) class name : public Unary { \
  public: \
    name (Functor* input) : Unary (input) { } \
    virtual void get (const std::vector<ssize_t>& pos, std::vector<value_type>& values) { \
      in->get (pos, values); \
      for (size_t n = 0; n < values.size(); ++n) { \
        value_type& val = values[n]; \
        values[n] = operation; \
      } \
    } \
}

#define DEF_BINARY(name, operation) class name : public Binary { \
  public: \
    name (Functor* input1, Functor* input2) : Binary (input1, input2) { } \
    virtual void get (const std::vector<ssize_t>& pos, std::vector<value_type>& values) { \
      values2.resize (values.size()); \
      in->get (pos, values); \
      in2->get (pos, values2); \
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

EXECUTE {
  Functor* last = new Source (argument[0].get_string());

  for (size_t n = 0; n < option.size(); ++n) {
    switch (option[n].index) {
      case 0: continue;
      case 1: last = new Abs (last); break;
      case 2: last = new Neg (last); break;
      case 3: last = new Sqrt (last); break;
      case 4: last = new Exp (last); break;
      case 5: last = new Log (last); break;
      case 6: last = new Cos (last); break;
      case 7: last = new Sin (last); break;
      case 8: last = new Tan (last); break;
      case 9: last = new Cosh (last); break;
      case 10: last = new Sinh (last); break;
      case 11: last = new Tanh (last); break;
      case 12: last = new Acos (last); break;
      case 13: last = new Asin (last); break;
      case 14: last = new Atan (last); break;
      case 15: last = new Acosh (last); break;
      case 16: last = new Asinh (last); break;
      case 17: last = new Atanh (last); break;
      case 18: last = new Round (last); break;
      case 19: last = new Ceil (last); break;
      case 20: last = new Floor (last); break;
      case 21: last = new Add (last, new Source (option[n][0].get_string())); break;
      case 22: last = new Subtract (last, new Source (option[n][0].get_string())); break;
      case 23: last = new Mult (last, new Source (option[n][0].get_string())); break;
      case 24: last = new Divide (last, new Source (option[n][0].get_string())); break;
      case 25: last = new Min (last, new Source (option[n][0].get_string())); break;
      case 26: last = new Max (last, new Source (option[n][0].get_string())); break;
      case 27: last = new LessThan (last, new Source (option[n][0].get_string())); break;
      case 28: last = new GreaterThan (last, new Source (option[n][0].get_string())); break;
      case 29: last = new LessThanOrEqualTo (last, new Source (option[n][0].get_string())); break;
      case 30: last = new GreaterThanOrEqualTo (last, new Source (option[n][0].get_string())); break;
      default: assert (0);
    }
  }

  const Image::Header* source_header = last->header();
  if (!source_header) throw Exception ("no source images found - aborting");
  Image::Header header = *source_header;

  header.axes.ndim() = last->ndim();
  for (size_t i = 0; i < last->ndim(); ++i)
    header.axes.dim(i) = last->dim(i);

  header.datatype() = DataType::Float32;
  OptionList opt = get_options (0); // datatype
  if (opt.size()) header.datatype().parse (DataType::identifiers[opt[0][0].get_int()]);


  Image::Header destination_header (argument[1].get_image (header));

  Image::Voxel<value_type> out (destination_header);
  Functor::inner_axis = DataSet::Stride::order (out)[0];

  Counter count (*last, out);

  DataSet::Loop loop ("performing mathematical operations...");
  for (loop.start (count, out); loop.ok(); loop.next (count, out)) {
    count.get (out);
  }

  delete last;
}


