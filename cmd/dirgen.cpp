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

#include <gsl/gsl_blas.h>
#include <gsl/gsl_multimin.h>

#include "command.h"
#include "progressbar.h"
#include "math/vector.h"
#include "math/matrix.h"
#include "math/rng.h"
#include "dwi/directions/file.h"


using namespace MR;
using namespace App;

void usage () {

DESCRIPTION
  + "generate a set of uniformly distributed directions using a bipolar electrostatic repulsion model.";

REFERENCES 
  + "* Jones, D.; Horsfield, M. & Simmons, A. "
  "Optimal strategies for measuring diffusion in anisotropic systems by magnetic resonance imaging. "
  "Magnetic Resonance in Medicine, 42: 515-525 (1999)."
             
  + "* Papadakis, N. G.; Murrills, C. D.; Hall, L. D.; Huang, C. L.-H. & Adrian Carpenter, T. "
  "Minimal gradient encoding for robust estimation of diffusion anisotropy. "
  "Magnetic Resonance Imaging, 18: 671-679 (2000).";

ARGUMENTS
  + Argument ("ndir", "the number of directions to generate.").type_integer (6, 60, std::numeric_limits<int>::max())
  + Argument ("dirs", "the text file to write the directions to, as [ az el ] pairs.").type_file_out();

OPTIONS
  + Option ("power", "specify exponent to use for repulsion power law (default: 2).")
  + Argument ("exp").type_integer (2, 2, std::numeric_limits<int>::max())

  + Option ("niter", "specify the maximum number of iterations to perform (default: 10000).")
  + Argument ("num").type_integer (1, 10000, std::numeric_limits<int>::max())

  + Option ("unipolar", "optimise assuming a unipolar electrostatic repulsion model rather than the bipolar model normally assumed in DWI")

  + Option ("cartesian", "Output the directions in Cartesian coordinates [x y z] instead of [az el].");

}





namespace
{
  double power = -1.0;
  size_t   ndirs = 0;
  bool bipolar = true;
}




class SinCos
{
  protected:
    double cos_az, sin_az, cos_el, sin_el;
    double r2_pos, r2_neg, multiplier;

    double energy ();
    void   dist (const SinCos& B);
    void   init_deriv ();
    double daz (const SinCos& B) const;
    double del (const SinCos& B) const;
    double rdel (const SinCos& B) const;

  public:
    SinCos (const gsl_vector* v, size_t index);
    double f (const SinCos& B);
    void   df (const SinCos& B, gsl_vector* deriv, size_t i, size_t j);
    double fdf (const SinCos& B, gsl_vector* deriv, size_t i, size_t j);
};



double energy_f (const gsl_vector* x, void* params);
void   energy_df (const gsl_vector* x, void* params, gsl_vector* df);
void   energy_fdf (const gsl_vector* x, void* params, double* f, gsl_vector* df);


void range (double& azimuth, double& elevation);


void run () {
  size_t niter = 10000;
  float target_power = 2.0;

  auto opt = get_options ("power");
  if (opt.size())
    target_power = opt[0][0];

  opt = get_options ("niter");
  if (opt.size())
    niter = opt[0][0];

  ndirs = to<int> (argument[0]);

  if (get_options ("unipolar").size())
    bipolar = false;

  Math::RNG    rng;
  std::uniform_real_distribution<double> uniform (0.0, 1.0);
  Math::Vector<double> v (2*ndirs);

  for (size_t n = 0; n < 2*ndirs; n+=2) {
    v[n] =  Math::pi * (2.0 * uniform(rng) - 1.0);
    v[n+1] = std::asin (2.0 * uniform(rng) - 1.0);
  }

  gsl_multimin_function_fdf fdf;

  fdf.f = energy_f;
  fdf.df = energy_df;
  fdf.fdf = energy_fdf;
  fdf.n = 2*ndirs;

  gsl_multimin_fdfminimizer* minimizer =
    gsl_multimin_fdfminimizer_alloc (gsl_multimin_fdfminimizer_conjugate_fr, 2*ndirs);


  {
    ProgressBar progress ("Optimising directions...");
    for (power = -1.0; power >= -target_power/2.0; power *= 2.0) {
      INFO ("setting power = " + str (-power*2.0));
      gsl_multimin_fdfminimizer_set (minimizer, &fdf, v.gsl(), 0.01, 1e-4);

      for (size_t iter = 0; iter < niter; iter++) {

        int status = gsl_multimin_fdfminimizer_iterate (minimizer);

        //for (size_t n = 0; n < 2*ndirs; ++n) 
          //std::cout << gsl_vector_get (minimizer->x, n) << " " << gsl_vector_get (minimizer->gradient, n) << "\n";

        if (iter%10 == 0)
          INFO ("[ " + str (iter) + " ] (pow = " + str (-power*2.0) + ") E = " + str (minimizer->f)
          + ", grad = " + str (gsl_blas_dnrm2 (minimizer->gradient)));

        if (status) {
          INFO (std::string ("iteration stopped: ") + gsl_strerror (status));
          break;
        }

        progress.update ([&]() { return "Optimising directions (power " + str(-2.0*power) + ", current energy: " + str(minimizer->f, 8) + ")..."; });
      }
      gsl_vector_memcpy (v.gsl(), minimizer->x);
    }
  }


  Math::Matrix<double> directions (ndirs, 2);
  for (size_t n = 0; n < ndirs; n++) {
    double az = gsl_vector_get (minimizer->x, 2*n);
    double el = gsl_vector_get (minimizer->x, 2*n+1);
    range (az, el);
    directions (n, 0) = az;
    directions (n, 1) = el;
  }

  gsl_multimin_fdfminimizer_free (minimizer);

  DWI::Directions::save (directions, argument[1], get_options ("cartesian").size());
}











inline double SinCos::energy ()
{
  double E = pow (r2_neg, power);
  if (bipolar) E += pow (r2_pos, power);
  return E;
}

inline void SinCos::dist (const SinCos& B)
{
  double a1 = cos_az*sin_el;
  double b1 = B.cos_az*B.sin_el;
  double a2 = sin_az*sin_el;
  double b2 = B.sin_az*B.sin_el;
  r2_neg = Math::pow2(a1-b1) + Math::pow2(a2-b2) + Math::pow2(cos_el-B.cos_el);
  r2_pos = bipolar ? Math::pow2(a1+b1) + Math::pow2(a2+b2) + Math::pow2(cos_el+B.cos_el) : 0.0;
}

inline void SinCos::init_deriv ()
{
  multiplier = pow (r2_neg, power-1.0);
  if (bipolar) multiplier -= pow (r2_pos, power-1.0);
  multiplier *= 2.0 * power;
}

inline double SinCos::daz (const SinCos& B) const
{
  return multiplier * (cos_az*sin_el*B.sin_az*B.sin_el - sin_az*sin_el*B.cos_az*B.sin_el);
}

inline double SinCos::del (const SinCos& B) const
{
  return multiplier * (cos_az*cos_el*B.cos_az*B.sin_el + sin_az*cos_el*B.sin_az*B.sin_el - sin_el*B.cos_el);
}

inline double SinCos::rdel (const SinCos& B) const
{
  return multiplier * (B.cos_az*B.cos_el*cos_az*sin_el + B.sin_az*B.cos_el*sin_az*sin_el - B.sin_el*cos_el);
}

inline SinCos::SinCos (const gsl_vector* v, size_t index)
{
  //double az = index > 1 ? gsl_vector_get (v, 2*index-3) : 0.0;
  //double el = index ? gsl_vector_get (v, 2*index-2) : 0.0;
  double az = gsl_vector_get (v, 2*index);
  double el = gsl_vector_get (v, 2*index+1);
  cos_az = std::cos (az);
  sin_az = std::sin (az);
  cos_el = std::cos (el);
  sin_el = std::sin (el);
}

inline double SinCos::f (const SinCos& B)
{
  dist (B);
  return energy();
}

inline void SinCos::df (const SinCos& B, gsl_vector* deriv, size_t i, size_t j)
{
  dist (B);
  init_deriv ();
  double d = daz (B);
  *gsl_vector_ptr (deriv, 2*i) -= d;
  *gsl_vector_ptr (deriv, 2*i+1) -= del (B);
  *gsl_vector_ptr (deriv, 2*j) += d;
  *gsl_vector_ptr (deriv, 2*j+1) -= rdel (B);
}


inline double SinCos::fdf (const SinCos& B, gsl_vector* deriv, size_t i, size_t j)
{
  df (B, deriv, i, j);
  return energy();
}


double energy_f (const gsl_vector* x, void* params)
{
  double  E = 0.0;
  for (size_t i = 0; i < ndirs; i++) {
    SinCos I (x, i);
    for (size_t j = i+1; j < ndirs; j++)
      E += 2.0 * I.f (SinCos (x, j));
  }
  return E;
}



void energy_df (const gsl_vector* x, void* params, gsl_vector* df)
{
  gsl_vector_set_zero (df);
  for (size_t i = 0; i < ndirs; i++) {
    SinCos I (x, i);
    for (size_t j = i+1; j < ndirs; j++)
      I.df (SinCos (x, j), df, i, j);
  }
}



void energy_fdf (const gsl_vector* x, void* params, double* f, gsl_vector* df)
{
  *f = 0.0;
  gsl_vector_set_zero (df);
  for (size_t i = 0; i < ndirs; i++) {
    SinCos I (x, i);
    for (size_t j = i+1; j < ndirs; j++)
      *f += 2.0 * I.fdf (SinCos (x, j), df, i, j);
  }
}





inline void range (double& azimuth, double& elevation)
{
  while (elevation < 0.0) elevation += 2.0*Math::pi;
  while (elevation >= 2.0*Math::pi) elevation -= 2.0*Math::pi;
  if (elevation >= Math::pi) {
    elevation = 2.0*Math::pi - elevation;
    azimuth -= Math::pi;
  }
  while (azimuth < -Math::pi) azimuth += 2.0*Math::pi;
  while (azimuth >= Math::pi) azimuth -= 2.0*Math::pi;
}


