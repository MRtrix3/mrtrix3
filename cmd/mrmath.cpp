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

using namespace MR; 

SET_VERSION_DEFAULT;

DESCRIPTION = {
  "apply generic mathematical operations to images.",
  NULL
};


ARGUMENTS = { 
  Argument ("image", "input image", "the input image.").type_image_in (),
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

  Option::End 
};

typedef double value_type;

std::vector<ssize_t> dimension;

void check_dim (const Image::Header& header) {
  try {
    if (header.ndim() != dimension.size()) throw 0;
    for (size_t n = 0; n < dimension.size(); ++n) 
      if (header.dim(n) != dimension[n]) throw 0;
    return;
  }
  catch (...) {
    throw Exception ("mismatch between dimensions of input images");
  }
}

class Functor {
  public:
    Functor (Functor* input) : in (input) { }
    virtual ~Functor () { }
    virtual void get (const std::vector<ssize_t>& pos, std::vector<value_type>& values) { }
    virtual size_t ndim () const { return (0); }
    virtual ssize_t dim (size_t axis) const { return (0); }
  protected:
    Ptr<Functor> in;
};


class Source : public Functor {
  public:
    Source (const Image::Header& header) :
      Functor (NULL),
      V (header) { }
    void get (const std::vector<ssize_t>& pos, std::vector<value_type>& values) {
      ssize_t axis = -1;
      for (size_t n = 0; n < V.ndim(); ++n) {
        if (pos[n] < 0) axis = n;
        else V.pos(n,pos[n]);
      }
      assert (axis >= 0);
      assert (size_t (V.dim(axis)) == values.size());
      for (V.pos(axis,0); V.pos(axis) < V.dim(axis); V.move(axis,1)) 
        values[V.pos(axis)] = V.value();
    }
    virtual size_t ndim () const { return (V.ndim()); }
    virtual ssize_t dim (size_t axis) const { return (V.dim (axis)); }
  private:
    Image::Voxel<value_type> V;
};


class Constant : public Functor {
  public:
    Constant (value_type value) :
      Functor (NULL),
      val (value) { }
    virtual void get (const std::vector<ssize_t>& pos, std::vector<value_type>& values) {
      for (size_t n = 0; n < values.size(); ++n) values[n] = val;
    }
    virtual size_t ndim () const { return (0); }
    virtual ssize_t dim (size_t axis) const { return (0); }
  private:
    value_type val;
};


class Unary : public Functor {
  public:
    Unary (Functor* input) : Functor (input) { }
    virtual size_t ndim () const { return (in->ndim()); }
    virtual ssize_t dim (size_t axis) const { return (in->dim (axis)); }
};

class Binary : public Functor {
  public:
    Binary (Functor* input1, Functor* input2) : Functor (input1), in2 (input2) { }
    virtual size_t ndim () const { assert (in->ndim() == in2->ndim()); return (in->ndim()); }
    virtual ssize_t dim (size_t axis) const { return (in->dim (axis) > 1 ? in->dim(axis) : in2->dim(axis)); }
  protected:
    Ptr<Functor> in2;
    std::vector<value_type> values2;
};


class Kernel {
  public:
    Kernel (Image::Voxel<value_type>& output, Functor* input) :
      out (output), in (input), N (in->ndim()), x (in->ndim()) { 
        for (size_t n = 0; n < N.size(); ++n) {
          x[n] = 0;
          N[n] = in->dim(n);
        }
        x[0] = -1;
        values.resize (N[0]);
      }
    ~Kernel () { ProgressBar::done(); }
    std::string name () { return ("math kernel"); }
    size_t ndim () const { return (N.size()); }
    ssize_t dim (size_t axis) const { return (N[axis]); }
    ssize_t pos (size_t axis) const { return (x[axis]); }
    void pos (size_t axis, ssize_t position) { x[axis] = position; out.pos (axis, position); }
    void move (size_t axis, ssize_t inc) { x[axis] += inc; out.move (axis, inc); }
    void check (size_t from_axis, size_t to_axis) const { 
      if (!DataSet::dimensions_match (out, *this))
        throw Exception ("dimensions mismatch in math kernel");
      ProgressBar::init (DataSet::voxel_count (*this, 1, SIZE_MAX), "performing mathematical operations...");
    }
    void operator() () { 
      in->get (x, values);
      for (out.pos(0,0); out.pos(0) < N[0]; out.move(0,1)) 
        out.value (values[out.pos(0)]);
      ProgressBar::inc();
    }
  protected:
    Image::Voxel<value_type>& out;
    Functor* in;
    std::vector<ssize_t> N, x;
    std::vector<value_type> values;
};

#define DEF_UNARY(name, operation) class name : public Unary { \
  public: \
    name (Functor* input) : Unary (input) { } \
    virtual void get (const std::vector<ssize_t>& pos, std::vector<value_type>& values) { \
      in->get (pos, values); \
      for (ssize_t n = 0; n < dimension[0]; ++n) { \
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
      for (ssize_t n = 0; n < dimension[0]; ++n) { \
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

DEF_BINARY (Mult, val1*val2);

EXECUTE {
  const Image::Header header_in (argument[0].get_image());
  Image::Header header (header_in);

  header.datatype() = DataType::Float32;
  OptionList opt = get_options (0); // datatype
  if (opt.size()) header.datatype().parse (DataType::identifiers[opt[0][0].get_int()]);

  Image::Header header_out (argument[1].get_image (header));
  dimension.resize (header.ndim());
  for (size_t n = 0; n < header.ndim(); ++n) 
    dimension[n] = header.dim(n);

  Functor* last = new Source (header_in);

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
      default: assert (0);
    }
  }

  Image::Voxel<value_type> vox (header_out);
  Kernel kernel (vox, last);
  DataSet::loop (kernel,1);

  delete last;
}


