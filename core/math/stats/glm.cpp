/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include "math/stats/glm.h"

#define GLM_BATCH_SIZE 1024

namespace MR
{
  namespace Math
  {
    namespace Stats
    {



      namespace GLM
      {




        matrix_type solve_betas (const matrix_type& measurements, const matrix_type& design)
        {
          return design.jacobiSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(measurements.transpose());
        }



        matrix_type abs_effect_size (const matrix_type& measurements, const matrix_type& design, const matrix_type& contrasts)
        {
          return contrasts * solve_betas (measurements, design);
        }


        matrix_type stdev (const matrix_type& measurements, const matrix_type& design)
        {
          matrix_type residuals = measurements.transpose() - design * solve_betas (measurements, design); //TODO
          residuals = residuals.array().pow(2.0);
          matrix_type one_over_dof (1, measurements.cols());  //TODO supply transposed measurements
          one_over_dof.fill (1.0 / value_type(design.rows()-Math::rank (design)));
          return (one_over_dof * residuals).array().sqrt();
        }


        matrix_type std_effect_size (const matrix_type& measurements, const matrix_type& design, const matrix_type& contrasts)
        {
          return abs_effect_size (measurements, design, contrasts).array() / stdev (measurements, design).array();
        }
      }








      GLMTTestFixed::GLMTTestFixed (const matrix_type& measurements, const matrix_type& design, const matrix_type& contrast) :
          GLMTestBase (measurements, design, contrast),
          pinvX (Math::pinv (X)),
          scaled_contrasts (calc_scaled_contrasts()) { }



      void GLMTTestFixed::operator() (const vector<size_t>& perm_labelling, matrix_type& output) const
      {
        output = matrix_type::Zero (num_elements(), num_outputs());
        matrix_type tvalues, betas, residuals, SX, pinvSX;

        // TODO Currently the entire design matrix is permuted;
        //   we may instead prefer Freedman-Lane
        // This however would be different for each row in the contrasts matrix,
        //   since the columns that correspond to nuisance variables
        //   varies between rows

        SX.resize (X.rows(), X.cols());
        pinvSX.resize (pinvX.rows(), pinvX.cols());
        for (ssize_t i = 0; i < X.rows(); ++i) {
          SX.row(i) = X.row (perm_labelling[i]);
          pinvSX.col(i) = pinvX.col (perm_labelling[i]);
        }

        SX.transposeInPlace();
        pinvSX.transposeInPlace();
        for (ssize_t i = 0; i < y.rows(); i += GLM_BATCH_SIZE) {
          const auto tmp = y.block (i, 0, std::min (GLM_BATCH_SIZE, (int)(y.rows()-i)), y.cols());
          ttest (tvalues, SX, pinvSX, tmp, betas, residuals);
          for (size_t col = 0; col != num_outputs(); ++col) {
            for (size_t n = 0; n != size_t(tvalues.rows()); ++n) {
              value_type val = tvalues(n, col);
              if (!std::isfinite (val))
                val = value_type(0);
              output(i+n, col) = val;
            }
          }
        }
      }



      // scale contrasts for use in ttest() member function
      /* This pre-scales the contrast matrix in order to make conversion from GLM betas
       * to t-values more computationally efficient.
       *
       * For design matrix X, contrast matrix c, beta vector b and variance o^2, the t-value is calculated as:
       *               c^T.b
       * t = --------------------------
       *     sqrt(o^2.c^T.(X^T.X)^-1.c)
       *
       * Definition of variance (for vector of residuals e):
       *       e^T.e
       * o^2 = ------
       *       DOF(X)
       *
       * (Note that the above equations are used directly in GLMTTestVariable)
       *
       * This function will generate scaled contrasts c' from c, such that:
       *                   DOF(X)
       * c' = c.sqrt(------------------)
       *              c^T.(X^T.X)^-1.c
       *
       *       c'^T.b
       * t = -----------
       *     sqrt(e^T.e)
       *
       * Note each row of the contrast matrix will still be treated as an independent contrast. The number
       * of elements in each contrast vector must equal the number of columns in the design matrix.
       */
    matrix_type GLMTTestFixed::calc_scaled_contrasts() const
    {
      const size_t dof = X.rows() - rank(X);
      const matrix_type XtX = X.transpose() * X;
      const matrix_type pinv_XtX = (XtX.transpose() * XtX).fullPivLu().solve (XtX.transpose());
      matrix_type result = c;
      for (size_t n = 0; n < size_t(c.rows()); ++n) {
        auto pinv_XtX_c = pinv_XtX * c.row(n).transpose();
        result.row(n) *= std::sqrt (value_type(dof) / c.row(n).dot (pinv_XtX_c));
      }
      return result.transpose();
    }



      void GLMTTestFixed::ttest (matrix_type& tvalues,
                                 const matrix_type& design,
                                 const matrix_type& pinv_design,
                                 Eigen::Block<const matrix_type> measurements,
                                 matrix_type& betas,
                                 matrix_type& residuals) const
      {
        betas.noalias() = measurements * pinv_design;
        residuals.noalias() = measurements - betas * design;
        tvalues.noalias() = betas * scaled_contrasts;
        for (size_t n = 0; n < size_t(tvalues.rows()); ++n)
          tvalues.row(n).array() /= residuals.row(n).norm();
      }










      GLMTTestVariable::GLMTTestVariable (const vector<CohortDataImport>& importers, const matrix_type& measurements, const matrix_type& design, const matrix_type& contrasts) :
          GLMTestBase (measurements, design, contrasts),
          importers (importers)
      {
        // Make sure that the specified contrasts reflect the full design matrix (with additional
        //   data loaded)
        assert (contrasts.cols() == X.cols() + importers.size());
      }



      void GLMTTestVariable::operator() (const vector<size_t>& perm_labelling, matrix_type& output) const
      {
        output = matrix_type::Zero (num_elements(), num_outputs());
        matrix_type tvalues, betas, residuals;

        // Set the size of the permuted design matrix to include the additional columns
        //   that will be imported from external files
        matrix_type SX (X.rows(), X.cols() + importers.size());

        // Pre-permute the fixed contents of the design matrix
        for (ssize_t row = 0; row != X.rows(); ++row)
          SX.block(row, 0, 1, X.cols()) = X.row (perm_labelling[row]);

        // Loop over all elements in the input image
        for (ssize_t element = 0; element != y.rows(); ++element) {

          // For each element (row in y), need to load the additional data for that element
          //   for all subjects in order to construct the design matrix
          // Would it be preferable to pre-calculate and store these per-element design matrices,
          //   rather than re-generating them each time? (More RAM, less CPU)
          // No, most of the time that subject data will be memory-mapped, so pre-loading (in
          //   addition to the duplication of the fixed design matrix contents) would hurt bad
          matrix_type extra_data (X.rows(), importers.size());
          for (ssize_t col = 0; col != ssize_t(importers.size()); ++col)
            extra_data.col(col) = importers[col] (element);

          // Make sure the data from the additional columns is appropriately permuted
          //   (i.e. in the same way as what the fixed portion of the design matrix experienced)
          for (ssize_t row = 0; row != X.rows(); ++row)
            SX.block(row, X.cols(), 1, importers.size()) = extra_data.row(perm_labelling[row]);

          // This call doesn't need pre-scaling of contrasts,
          //   nor does it need a pre-computed pseudo-inverse of the design matrix
          ttest (tvalues, SX.transpose(), y.row(element), betas, residuals);

          for (size_t col = 0; col != num_outputs(); ++col) {
            value_type val = tvalues (element, col);
            if (!std::isfinite (val))
              val = value_type(0);
            output(element, col) = val;
          }

        }
      }



      matrix_type GLMTTestVariable::default_design (const matrix_type& design, const size_t index) const
      {
        matrix_type output (design.rows(), design.cols() + importers.size());
        output.block (0, 0, design.rows(), design.cols()) = design;
        for (size_t i = 0; i != importers.size(); ++i)
          output.col (design.cols() + i) = importers[i] (index);
        return output;
      }



      void GLMTTestVariable::ttest (matrix_type& tvalues,
                                    const matrix_type& design,
                                    const matrix_type& measurements,
                                    matrix_type& betas,
                                    matrix_type& residuals) const
      {
        const matrix_type pinv_design = Math::pinv (design);
        betas.noalias() = measurements * pinv_design;
        residuals.noalias() = measurements - betas * design;
        const matrix_type XtX = design.transpose() * design;
        const matrix_type pinv_XtX = (XtX.transpose() * XtX).fullPivLu().solve (XtX.transpose());
        const size_t degrees_of_freedom = design.rows() - rank(design);
        tvalues.noalias() = betas * c;
        for (size_t n = 0; n != size_t(tvalues.rows()); ++n) {
          const default_type variance = residuals.row(n).squaredNorm() / degrees_of_freedom;
          tvalues.row(n).array() /= sqrt(variance * c.row(n).dot (pinv_XtX * c.row(n).transpose()));
        }
      }










      GLMFTestFixed::GLMFTestFixed (const matrix_type& measurements, const matrix_type& design, const matrix_type& contrasts, const matrix_type& ftests) :
        GLMTestBase (measurements, design, contrasts),
        ftests (ftests) { }



      void GLMFTestFixed::operator() (const vector<size_t>& perm_labelling, matrix_type& output) const
      {

      }









    }
  }
}

