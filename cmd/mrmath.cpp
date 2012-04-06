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
#include "image/buffer.h"
#include "image/buffer_scratch.h"
#include "math/rng.h"
#include "image/loop.h"
#include "image/threaded_loop.h"
#include "image/threaded_copy.h"
#include "image/stride.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;


void usage () {
DESCRIPTION
  + "apply generic voxel-wise mathematical operations to images."
  
  + "This command uses a stack-based syntax, with operators "
  "(specified using options) operating on the top-most entries "
  "(i.e. images or values) in the stack. Operands (values or "
  "images) are pushed onto the stack in the order they appear "
  "(as arguments) on the command-line, and operands (specified "
  "as options) operate on and comsume the top-most entries in "
  "the stack, and push their output as a new entry on the stack. "
  "For example:"
  
  + "$ mrmath a.mif 2 -mult r.mif"
  
  + "performs the operation r = a x 2 for every voxel a,r in "
  "images a.mif and r.mif respectively. Similarly:"
  
  + "$ mrmath a.mif -neg b.mif -div -exp 9.3 -mult r.mif"
  
  + "performs the operation r = 9.3*exp(-a/b), and:"
  
  + "$ mrmath a.mif b.mif -add c.mif d.mif -mult 4.2 -add -div r.mif"
  
  + "performs r = (a+b)/(c*d+4.2).";

ARGUMENTS
  + Argument ("operand", "an input image or intensity.").allow_multiple();

OPTIONS
  + OptionGroup ("Unary operators")

  + Option ("abs", "absolute value").allow_multiple()
  + Option ("neg", "negative value").allow_multiple()
  + Option ("sqrt", "square root").allow_multiple()
  + Option ("exp", "exponential function").allow_multiple()
  + Option ("log", "natural logarithm").allow_multiple()
  + Option ("log10", "common logarithm").allow_multiple()
  + Option ("cos", "cosine").allow_multiple()
  + Option ("sin", "sine").allow_multiple()
  + Option ("tan", "tangent").allow_multiple()
  + Option ("cosh", "hyperbolic cosine").allow_multiple()
  + Option ("sinh", "hyperbolic sine").allow_multiple()
  + Option ("tanh", "hyperbolic tangent").allow_multiple()
  + Option ("acos", "inverse cosine").allow_multiple()
  + Option ("asin", "inverse sine").allow_multiple()
  + Option ("atan", "inverse tangent").allow_multiple()
  + Option ("acosh", "inverse hyperbolic cosine").allow_multiple()
  + Option ("asinh", "inverse hyperbolic sine").allow_multiple()
  + Option ("atanh", "inverse hyperbolic tangent").allow_multiple()
  + Option ("round", "round to nearest integer").allow_multiple()
  + Option ("ceil", "round up to nearest integer").allow_multiple()
  + Option ("floor", "round down to nearest integer").allow_multiple()

  + Option ("real", "real part of complex number").allow_multiple()
  + Option ("imag", "imaginary part of complex number").allow_multiple()
  + Option ("phase", "phase of complex number").allow_multiple()
  + Option ("conj", "complex conjugate").allow_multiple()

  + OptionGroup ("Binary operators")

  + Option ("add", "add values").allow_multiple()
  + Option ("subtract", "subtract nth operand from (n-1)th").allow_multiple()
  + Option ("multiply", "multiply values").allow_multiple()
  + Option ("divide", "divide (n-1)th operand by nth").allow_multiple()
  + Option ("pow", "raise (n-1)th operand to nth power").allow_multiple()
  + Option ("min", "smallest of last two operands").allow_multiple()
  + Option ("max", "greatest of last two operands").allow_multiple()
  + Option ("lt", "true (1) if (n-1)th operand is smaller than nth, false (0) otherwise").allow_multiple()
  + Option ("gt", "true (1) if (n-1)th operand is greater than nth, false (0) otherwise").allow_multiple()
  + Option ("le", "true (1) if (n-1)th operand is smaller than or equal to nth, false (0) otherwise").allow_multiple()
  + Option ("ge", "true (1) if (n-1)th operand is greater than or equal to nth, false (0) otherwise").allow_multiple()
  + Option ("eq", "true (1) if last two operands are equal, false (0) otherwise").allow_multiple()
  + Option ("neq", "true (1) if last two operands are not equal, false (0) otherwise").allow_multiple()

  + Option ("complex", "create complex number using the last two operands as real,imaginary components").allow_multiple()

  + DataType::options();
}


typedef float real_type;
typedef cfloat complex_type;

typedef Image::Buffer<real_type>::voxel_type real_vox_type;
typedef Image::Buffer<complex_type>::voxel_type complex_vox_type;




/**********************************************************************
  STACK FRAMEWORK:
 **********************************************************************/


class Evaluator;


class Chunk : public std::vector<complex_type> {
  public:
    complex_type value;
};


class ThreadLocalStorageItem {
  public:
    Chunk chunk;
    Ptr<complex_vox_type> vox;
};

class ThreadLocalStorage : public std::vector<ThreadLocalStorageItem> {
  public:

      void load (Chunk& chunk, Image::Buffer<complex_type>::voxel_type& vox) {
        for (size_t n = 0; n < vox.ndim(); ++n)
          if (vox.dim(n) > 1)
            vox[n] = (*iter)[n];

        size_t n = 0;
        for (size_t y = 0; y < dim[1]; ++y) {
          if (axes[1] < vox.ndim()) if (vox.dim(axes[1]) > 1) vox[axes[1]] = y;
          for (size_t x = 0; x < dim[0]; ++x) {
            if (axes[0] < vox.ndim()) if (vox.dim(axes[0]) > 1) vox[axes[0]] = x;
            chunk[n++] = vox.value();
          }
        }
      }

    Chunk& next () {
      ThreadLocalStorageItem& item ((*this)[current++]);
      if (item.vox) load (item.chunk, *item.vox);
      return item.chunk;
    }

    void reset (const Image::Iterator& current_position) { current = 0; iter = &current_position; }

    const Image::Iterator* iter;
    std::vector<size_t> axes;
    std::vector<size_t> dim;

  private:
    size_t current;
};






class StackEntry {
  public:

    StackEntry (const char* entry) : 
      arg (entry) { }

    StackEntry (Evaluator* evaluator_p) : 
      arg (NULL),
      evaluator (evaluator_p) { }

    void load () {
      if (!arg) 
        return;
      try {
        buffer = new Image::Buffer<complex_type> (arg);
      }
      catch (Exception) {
        value = to<complex_type> (arg);
      }
      arg = NULL;
    }

    const char* arg;
    RefPtr<Evaluator> evaluator;
    RefPtr<Image::Buffer<complex_type> > buffer;
    complex_type value;

    bool is_complex () const;

    Chunk& evaluate (ThreadLocalStorage& storage) const;
};


class Evaluator
{
  public: 
    Evaluator (const std::string& name, bool complex_maps_to_real = false, bool real_maps_to_complex = false) : 
      id (name),
      ZtoR (complex_maps_to_real),
      RtoZ (real_maps_to_complex) { }
    const std::string id;
    bool ZtoR, RtoZ;
    std::vector<StackEntry> operands;

    Chunk& evaluate (ThreadLocalStorage& storage) const {
      Chunk& in1 (operands[0].evaluate (storage));
      if (is_unary()) return evaluate (in1);
      Chunk& in2 (operands[1].evaluate (storage));
      return evaluate (in1, in2);
    }
    virtual Chunk& evaluate (Chunk& in) const { throw Exception ("operation \"" + id + "\" not supported!"); return in; }
    virtual Chunk& evaluate (Chunk& a, Chunk& b) const { throw Exception ("operation \"" + id + "\" not supported!"); return a; }

    virtual bool is_complex () const {
      for (size_t n = 0; n < operands.size(); ++n) 
        if (operands[n].is_complex())  
          return !ZtoR;
      return RtoZ;
    }
    bool is_unary () const { return operands.size() == 1; }

};



inline bool StackEntry::is_complex () const {
  if (buffer) return buffer->datatype().is_complex();
  if (evaluator) return evaluator->is_complex();
  return value.imag() != 0.0;
}



inline Chunk& StackEntry::evaluate (ThreadLocalStorage& storage) const
{
  return evaluator ? evaluator->evaluate (storage) : storage.next();
}




/*
void print_stack (const std::vector<StackEntry>& stack, const std::string& prefix = "  ") 
{
  for (size_t n = 0; n < stack.size(); ++n) {
    std::cout << prefix << n << ": ";
    if (stack[n].buffer) std::cout << "[IMA] " << stack[n].buffer->name();
    else if (stack[n].evaluator) std::cout << "[EVAL] " << stack[n].evaluator->id;
    else if (stack[n].arg) std::cout << "[ARG] " << stack[n].arg;
    else std::cout << "[VAL] " << str(stack[n].value);
    std::cout << " [" << ( stack[n].is_complex() ? "COMPLEX" : "REAL" ) << "]\n";
    if (stack[n].evaluator) 
      print_stack (stack[n].evaluator->operands, prefix + "  ");
  }
}
*/


template <class Operation>
class UnaryEvaluator : public Evaluator 
{
  public:
    UnaryEvaluator (const std::string& name, Operation operation, const StackEntry& operand) : 
      Evaluator (name, operation.ZtoR, operation.RtoZ), 
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
class BinaryEvaluator : public Evaluator 
{
  public:
    BinaryEvaluator (const std::string& name, Operation operation, const StackEntry& operand1, const StackEntry& operand2) : 
      Evaluator (name, operation.ZtoR, operation.RtoZ),
      op (operation) { 
        operands.push_back (operand1);
        operands.push_back (operand2);
      }

    Operation op;

    virtual Chunk& evaluate (Chunk& a, Chunk& b) const {
      bool do_complex_op = operands[0].is_complex() || operands[1].is_complex();
      if (a.size() && b.size()) {
        if (do_complex_op)
          for (size_t n = 0; n < a.size(); ++n)
            a[n] = op.Z (a[n], b[n]);
        else 
          for (size_t n = 0; n < a.size(); ++n)
            a[n] = op.R (a[n].real(), b[n].real());
        return a;
      }
      else if (a.size()) {
        if (do_complex_op)
          for (size_t n = 0; n < a.size(); ++n)
            a[n] = op.Z (a[n], b.value);
        else 
          for (size_t n = 0; n < a.size(); ++n)
            a[n] = op.R (a[n].real(), b.value.real());
        return a;
      }
      else {
        if (do_complex_op)
          for (size_t n = 0; n < a.size(); ++n)
            b[n] = op.Z (a.value, b[n]);
        else 
          for (size_t n = 0; n < a.size(); ++n)
            b[n] = op.R (a.value.real(), b[n].real());
        return b;
      }
    }

};





template <class Operation>
void unary_operation (const std::string& operation_name, std::vector<StackEntry>& stack, Operation operation)
{
  if (stack.empty()) 
    throw Exception ("no operand in stack for operation \"" + operation_name + "\"!");
  StackEntry& a (stack[stack.size()-1]);
  a.load();
  if (a.evaluator || a.buffer) {
    StackEntry entry (new UnaryEvaluator<Operation> (operation_name, operation, stack.back()));
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
};





template <class Operation>
void binary_operation (const std::string& operation_name, std::vector<StackEntry>& stack, Operation operation)
{
  if (stack.size() < 2) 
    throw Exception ("not enough operands in stack for operation \"" + operation_name + "\"");
  StackEntry& a (stack[stack.size()-2]);
  StackEntry& b (stack[stack.size()-1]);
  a.load();
  b.load();
  if (a.evaluator || a.buffer || b.evaluator || b.buffer) {
    StackEntry entry (new BinaryEvaluator<Operation> (operation_name, operation, stack[stack.size()-2], stack[stack.size()-1]));
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





/**********************************************************************
   MULTI-THREADED RUNNING OF OPERATIONS:
 **********************************************************************/


void get_header (const StackEntry& entry, Image::Header& header) 
{
  if (entry.evaluator) {
    for (size_t n = 0; n < entry.evaluator->operands.size(); ++n)
      get_header (entry.evaluator->operands[n], header);
    return;
  }

  if (!entry.buffer) 
    return;

  if (header.ndim() == 0) {
    header = *entry.buffer;
    return;
  }

  if (header.ndim() < entry.buffer->ndim()) 
    header.set_ndim (entry.buffer->ndim());
  for (size_t n = 0; n < std::min (header.ndim(), entry.buffer->ndim()); ++n) {
    if (header.dim(n) > 1 && entry.buffer->dim(n) > 1 && header.dim(n) != entry.buffer->dim(n))
      throw Exception ("dimensions of input images do not match - aborting");
    header.dim(n) = std::max (header.dim(n), entry.buffer->dim(n));
    if (!finite (header.vox(n))) 
      header.vox(n) = entry.buffer->vox(n);
  }

}






class ThreadFunctor {
  public:
    ThreadFunctor (
        const Image::ThreadedLoop& threaded_loop,
        const StackEntry& top_of_stack, 
        Image::Buffer<complex_type>& output_image) :
      top_entry (top_of_stack),
      vox (output_image),
      loop (threaded_loop.inner_axes()) {
        storage.axes = loop.axes();
        storage.dim.push_back (vox.dim(storage.axes[0]));
        storage.dim.push_back (vox.dim(storage.axes[1]));
        allocate_storage (top_entry);
      }

    void allocate_storage (const StackEntry& entry) {
      if (entry.evaluator) {
        for (size_t n = 0; n < entry.evaluator->operands.size(); ++n)
          allocate_storage (entry.evaluator->operands[n]);
        return;
      }

      storage.push_back (ThreadLocalStorageItem());
      if (entry.buffer) {
        storage.back().vox = new Image::Buffer<complex_type>::voxel_type (*entry.buffer);
        storage.back().chunk.resize (vox.dim(storage.axes[0])*vox.dim(storage.axes[1]));
        return;
      }
      else storage.back().chunk.value = entry.value;
    }


    void operator() (const Image::Iterator& iter) {
      storage.reset (iter);
      Image::voxel_assign (vox, iter);

      Chunk& chunk = top_entry.evaluate (storage);

      std::vector<complex_type>::const_iterator value (chunk.begin());
      for (loop.start (vox); loop.ok(); loop.next (vox)) 
        vox.value() = *(value++);
    }



    const StackEntry& top_entry;
    Image::Buffer<complex_type>::voxel_type vox;
    Image::LoopInOrder loop;
    ThreadLocalStorage storage;
};





void run_operations (const std::vector<StackEntry>& stack) 
{
  if (!stack[1].arg)
    throw Exception (std::string ("error opening output image \"") + stack[1].arg + "\"!");

  Image::Header header;
  get_header (stack[0], header);

  if (stack[0].is_complex()) {
    header.datatype() = DataType::from_command_line (DataType::CFloat32);
    if (!header.datatype().is_complex())
      throw Exception ("output datatype must be complex");
  }
  else header.datatype() = DataType::from_command_line (DataType::Float32);

  Image::Buffer<complex_type> output (stack[1].arg, header);

  Image::ThreadedLoop loop ("evaluating mathematical operations...", output, 2);

  ThreadFunctor functor (loop, stack[0], output);
  loop.run_outer (functor);
}







/**********************************************************************
        OPERATIONS BASIC FRAMEWORK:
**********************************************************************/

class OpBase {
  public:
    OpBase (bool complex_maps_to_real = false, bool real_map_to_complex = false) :
      ZtoR (complex_maps_to_real),
      RtoZ (real_map_to_complex) { }
    const bool ZtoR, RtoZ;
};

class OpUnary : public OpBase {
  public:
    OpUnary (bool complex_maps_to_real = false, bool real_map_to_complex = false) :
      OpBase (complex_maps_to_real, real_map_to_complex) { }
    complex_type R (real_type v) const { throw Exception ("operation not supported!"); return v; }
    complex_type Z (complex_type v) const { throw Exception ("operation not supported!"); return v; }
};


class OpBinary : public OpBase {
  public:
    OpBinary (bool complex_maps_to_real = false, bool real_map_to_complex = false) :
      OpBase (complex_maps_to_real, real_map_to_complex) { }
    complex_type R (real_type a, real_type b) const { throw Exception ("operation not supported!"); return a; }
    complex_type Z (complex_type a, complex_type b) const { throw Exception ("operation not supported!"); return a; }
};



/**********************************************************************
        UNARY OPERATIONS:
**********************************************************************/

class OpAbs : public OpUnary {
  public:
    OpAbs () : OpUnary (true) { }
    complex_type R (real_type v) const { return Math::abs (v); }
    complex_type Z (complex_type v) const { return std::abs (v); }
};

class OpNeg : public OpUnary {
  public:
    complex_type R (real_type v) const { return -v; }
    complex_type Z (complex_type v) const { return -v; }
};

class OpSqrt : public OpUnary {
  public:
    complex_type R (real_type v) const { return Math::sqrt (v); }
    complex_type Z (complex_type v) const { return std::sqrt (v); }
};

class OpExp : public OpUnary {
  public:
    complex_type R (real_type v) const { return Math::exp (v); }
    complex_type Z (complex_type v) const { return std::exp (v); }
};

class OpLog : public OpUnary {
  public:
    complex_type R (real_type v) const { return Math::log (v); }
    complex_type Z (complex_type v) const { return std::log (v); }
};

class OpLog10 : public OpUnary {
  public:
    complex_type R (real_type v) const { return Math::log10 (v); }
    complex_type Z (complex_type v) const { return std::log10 (v); }
};

class OpCos : public OpUnary {
  public:
    complex_type R (real_type v) const { return Math::cos (v); }
    complex_type Z (complex_type v) const { return std::cos (v); }
};

class OpSin : public OpUnary {
  public:
    complex_type R (real_type v) const { return Math::sin (v); }
    complex_type Z (complex_type v) const { return std::sin (v); }
};

class OpTan : public OpUnary {
  public:
    complex_type R (real_type v) const { return Math::tan (v); }
    complex_type Z (complex_type v) const { return std::tan (v); }
};

class OpCosh : public OpUnary {
  public:
    complex_type R (real_type v) const { return Math::cosh (v); }
    complex_type Z (complex_type v) const { return std::cosh (v); }
};

class OpSinh : public OpUnary {
  public:
    complex_type R (real_type v) const { return Math::sinh (v); }
    complex_type Z (complex_type v) const { return std::sinh (v); }
};

class OpTanh : public OpUnary {
  public:
    complex_type R (real_type v) const { return Math::tanh (v); }
    complex_type Z (complex_type v) const { return std::tanh (v); }
};

class OpAcos : public OpUnary {
  public:
    complex_type R (real_type v) const { return Math::acos (v); }
};

class OpAsin : public OpUnary {
  public:
    complex_type R (real_type v) const { return Math::asin (v); }
};

class OpAtan : public OpUnary {
  public:
    complex_type R (real_type v) const { return Math::atan (v); }
};

class OpAcosh : public OpUnary {
  public:
    complex_type R (real_type v) const { return Math::acosh (v); }
};

class OpAsinh : public OpUnary {
  public:
    complex_type R (real_type v) const { return Math::asinh (v); }
};

class OpAtanh : public OpUnary {
  public:
    complex_type R (real_type v) const { return Math::atanh (v); }
};


class OpRound : public OpUnary {
  public:
    complex_type R (real_type v) const { return Math::round (v); }
};

class OpCeil : public OpUnary {
  public:
    complex_type R (real_type v) const { return Math::ceil (v); }
};

class OpFloor : public OpUnary {
  public:
    complex_type R (real_type v) const { return Math::floor (v); }
};

class OpReal : public OpUnary {
  public:
    OpReal () : OpUnary (true) { }
    complex_type Z (complex_type v) const { return v.real(); }
};

class OpImag : public OpUnary {
  public:
    OpImag () : OpUnary (true) { }
    complex_type Z (complex_type v) const { return v.imag(); }
};

class OpPhase : public OpUnary {
  public:
    OpPhase () : OpUnary (true) { }
    complex_type Z (complex_type v) const { return std::arg (v); }
};

class OpConj : public OpUnary {
  public:
    complex_type Z (complex_type v) const { return std::conj (v); }
};


/**********************************************************************
        BINARY OPERATIONS:
**********************************************************************/

class OpAdd : public OpBinary {
  public:
    complex_type R (real_type a, real_type b) const { return a+b; }
    complex_type Z (complex_type a, complex_type b) const { return a+b; }
};

class OpSubtract : public OpBinary {
  public:
    complex_type R (real_type a, real_type b) const { return a-b; }
    complex_type Z (complex_type a, complex_type b) const { return a-b; }
};

class OpMultiply : public OpBinary {
  public:
    complex_type R (real_type a, real_type b) const { return a*b; }
    complex_type Z (complex_type a, complex_type b) const { return a*b; }
};

class OpDivide : public OpBinary {
  public:
    complex_type R (real_type a, real_type b) const { return a/b; }
    complex_type Z (complex_type a, complex_type b) const { return a/b; }
};

class OpPow : public OpBinary {
  public:
    complex_type R (real_type a, real_type b) const { return Math::pow (a, b); }
    complex_type Z (complex_type a, complex_type b) const { return std::pow (a, b); }
};

class OpMin : public OpBinary {
  public:
    complex_type R (real_type a, real_type b) const { return std::min (a, b); }
};

class OpMax : public OpBinary {
  public:
    complex_type R (real_type a, real_type b) const { return std::max (a, b); }
};

class OpLessThan : public OpBinary {
  public:
    complex_type R (real_type a, real_type b) const { return a < b; }
};

class OpGreaterThan : public OpBinary {
  public:
    complex_type R (real_type a, real_type b) const { return a > b; }
};

class OpLessThanOrEqual : public OpBinary {
  public:
    complex_type R (real_type a, real_type b) const { return a <= b; }
};

class OpGreaterThanOrEqual : public OpBinary {
  public:
    complex_type R (real_type a, real_type b) const { return a >= b; }
};

class OpEqual : public OpBinary {
  public:
    OpEqual () : OpBinary (true) { }
    complex_type R (real_type a, real_type b) const { return a == b; }
    complex_type Z (complex_type a, complex_type b) const { return a == b; }
};

class OpNotEqual : public OpBinary {
  public:
    OpNotEqual () : OpBinary (true) { }
    complex_type R (real_type a, real_type b) const { return a != b; }
    complex_type Z (complex_type a, complex_type b) const { return a != b; }
};

class OpComplex : public OpBinary {
  public:
    OpComplex () : OpBinary (false, true) { }
    complex_type R (real_type a, real_type b) const { return complex_type (a, b); }
};









/**********************************************************************
  MAIN BODY OF COMMAND:
 **********************************************************************/

void run () {
  std::vector<StackEntry> stack;

  for (int n = 1; n < App::argc; ++n) {

    const Option* opt = match_option (App::argv[n]);
    if (opt) {
      if (opt->is ("abs")) unary_operation (opt->id, stack, OpAbs()); 
      else if (opt->is ("neg")) unary_operation (opt->id, stack, OpNeg());
      else if (opt->is ("sqrt")) unary_operation (opt->id, stack, OpSqrt());
      else if (opt->is ("exp")) unary_operation (opt->id, stack, OpExp());
      else if (opt->is ("log")) unary_operation (opt->id, stack, OpLog());
      else if (opt->is ("log10")) unary_operation (opt->id, stack, OpLog10());

      else if (opt->is ("cos")) unary_operation (opt->id, stack, OpCos());
      else if (opt->is ("sin")) unary_operation (opt->id, stack, OpSin());
      else if (opt->is ("tan")) unary_operation (opt->id, stack, OpTan());

      else if (opt->is ("cosh")) unary_operation (opt->id, stack, OpCosh());
      else if (opt->is ("sinh")) unary_operation (opt->id, stack, OpSinh());
      else if (opt->is ("tanh")) unary_operation (opt->id, stack, OpTanh());

      else if (opt->is ("acos")) unary_operation (opt->id, stack, OpAcos());
      else if (opt->is ("asin")) unary_operation (opt->id, stack, OpAsin());
      else if (opt->is ("atan")) unary_operation (opt->id, stack, OpAtan());

      else if (opt->is ("acosh")) unary_operation (opt->id, stack, OpAcosh());
      else if (opt->is ("asinh")) unary_operation (opt->id, stack, OpAsinh());
      else if (opt->is ("atanh")) unary_operation (opt->id, stack, OpAtanh());

      else if (opt->is ("round")) unary_operation (opt->id, stack, OpRound());
      else if (opt->is ("ceil")) unary_operation (opt->id, stack, OpCeil());
      else if (opt->is ("floor")) unary_operation (opt->id, stack, OpFloor());

      else if (opt->is ("real")) unary_operation (opt->id, stack, OpReal());
      else if (opt->is ("imag")) unary_operation (opt->id, stack, OpImag());
      else if (opt->is ("phase")) unary_operation (opt->id, stack, OpPhase());
      else if (opt->is ("conj")) unary_operation (opt->id, stack, OpConj());

      else if (opt->is ("add")) binary_operation (opt->id, stack, OpAdd());
      else if (opt->is ("subtract")) binary_operation (opt->id, stack, OpSubtract());
      else if (opt->is ("multiply")) binary_operation (opt->id, stack, OpMultiply());
      else if (opt->is ("divide")) binary_operation (opt->id, stack, OpDivide());
      else if (opt->is ("pow")) binary_operation (opt->id, stack, OpPow());

      else if (opt->is ("min")) binary_operation (opt->id, stack, OpMin());
      else if (opt->is ("max")) binary_operation (opt->id, stack, OpMax());
      else if (opt->is ("lt")) binary_operation (opt->id, stack, OpLessThan());
      else if (opt->is ("gt")) binary_operation (opt->id, stack, OpGreaterThan());
      else if (opt->is ("le")) binary_operation (opt->id, stack, OpLessThanOrEqual());
      else if (opt->is ("ge")) binary_operation (opt->id, stack, OpGreaterThanOrEqual());
      else if (opt->is ("eq")) binary_operation (opt->id, stack, OpEqual());
      else if (opt->is ("neq")) binary_operation (opt->id, stack, OpNotEqual());

      else if (opt->is ("complex")) binary_operation (opt->id, stack, OpComplex());

      else if (opt->is ("datatype")) ++n;
      else if (opt->is ("nthreads")) ++n;
      else if (opt->is ("force") || opt->is ("info") || opt->is ("debug") || opt->is ("quiet"))
        continue;

      else error (std::string ("operation \"") + opt->id + "\" not yet implemented!");

    }
    else {
      stack.push_back (App::argv[n]);
    }

  }

  if (stack.size() == 1) {
    if (!stack[0].evaluator && !stack[0].buffer) 
      print (str(stack[0].value));
    return;
  }

  if (stack.size() == 2) {
    run_operations (stack);
    return;
  }

  throw Exception ("stack is not empty!");
}



