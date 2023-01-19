/* Copyright (c) 2008-2023 the MRtrix3 contributors.
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




/**********************************************************************
  CONVENIENCE MACROS:
 **********************************************************************/

#ifdef SECTION

#undef SECTION_TITLE
#undef UNARY_OP
#undef BINARY_OP
#undef TERNARY_OP

# if SECTION == 1 // usage section

#define SECTION_TITLE(TITLE) \
  + OptionGroup (TITLE)

#define UNARY_OP(OPTION,FEEDBACK,FLAGS,DESCRIPTION,REAL_OPERATION,COMPLEX_OPERATION) \
  + Option (#OPTION, FEEDBACK " : " DESCRIPTION).allow_multiple()

#define BINARY_OP(OPTION,FEEDBACK,FLAGS,DESCRIPTION,REAL_OPERATION,COMPLEX_OPERATION) \
  + Option (#OPTION, FEEDBACK " : " DESCRIPTION).allow_multiple()

#define TERNARY_OP(OPTION,FEEDBACK,FLAGS,DESCRIPTION,REAL_OPERATION,COMPLEX_OPERATION) \
  + Option (#OPTION, FEEDBACK " : " DESCRIPTION).allow_multiple()

# elif SECTION == 2 // code section

#define SECTION_TITLE(TITLE)

#define UNARY_OP(OPTION,FEEDBACK,FLAGS,DESCRIPTION,REAL_OPERATION,COMPLEX_OPERATION) \
  class Op_##OPTION : public OpUnary { NOMEMALIGN \
    public: \
      Op_##OPTION () : OpUnary (FEEDBACK, FLAGS & COMPLEX_MAPS_TO_REAL, FLAGS & REAL_MAPS_TO_COMPLEX) { } \
      complex_type R (real_type v) const REAL_OPERATION \
      complex_type Z (complex_type v) const COMPLEX_OPERATION \
  };

#define BINARY_OP(OPTION,FEEDBACK,FLAGS,DESCRIPTION,REAL_OPERATION,COMPLEX_OPERATION) \
  class Op_##OPTION : public OpBinary { NOMEMALIGN \
    public: \
      Op_##OPTION () : OpBinary (FEEDBACK, FLAGS & COMPLEX_MAPS_TO_REAL, FLAGS & REAL_MAPS_TO_COMPLEX) { } \
      complex_type R (real_type a, real_type b) const REAL_OPERATION \
      complex_type Z (complex_type a, complex_type b) const COMPLEX_OPERATION \
  };

#define TERNARY_OP(OPTION,FEEDBACK,FLAGS,DESCRIPTION,REAL_OPERATION,COMPLEX_OPERATION) \
  class Op_##OPTION : public OpTernary { NOMEMALIGN \
    public: \
      Op_##OPTION () : OpTernary (FEEDBACK, FLAGS & COMPLEX_MAPS_TO_REAL, FLAGS & REAL_MAPS_TO_COMPLEX) { } \
      complex_type R (real_type a, real_type b, real_type c) const REAL_OPERATION \
      complex_type Z (complex_type a, complex_type b, complex_type c) const COMPLEX_OPERATION \
  };

# elif SECTION == 3 // parsing section

#define SECTION_TITLE(TITLE)

#define UNARY_OP(OPTION,FEEDBACK,FLAGS,DESCRIPTION,REAL_OPERATION,COMPLEX_OPERATION) \
  else if (opt->is (#OPTION)) unary_operation (opt->id, stack, Op_##OPTION());

#define BINARY_OP(OPTION,FEEDBACK,FLAGS,DESCRIPTION,REAL_OPERATION,COMPLEX_OPERATION) \
  else if (opt->is (#OPTION)) binary_operation (opt->id, stack, Op_##OPTION());

#define TERNARY_OP(OPTION,FEEDBACK,FLAGS,DESCRIPTION,REAL_OPERATION,COMPLEX_OPERATION) \
  else if (opt->is (#OPTION)) ternary_operation (opt->id, stack, Op_##OPTION());

# endif

#define NORMAL 0U
#define COMPLEX_MAPS_TO_REAL 1U
#define REAL_MAPS_TO_COMPLEX 2U
#define NOT_IMPLEMENTED { throw Exception ("operation not supported"); }



/**********************************************************************
  Operations defined below:
 **********************************************************************/

SECTION_TITLE ("basic operations")
UNARY_OP (abs, "|%1|", COMPLEX_MAPS_TO_REAL, "return absolute value (magnitude) of real or complex number", { return abs (v); }, { return abs (v); } )
UNARY_OP (neg, "-%1", NORMAL, "negative value", { return -v; }, { return -v; })
BINARY_OP (add, "(%1 + %2)", NORMAL, "add values", { return a+b; }, { return a+b; })
BINARY_OP (subtract, "(%1 - %2)", NORMAL, "subtract nth operand from (n-1)th", { return a-b; }, { return a-b; })
BINARY_OP (multiply, "(%1 * %2)", NORMAL, "multiply values", { return a*b; }, { return a*b; })
BINARY_OP (divide, "(%1 / %2)", NORMAL, "divide (n-1)th operand by nth", { return a/b; }, { return a/b; })
BINARY_OP (min, "min (%1, %2)", NORMAL, "smallest of last two operands", { return std::min (a, b); }, NOT_IMPLEMENTED)
BINARY_OP (max, "max (%1, %2)", NORMAL, "greatest of last two operands", { return std::max (a, b); }, NOT_IMPLEMENTED)

SECTION_TITLE ("comparison operators")
BINARY_OP (lt, "(%1 < %2)", NORMAL, "less-than operator (true=1, false=0)", { return a < b; }, NOT_IMPLEMENTED)
BINARY_OP (gt, "(%1 > %2)", NORMAL, "greater-than operator (true=1, false=0)", { return a > b; }, NOT_IMPLEMENTED)
BINARY_OP (le, "(%1 <= %2)", NORMAL, "less-than-or-equal-to operator (true=1, false=0)", { return a <= b; }, NOT_IMPLEMENTED)
BINARY_OP (ge, "(%1 >= %2)", NORMAL, "greater-than-or-equal-to operator (true=1, false=0)", { return a >= b; }, NOT_IMPLEMENTED)
BINARY_OP (eq, "(%1 == %2)", COMPLEX_MAPS_TO_REAL, "equal-to operator (true=1, false=0)", { return a == b; }, { return a == b; })
BINARY_OP (neq, "(%1 != %2)", COMPLEX_MAPS_TO_REAL, "not-equal-to operator (true=1, false=0)", { return a != b; }, { return a != b; })

SECTION_TITLE ("conditional operators")
TERNARY_OP (if, "(%1 ? %2 : %3)", NORMAL, "if first operand is true (non-zero), return second operand, otherwise return third operand", { return a ? b : c; }, { return is_true(a) ? b : c; })
TERNARY_OP (replace, "(%1, %2 -> %3)", NORMAL, "Wherever first operand is equal to the second operand, replace with third operand", { return (a==b) ? c : a; }, { return (a==b) ? c : a; })

SECTION_TITLE ("power functions")
UNARY_OP (sqrt, "sqrt (%1)", NORMAL, "square root", { return std::sqrt (v); }, { return std::sqrt (v); })
BINARY_OP (pow, "%1^%2", NORMAL, "raise (n-1)th operand to nth power", { return std::pow (a, b); }, { return std::pow (a, b); })

SECTION_TITLE ("nearest integer operations")
UNARY_OP (round, "round (%1)", NORMAL, "round to nearest integer", { return std::round (v); }, NOT_IMPLEMENTED)
UNARY_OP (ceil, "ceil (%1)", NORMAL, "round up to nearest integer", { return std::ceil (v); }, NOT_IMPLEMENTED)
UNARY_OP (floor, "floor (%1)", NORMAL, "round down to nearest integer", { return std::floor (v); }, NOT_IMPLEMENTED)

SECTION_TITLE ("logical operators")
UNARY_OP (not, "!%1", NORMAL, "NOT operator: true (1) if operand is false (i.e. zero)", { return !v; }, { return !is_true (v); })
BINARY_OP (and, "(%1 && %2)", NORMAL, "AND operator: true (1) if both operands are true (i.e. non-zero)", { return a && b; }, { return is_true(a) && is_true(b); })
BINARY_OP (or, "(%1 || %2)", NORMAL, "OR operator: true (1) if either operand is true (i.e. non-zero)", { return a || b; }, { return is_true(a) || is_true(b); })
BINARY_OP (xor, "(%1 ^^ %2)", NORMAL, "XOR operator: true (1) if only one of the operands is true (i.e. non-zero)", { return (!a) != (!b); }, { return is_true(a) != is_true(b); })

SECTION_TITLE ("classification functions")
UNARY_OP (isnan, "isnan (%1)", COMPLEX_MAPS_TO_REAL, "true (1) if operand is not-a-number (NaN)", { return std::isnan (v); }, { return std::isnan (v.real()) || std::isnan (v.imag()); })
UNARY_OP (isinf, "isinf (%1)", COMPLEX_MAPS_TO_REAL, "true (1) if operand is infinite (Inf)", { return std::isinf (v); }, { return std::isinf (v.real()) || std::isinf (v.imag()); })
UNARY_OP (finite, "finite (%1)", COMPLEX_MAPS_TO_REAL, "true (1) if operand is finite (i.e. not NaN or Inf)", { return std::isfinite (v); }, { return std::isfinite (v.real()) && std::isfinite (v.imag()); })

SECTION_TITLE ("complex numbers")
BINARY_OP (complex, "(%1 + %2 i)", REAL_MAPS_TO_COMPLEX, "create complex number using the last two operands as real,imaginary components", { return complex_type (a, b); }, NOT_IMPLEMENTED)
BINARY_OP (polar, "(%1 /_ %2)", REAL_MAPS_TO_COMPLEX, "create complex number using the last two operands as magnitude,phase components (phase in radians)", { return std::polar (a, b); }, NOT_IMPLEMENTED)
UNARY_OP (real, "real (%1)", COMPLEX_MAPS_TO_REAL, "real part of complex number", { return v; }, { return v.real(); })
UNARY_OP (imag, "imag (%1)", COMPLEX_MAPS_TO_REAL, "imaginary part of complex number", { return 0.0; }, { return v.imag(); })
UNARY_OP (phase, "phase (%1)", COMPLEX_MAPS_TO_REAL, "phase of complex number (use -abs for magnitude)", { return v < 0.0 ? Math::pi : 0.0; }, { return std::arg (v); })
UNARY_OP (conj, "conj (%1)", NORMAL, "complex conjugate", { return v; }, { return std::conj (v); })
UNARY_OP (proj, "proj (%1)", REAL_MAPS_TO_COMPLEX, "projection onto the Riemann sphere", { return std::proj (v); }, { return std::proj (v); })

SECTION_TITLE ("exponential functions")
UNARY_OP (exp, "exp (%1)", NORMAL, "exponential function", { return std::exp (v); }, { return std::exp (v); })
UNARY_OP (log, "log (%1)", NORMAL, "natural logarithm", { return std::log (v); }, { return std::log (v); })
UNARY_OP (log10, "log10 (%1)", NORMAL, "common logarithm", { return std::log10 (v); }, { return std::log10 (v); })

SECTION_TITLE ("trigonometric functions")
UNARY_OP (cos, "cos (%1)", NORMAL, "cosine", { return std::cos (v); }, { return std::cos (v); })
UNARY_OP (sin, "sin (%1)", NORMAL, "sine", { return std::sin (v); }, { return std::sin (v); })
UNARY_OP (tan, "tan (%1)", NORMAL, "tangent", { return std::tan (v); }, { return std::tan (v); })
UNARY_OP (acos, "acos (%1)", NORMAL, "inverse cosine", { return std::acos (v); }, { return std::acos (v); })
UNARY_OP (asin, "asin (%1)", NORMAL, "inverse sine", { return std::asin (v); }, { return std::asin (v); })
UNARY_OP (atan, "atan (%1)", NORMAL, "inverse tangent", { return std::atan (v); }, { return std::atan (v); })

SECTION_TITLE ("hyperbolic functions")
UNARY_OP (cosh, "cosh (%1)", NORMAL, "hyperbolic cosine", { return std::cosh (v); }, { return std::cosh (v); })
UNARY_OP (sinh, "sinh (%1)", NORMAL, "hyperbolic sine", { return std::sinh (v); }, { return std::sinh (v); })
UNARY_OP (tanh, "tanh (%1)", NORMAL, "hyperbolic tangent", { return std::tanh (v); }, { return std::tanh (v); })
UNARY_OP (acosh, "acosh (%1)", NORMAL, "inverse hyperbolic cosine", { return std::acosh (v); }, { return std::acosh (v); })
UNARY_OP (asinh, "asinh (%1)", NORMAL, "inverse hyperbolic sine", { return std::asinh (v); }, { return std::asinh (v); })
UNARY_OP (atanh, "atanh (%1)", NORMAL, "inverse hyperbolic tangent", { return std::atanh (v); }, { return std::atanh (v); })



#undef SECTION

#else


/**********************************************************************
  Main program
 **********************************************************************/

#include "command.h"
#include "image.h"
#include "memory.h"
#include "phase_encoding.h"
#include "math/rng.h"
#include "algo/threaded_copy.h"
#include "dwi/gradient.h"

using namespace MR;
using namespace App;


using real_type = float;
using complex_type = cfloat;
static bool transform_mis_match_reported (false);

inline bool is_true (const complex_type& z) { return z.real() || z.imag(); }


void usage () {

AUTHOR = "J-Donald Tournier (jdtournier@gmail.com)";

SYNOPSIS = "Apply generic voxel-wise mathematical operations to images";

DESCRIPTION
  + "This command will only compute per-voxel operations. "
  "Use 'mrmath' to compute summary statistics across images or "
  "along image axes."

  + "This command uses a stack-based syntax, with operators "
  "(specified using options) operating on the top-most entries "
  "(i.e. images or values) in the stack. Operands (values or "
  "images) are pushed onto the stack in the order they appear "
  "(as arguments) on the command-line, and operators (specified "
  "as options) operate on and consume the top-most entries in "
  "the stack, and push their output as a new entry on the stack."

  + "As an additional feature, this command will allow images with different "
  "dimensions to be processed, provided they satisfy the following "
  "conditions: for each axis, the dimensions match if they are the same size, "
  "or one of them has size one. In the latter case, the entire image will be "
  "replicated along that axis. This allows for example a 4D image of "
  "size [ X Y Z N ] to be added to a 3D image of size [ X Y Z ], as if it "
  "consisted of N copies of the 3D image along the 4th axis (the missing "
  "dimension is assumed to have size 1). Another example would a "
  "single-voxel 4D image of size [ 1 1 1 N ], multiplied by a 3D image of "
  "size [ X Y Z ], which would allow the creation of a 4D image where each "
  "volume consists of the 3D image scaled by the corresponding value for "
  "that volume in the single-voxel image.";

EXAMPLES
  + Example ("Double the value stored in every voxel",
             "mrcalc a.mif 2 -mult r.mif",
             "This performs the operation: r = 2*a  for every voxel a,r in "
             "images a.mif and r.mif respectively.")

  + Example ("A more complex example",
             "mrcalc a.mif -neg b.mif -div -exp 9.3 -mult r.mif",
             "This performs the operation: r = 9.3*exp(-a/b)")

  + Example ("Another complex example",
             "mrcalc a.mif b.mif -add c.mif d.mif -mult 4.2 -add -div r.mif",
             "This performs: r = (a+b)/(c*d+4.2).")

  + Example ("Rescale the densities in a SH l=0 image",
             "mrcalc ODF_CSF.mif 4 pi -mult -sqrt -div ODF_CSF_scaled.mif",
             "This applies the spherical harmonic basis scaling factor: "
             "1.0/sqrt(4*pi), such that a single-tissue voxel containing the "
             "same intensities as the response function of that tissue "
             "should contain the value 1.0.");

ARGUMENTS
  + Argument ("operand", "an input image, intensity value, or the special keywords "
      "'rand' (random number between 0 and 1) or 'randn' (random number from unit "
      "std.dev. normal distribution) or the mathematical constants 'e' and 'pi'.").type_various().allow_multiple();

OPTIONS

#define SECTION 1
#include "mrcalc.cpp"

  + DataType::options();
}




/**********************************************************************
  STACK FRAMEWORK:
 **********************************************************************/


class Evaluator;


class Chunk : public vector<complex_type> { NOMEMALIGN
  public:
    complex_type value;
};


class ThreadLocalStorageItem { NOMEMALIGN
  public:
    Chunk chunk;
    copy_ptr<Image<complex_type>> image;
};

class ThreadLocalStorage : public vector<ThreadLocalStorageItem> { NOMEMALIGN
  public:

      void load (Chunk& chunk, Image<complex_type>& image) {
        for (size_t n = 0; n < image.ndim(); ++n)
          if (image.size(n) > 1)
            image.index(n) = iter->index(n);

        size_t n = 0;
        for (size_t y = 0; y < size[1]; ++y) {
          if (axes[1] < image.ndim()) if (image.size (axes[1]) > 1) image.index(axes[1]) = y;
          for (size_t x = 0; x < size[0]; ++x) {
            if (axes[0] < image.ndim()) if (image.size (axes[0]) > 1) image.index(axes[0]) = x;
            chunk[n++] = image.value();
          }
        }
      }

    Chunk& next () {
      ThreadLocalStorageItem& item ((*this)[current++]);
      if (item.image) load (item.chunk, *item.image);
      return item.chunk;
    }

    void reset (const Iterator& current_position) { current = 0; iter = &current_position; }

    const Iterator* iter;
    vector<size_t> axes, size;

  private:
    size_t current;
};




class LoadedImage { NOMEMALIGN
  public:
    LoadedImage (std::shared_ptr<Image<complex_type>>& i, const bool c) :
        image (i),
        image_is_complex (c) { }
    std::shared_ptr<Image<complex_type>> image;
    bool image_is_complex;
};




class StackEntry { NOMEMALIGN
  public:

    StackEntry (const char* entry) :
        arg (entry),
        rng_gaussian (false),
        image_is_complex (false) { }

    StackEntry (Evaluator* evaluator_p) :
        arg (nullptr),
        evaluator (evaluator_p),
        rng_gaussian (false),
        image_is_complex (false) { }

    void load () {
      if (!arg)
        return;
      auto search = image_list.find (arg);
      if (search != image_list.end()) {
        DEBUG (std::string ("image \"") + arg + "\" already loaded - re-using exising image");
        image = search->second.image;
        image_is_complex = search->second.image_is_complex;
      }
      else {
        try {
          auto header = Header::open (arg);
          image_is_complex = header.datatype().is_complex();
          image.reset (new Image<complex_type> (header.get_image<complex_type>()));
          image_list.insert (std::make_pair (arg, LoadedImage (image, image_is_complex)));
        }
        catch (Exception& e_image) {
          try {
            std::string a = lowercase (arg);
            if      (a == "pi")    { value = Math::pi; }
            else if (a == "e")     { value = Math::e; }
            else if (a == "rand")  { value = 0.0; rng.reset (new Math::RNG()); rng_gaussian = false; }
            else if (a == "randn") { value = 0.0; rng.reset (new Math::RNG()); rng_gaussian = true; }
            else                   { value =  to<complex_type> (arg); }
          } catch (Exception& e_number) {
            Exception e (std::string ("Could not interpret string \"") + arg + "\" as either an image path or a numerical value");
            e.push_back ("As image: ");
            for (size_t i = 0; i != e_image.num(); ++i)
              e.push_back (e_image[i]);
            e.push_back ("As numerical value: ");
            for (size_t i = 0; i != e_number.num(); ++i)
              e.push_back (e_number[i]);
            throw e;
          }
        }
      }
      arg = nullptr;
    }

    const char* arg;
    std::shared_ptr<Evaluator> evaluator;
    std::shared_ptr<Image<complex_type>> image;
    copy_ptr<Math::RNG> rng;
    complex_type value;
    bool rng_gaussian;
    bool image_is_complex;

    bool is_complex () const;

    static std::map<std::string, LoadedImage> image_list;

    Chunk& evaluate (ThreadLocalStorage& storage) const;
};

std::map<std::string, LoadedImage> StackEntry::image_list;


class Evaluator { NOMEMALIGN
  public:
    Evaluator (const std::string& name, const char* format_string, bool complex_maps_to_real = false, bool real_maps_to_complex = false) :
      id (name),
      format (format_string),
      ZtoR (complex_maps_to_real),
      RtoZ (real_maps_to_complex) { }
    virtual ~Evaluator() { }
    const std::string id;
    const char* format;
    bool ZtoR, RtoZ;
    vector<StackEntry> operands;

    Chunk& evaluate (ThreadLocalStorage& storage) const {
      Chunk& in1 (operands[0].evaluate (storage));
      if (num_args() == 1) return evaluate (in1);
      Chunk& in2 (operands[1].evaluate (storage));
      if (num_args() == 2) return evaluate (in1, in2);
      Chunk& in3 (operands[2].evaluate (storage));
      return evaluate (in1, in2, in3);
    }
    virtual Chunk& evaluate (Chunk& in) const { throw Exception ("operation \"" + id + "\" not supported!"); return in; }
    virtual Chunk& evaluate (Chunk& a, Chunk& b) const { throw Exception ("operation \"" + id + "\" not supported!"); return a; }
    virtual Chunk& evaluate (Chunk& a, Chunk& b, Chunk& c) const { throw Exception ("operation \"" + id + "\" not supported!"); return a; }

    virtual bool is_complex () const {
      for (size_t n = 0; n < operands.size(); ++n)
        if (operands[n].is_complex())
          return !ZtoR;
      return RtoZ;
    }
    size_t num_args () const { return operands.size(); }

};



inline bool StackEntry::is_complex () const {
  if (image) return image_is_complex;
  if (evaluator) return evaluator->is_complex();
  if (rng) return false;
  return value.imag() != 0.0;
}



inline Chunk& StackEntry::evaluate (ThreadLocalStorage& storage) const
{
  if (evaluator) return evaluator->evaluate (storage);
  if (rng) {
    Chunk& chunk = storage.next();
    if (rng_gaussian) {
      std::normal_distribution<real_type> dis (0.0, 1.0);
      for (size_t n = 0; n < chunk.size(); ++n)
        chunk[n] = dis (*rng);
    }
    else {
      std::uniform_real_distribution<real_type> dis (0.0, 1.0);
      for (size_t n = 0; n < chunk.size(); ++n)
        chunk[n] = dis (*rng);
    }
    return chunk;
  }
  return storage.next();
}







inline void replace (std::string& orig, size_t n, const std::string& value)
{
  if (orig[0] == '(' && orig[orig.size()-1] == ')') {
    size_t pos = orig.find ("(%"+str(n+1)+")");
    if (pos != orig.npos) {
      orig.replace (pos, 4, value);
      return;
    }
  }

  size_t pos = orig.find ("%"+str(n+1));
  if (pos != orig.npos)
    orig.replace (pos, 2, value);
}


// TODO: move this into StackEntry class and compute string at construction
// to make sure full operation is recorded, even for scalar operations that
// get evaluated there and then and so get left out if the string is created
// later:
std::string operation_string (const StackEntry& entry)
{
  if (entry.image)
    return entry.image->name();
  else if (entry.rng)
    return entry.rng_gaussian ? "randn()" : "rand()";
  else if (entry.evaluator) {
    std::string s = entry.evaluator->format;
    for (size_t n = 0; n < entry.evaluator->operands.size(); ++n)
      replace (s, n, operation_string (entry.evaluator->operands[n]));
    return s;
  }
  else return str(entry.value);
}





template <class Operation>
class UnaryEvaluator : public Evaluator { NOMEMALIGN
  public:
    UnaryEvaluator (const std::string& name, Operation operation, const StackEntry& operand) :
      Evaluator (name, operation.format, operation.ZtoR, operation.RtoZ),
      op (operation) {
        operands.push_back (operand);
      }

    Operation op;

    virtual Chunk& evaluate (Chunk& in) const {
      if (operands[0].is_complex())
        for (size_t n = 0; n < in.size(); ++n)
          in[n] = op.Z (in[n]);
      else
        for (size_t n = 0; n < in.size(); ++n)
          in[n] = op.R (in[n].real());

      return in;
    }
};






template <class Operation>
class BinaryEvaluator : public Evaluator { NOMEMALIGN
  public:
    BinaryEvaluator (const std::string& name, Operation operation, const StackEntry& operand1, const StackEntry& operand2) :
      Evaluator (name, operation.format, operation.ZtoR, operation.RtoZ),
      op (operation) {
        operands.push_back (operand1);
        operands.push_back (operand2);
      }

    Operation op;

    virtual Chunk& evaluate (Chunk& a, Chunk& b) const {
      Chunk& out (a.size() ? a : b);
      if (operands[0].is_complex() || operands[1].is_complex()) {
        for (size_t n = 0; n < out.size(); ++n)
          out[n] = op.Z (
              a.size() ? a[n] : a.value,
              b.size() ? b[n] : b.value );
      }
      else {
        for (size_t n = 0; n < out.size(); ++n)
          out[n] = op.R (
              a.size() ? a[n].real() : a.value.real(),
              b.size() ? b[n].real() : b.value.real() );
      }
      return out;
    }

};



template <class Operation>
class TernaryEvaluator : public Evaluator { NOMEMALIGN
  public:
    TernaryEvaluator (const std::string& name, Operation operation, const StackEntry& operand1, const StackEntry& operand2, const StackEntry& operand3) :
      Evaluator (name, operation.format, operation.ZtoR, operation.RtoZ),
      op (operation) {
        operands.push_back (operand1);
        operands.push_back (operand2);
        operands.push_back (operand3);
      }

    Operation op;

    virtual Chunk& evaluate (Chunk& a, Chunk& b, Chunk& c) const {
      Chunk& out (a.size() ? a : (b.size() ? b : c));
      if (operands[0].is_complex() || operands[1].is_complex() || operands[2].is_complex()) {
        for (size_t n = 0; n < out.size(); ++n)
          out[n] = op.Z (
              a.size() ? a[n] : a.value,
              b.size() ? b[n] : b.value,
              c.size() ? c[n] : c.value );
      }
      else {
        for (size_t n = 0; n < out.size(); ++n)
          out[n] = op.R (
              a.size() ? a[n].real() : a.value.real(),
              b.size() ? b[n].real() : b.value.real(),
              c.size() ? c[n].real() : c.value.real() );
      }
      return out;
    }

};





template <class Operation>
void unary_operation (const std::string& operation_name, vector<StackEntry>& stack, Operation operation)
{
  if (stack.empty())
    throw Exception ("no operand in stack for operation \"" + operation_name + "\"!");
  StackEntry& a (stack[stack.size()-1]);
  a.load();
  if (a.evaluator || a.image || a.rng) {
    StackEntry entry (new UnaryEvaluator<Operation> (operation_name, operation, a));
    stack.back() = entry;
  }
  else {
    try {
      a.value = ( a.value.imag() == 0.0 ? operation.R (a.value.real()) : operation.Z (a.value) );
    }
    catch (...) {
      throw Exception ("operation \"" + operation_name + "\" not supported for data type supplied");
    }
  }
}





template <class Operation>
void binary_operation (const std::string& operation_name, vector<StackEntry>& stack, Operation operation)
{
  if (stack.size() < 2)
    throw Exception ("not enough operands in stack for operation \"" + operation_name + "\"");
  StackEntry& a (stack[stack.size()-2]);
  StackEntry& b (stack[stack.size()-1]);
  a.load();
  b.load();
  if (a.evaluator || a.image || a.rng || b.evaluator || b.image || b.rng) {
    StackEntry entry (new BinaryEvaluator<Operation> (operation_name, operation, a, b));
    stack.pop_back();
    stack.back() = entry;
  }
  else {
    a.value = ( a.value.imag() == 0.0  && b.value.imag() == 0.0 ?
      operation.R (a.value.real(), b.value.real()) :
      operation.Z (a.value, b.value) );
    stack.pop_back();
  }
}




template <class Operation>
void ternary_operation (const std::string& operation_name, vector<StackEntry>& stack, Operation operation)
{
  if (stack.size() < 3)
    throw Exception ("not enough operands in stack for operation \"" + operation_name + "\"");
  StackEntry& a (stack[stack.size()-3]);
  StackEntry& b (stack[stack.size()-2]);
  StackEntry& c (stack[stack.size()-1]);
  a.load();
  b.load();
  c.load();
  if (a.evaluator || a.image || a.rng || b.evaluator || b.image || b.rng || c.evaluator || c.image || c.rng) {
    StackEntry entry (new TernaryEvaluator<Operation> (operation_name, operation, a, b, c));
    stack.pop_back();
    stack.pop_back();
    stack.back() = entry;
  }
  else {
    a.value = ( a.value.imag() == 0.0  && b.value.imag() == 0.0  && c.value.imag() == 0.0 ?
      operation.R (a.value.real(), b.value.real(), c.value.real()) :
      operation.Z (a.value, b.value, c.value) );
    stack.pop_back();
    stack.pop_back();
  }
}





/**********************************************************************
   MULTI-THREADED RUNNING OF OPERATIONS:
 **********************************************************************/


void get_header (const StackEntry& entry, Header& header)
{
  if (entry.evaluator) {
    for (size_t n = 0; n < entry.evaluator->operands.size(); ++n)
      get_header (entry.evaluator->operands[n], header);
    return;
  }

  if (!entry.image)
    return;

  if (header.ndim() == 0) {
    header = *entry.image;
    return;
  }

  if (header.ndim() < entry.image->ndim())
    header.ndim() = entry.image->ndim();
  for (size_t n = 0; n < std::min<size_t> (header.ndim(), entry.image->ndim()); ++n) {
    if (header.size(n) > 1 && entry.image->size(n) > 1 && header.size(n) != entry.image->size(n))
      throw Exception ("dimensions of input images do not match - aborting");
    if (!voxel_grids_match_in_scanner_space (header, *(entry.image), 1.0e-4) && !transform_mis_match_reported) {
      WARN ("header transformations of input images do not match");
      transform_mis_match_reported = true;
    }
    header.size(n) = std::max (header.size(n), entry.image->size(n));
    if (!std::isfinite (header.spacing(n)))
      header.spacing(n) = entry.image->spacing(n);
  }

  header.merge_keyval (*entry.image);
}






class ThreadFunctor { NOMEMALIGN
  public:
    ThreadFunctor (
        const vector<size_t>& inner_axes,
        const StackEntry& top_of_stack,
        Image<complex_type>& output_image) :
      top_entry (top_of_stack),
      image (output_image),
      loop (Loop (inner_axes)) {
        storage.axes = loop.axes;
        storage.size.push_back (image.size(storage.axes[0]));
        storage.size.push_back (image.size(storage.axes[1]));
        chunk_size = image.size (storage.axes[0]) * image.size (storage.axes[1]);
        allocate_storage (top_entry);
      }

    void allocate_storage (const StackEntry& entry) {
      if (entry.evaluator) {
        for (size_t n = 0; n < entry.evaluator->operands.size(); ++n)
          allocate_storage (entry.evaluator->operands[n]);
        return;
      }

      storage.push_back (ThreadLocalStorageItem());
      if (entry.image) {
        storage.back().image.reset (new Image<complex_type> (*entry.image));
        storage.back().chunk.resize (chunk_size);
        return;
      }
      else if (entry.rng) {
        storage.back().chunk.resize (chunk_size);
      }
      else storage.back().chunk.value = entry.value;
    }


    void operator() (const Iterator& iter) {
      storage.reset (iter);
      assign_pos_of (iter).to (image);

      Chunk& chunk = top_entry.evaluate (storage);

      auto value = chunk.cbegin();
      for (auto l = loop (image); l; ++l)
        image.value() = *(value++);
    }



    const StackEntry& top_entry;
    Image<complex_type> image;
    decltype (Loop (vector<size_t>())) loop;
    ThreadLocalStorage storage;
    size_t chunk_size;
};





void run_operations (const vector<StackEntry>& stack)
{
  Header header;
  get_header (stack[0], header);

  if (header.ndim() == 0) {
    DEBUG ("no valid images supplied - assuming calculator mode");
    if (stack.size() != 1)
      throw Exception ("too many operands left on stack!");

    assert (!stack[0].evaluator);
    assert (!stack[0].image);

    print (str (stack[0].value) + "\n");
    return;
  }

  if (stack.size() == 1)
    throw Exception ("output image not specified");

  if (stack.size() > 2)
    throw Exception ("too many operands left on stack!");

  if (!stack[1].arg)
    throw Exception ("output image not specified");

  if (stack[0].is_complex()) {
    header.datatype() = DataType::from_command_line (DataType::CFloat32);
    if (!header.datatype().is_complex())
      throw Exception ("output datatype must be complex");
  }
  else header.datatype() = DataType::from_command_line (DataType::Float32);

  auto output = Header::create (stack[1].arg, header).get_image<complex_type>();

  auto loop = ThreadedLoop ("computing: " + operation_string(stack[0]), output, 0, output.ndim(), 2);

  ThreadFunctor functor (loop.inner_axes, stack[0], output);
  loop.run_outer (functor);
}







/**********************************************************************
        OPERATIONS BASIC FRAMEWORK:
**********************************************************************/

class OpBase { NOMEMALIGN
  public:
    OpBase (const char* format_string, bool complex_maps_to_real = false, bool real_map_to_complex = false) :
      format (format_string),
      ZtoR (complex_maps_to_real),
      RtoZ (real_map_to_complex) { }
    const char* format;
    const bool ZtoR, RtoZ;
};

class OpUnary : public OpBase { NOMEMALIGN
  public:
    OpUnary (const char* format_string, bool complex_maps_to_real = false, bool real_map_to_complex = false) :
      OpBase (format_string, complex_maps_to_real, real_map_to_complex) { }
    complex_type R (real_type v) const { throw Exception ("operation not supported!"); return v; }
    complex_type Z (complex_type v) const { throw Exception ("operation not supported!"); return v; }
};


class OpBinary : public OpBase { NOMEMALIGN
  public:
    OpBinary (const char* format_string, bool complex_maps_to_real = false, bool real_map_to_complex = false) :
      OpBase (format_string, complex_maps_to_real, real_map_to_complex) { }
    complex_type R (real_type a, real_type b) const { throw Exception ("operation not supported!"); return a; }
    complex_type Z (complex_type a, complex_type b) const { throw Exception ("operation not supported!"); return a; }
};

class OpTernary : public OpBase { NOMEMALIGN
  public:
    OpTernary (const char* format_string, bool complex_maps_to_real = false, bool real_map_to_complex = false) :
      OpBase (format_string, complex_maps_to_real, real_map_to_complex) { }
    complex_type R (real_type a, real_type b, real_type c) const { throw Exception ("operation not supported!"); return a; }
    complex_type Z (complex_type a, complex_type b, complex_type c) const { throw Exception ("operation not supported!"); return a; }
};


/**********************************************************************
        EXPAND OPERATIONS:
**********************************************************************/

#define SECTION 2
#include "mrcalc.cpp"



/**********************************************************************
  MAIN BODY OF COMMAND:
 **********************************************************************/

void run () {
  vector<StackEntry> stack;

  for (int n = 1; n < App::argc; ++n) {

    const Option* opt = match_option (App::argv[n]);
    if (opt) {

      if (opt->is ("datatype")) ++n;
      else if (opt->is ("nthreads")) ++n;
      else if (opt->is ("force") || opt->is ("info") || opt->is ("debug") || opt->is ("quiet"))
        continue;

#define SECTION 3
#include "mrcalc.cpp"

      else
        throw Exception (std::string ("operation \"") + opt->id + "\" not yet implemented!");

    }
    else {
      stack.push_back (App::argv[n]);
    }

  }

  stack[0].load();
  run_operations (stack);
}

#endif

