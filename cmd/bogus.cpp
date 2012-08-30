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
#include "debug.h"
#include "math/matrix.h"
#include "math/gradient_descent.h"
#include "math/check_gradient.h"

MRTRIX_APPLICATION

using namespace MR;
using namespace App;



void usage () {

  DESCRIPTION 
    + "this is used to test stuff. I need to write a lot of stuff here to pad this out and check that the wrapping functionality works as advertised... Seems to do an OK job so far. Wadaya reckon?"
    + "some more details here.";
}


typedef double value_type;

class Cost {
  public:
    Cost () : M(3,2), b(3), y(3) {
      M.zero();
      M(0,0) = 1.0;
      M(1,1) = 2.0;
      M(2,1) = 5.0;
      VAR (M);

      b[0] = 2.0;
      b[1] = -1.0;
      b[2] = 5.0;

      VAR (b);
    }


    typedef ::value_type value_type;

    size_t size () const { return 2; }

    value_type init (Math::Vector<value_type>& x) const {
      x = 0.0;
      return 5.0;
    }

    value_type operator() (const Math::Vector<value_type>& x, Math::Vector<value_type>& g) const {

      Math::mult (y, M, x);
      y -= b;
      value_type cost = Math::norm2 (y);

      Math::mult (g, value_type(0.0), value_type(2.0), CblasTrans, M, y);

      return cost;
    }

  protected:
    Math::Matrix<value_type> M;
    Math::Vector<value_type> b;
    mutable Math::Vector<value_type> y;
};



void run () 
{
  Cost cost;
  Math::Vector<value_type> x (cost.size());
  cost.init (x);
  Math::check_function_gradient (cost, x, 1.0e-4, true);

  Math::GradientDescent<Cost> optim (cost);

  Math::Vector<value_type> preconditioner (2);
  preconditioner[0] = 1.0;
  preconditioner[1] = 1.0/29.0;

  Math::check_function_gradient (cost, x, 1.0e-4, true, preconditioner);

  optim.precondition (preconditioner);
  optim.run ();
  VAR (optim.state());
  VAR (optim.function_evaluations());

  x = optim.state();
  Math::check_function_gradient (cost, x, 1.0e-4, true);
  Math::check_function_gradient (cost, x, 1.0e-4, true, preconditioner);
}

