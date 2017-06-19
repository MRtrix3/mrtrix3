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

#include "bitset.h"
#include "debug.h"

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


        void all_stats (const matrix_type& measurements,
                        const matrix_type& design,
                        const matrix_type& contrasts,
                        matrix_type& betas,
                        matrix_type& abs_effect_size,
                        matrix_type& std_effect_size,
                        matrix_type& stdev)
        {
          betas = solve_betas (measurements, design);
          //std::cerr << "Betas: " << betas.rows() << " x " << betas.cols() << ", max " << betas.array().maxCoeff() << "\n";
          abs_effect_size = contrasts * betas;
          //std::cerr << "abs_effect_size: " << abs_effect_size.rows() << " x " << abs_effect_size.cols() << ", max " << abs_effect_size.array().maxCoeff() << "\n";
          matrix_type residuals = measurements.transpose() - design * betas;
          residuals = residuals.array().pow(2.0);
          //std::cerr << "residuals: " << residuals.rows() << " x " << residuals.cols() << ", max " << residuals.array().maxCoeff() << "\n";
          matrix_type one_over_dof (1, measurements.cols());
          one_over_dof.fill (1.0 / value_type(design.rows()-Math::rank (design)));
          //std::cerr << "one_over_dof: " << one_over_dof.rows() << " x " << one_over_dof.cols() << ", max " << one_over_dof.array().maxCoeff() << "\n";
          //VAR (design.rows());
          //VAR (Math::rank (design));
          stdev = (one_over_dof * residuals).array().sqrt();
          //std::cerr << "stdev: " << stdev.rows() << " x " << stdev.cols() << ", max " << stdev.array().maxCoeff() << "\n";
          std_effect_size = abs_effect_size.array() / stdev.array();
          //std::cerr << "std_effect_size: " << std_effect_size.rows() << " x " << std_effect_size.cols() << ", max " << std_effect_size.array().maxCoeff() << "\n";
          //TRACE;
        }

      }








      GLMTTestFixed::GLMTTestFixed (const matrix_type& measurements, const matrix_type& design, const matrix_type& contrasts) :
          GLMTestBase (measurements, design, contrasts),
          pinvX (Math::pinv (X)),
          scaled_contrasts_t (scale_contrasts().transpose())
      {
        assert (contrasts.cols() == design.cols());
      }



      void GLMTTestFixed::operator() (const vector<size_t>& perm_labelling, vector_type& output) const
      {
        output = vector_type::Zero (y.rows());
        matrix_type tvalues, betas, residuals_t, SX_t, pinvSX_t;

        // TODO Currently the entire design matrix is permuted;
        //   we may instead prefer Freedman-Lane
        // This however would be different for each row in the contrasts matrix,
        //   since the columns that correspond to nuisance variables
        //   varies between rows

        SX_t.resize (X.rows(), X.cols());
        pinvSX_t.resize (pinvX.rows(), pinvX.cols());
        for (ssize_t i = 0; i < X.rows(); ++i) {
          SX_t.row(i) = X.row (perm_labelling[i]);
          pinvSX_t.col(i) = pinvX.col (perm_labelling[i]);
        }

        SX_t.transposeInPlace();
        pinvSX_t.transposeInPlace();
        for (ssize_t i = 0; i < y.rows(); i += GLM_BATCH_SIZE) {
          const matrix_type tmp = y.block (i, 0, std::min (GLM_BATCH_SIZE, (int)(y.rows()-i)), y.cols());
          ttest (tvalues, SX_t, pinvSX_t, tmp, betas, residuals_t);
          for (ssize_t n = 0; n < tvalues.rows(); ++n) {
            value_type val = tvalues(n,0);
            if (!std::isfinite (val))
              val = value_type(0);
            output[i+n] = val;
          }
        }
      }



      void GLMTTestFixed::ttest (matrix_type& tvalues,
                                 const matrix_type& design_t,
                                 const matrix_type& pinv_design_t,
                                 const matrix_type& measurements,
                                 matrix_type& betas,
                                 matrix_type& residuals_t) const
      {
        betas.noalias() = measurements * pinv_design_t;
        residuals_t.noalias() = measurements - betas * design_t;
        tvalues.noalias() = betas * scaled_contrasts_t;
        for (size_t n = 0; n < size_t(tvalues.rows()); ++n)
          tvalues.row(n).array() /= residuals_t.row(n).norm();
      }




      // scale contrasts for use in ttest() member function
      /* This function pre-scales a contrast matrix in order to make conversion from GLM betas
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
      matrix_type GLMTTestFixed::scale_contrasts() const
      {
        const matrix_type XtX = X.transpose() * X;
        const matrix_type pinv_XtX = (XtX.transpose() * XtX).fullPivLu().solve (XtX.transpose());
        const size_t degrees_of_freedom = X.rows() - Math::rank (X);
        matrix_type scaled_contrasts (c);
        for (size_t n = 0; n < size_t(c.rows()); ++n) {
          const auto ct_pinv_XtX_c = c.row(n).dot (pinv_XtX * c.row(n).transpose());
          scaled_contrasts.row(n) *= std::sqrt (value_type(degrees_of_freedom) / ct_pinv_XtX_c);
        }
        return scaled_contrasts;
      }









      GLMTTestVariable::GLMTTestVariable (const vector<CohortDataImport>& importers, const matrix_type& measurements, const matrix_type& design, const matrix_type& contrasts, const bool nans_in_data, const bool nans_in_columns) :
          GLMTestBase (measurements, design, contrasts),
          importers (importers),
          nans_in_data (nans_in_data),
          nans_in_columns (nans_in_columns)
      {
        // Make sure that the specified contrasts reflect the full design matrix (with additional
        //   data loaded)
        assert (contrasts.cols() == X.cols() + ssize_t(importers.size()));
      }



      void GLMTTestVariable::operator() (const vector<size_t>& perm_labelling, vector_type& output) const
      {
        output = vector_type::Zero (y.rows());
        vector_type tvalues, betas, residuals;

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

          // If there are non-finite values present either in the input
          //   data or the element-wise design matrix columns (or both),
          //   need to track which rows are being kept / discarded
          BitSet row_mask (X.rows(), true);
          if (nans_in_data) {
            for (ssize_t row = 0; row != y.rows(); ++row) {
              if (!std::isfinite (y (row, element)))
                row_mask[row] = false;
            }
          }
          if (nans_in_columns) {
            // Bear in mind that we need to test for finite values in the
            //   row in which this data is going to be written to based on
            //   the permutation labelling
            for (ssize_t row = 0; row != extra_data.rows(); ++row) {
              if (!extra_data.row (perm_labelling[row]).allFinite())
                row_mask[row] = false;
            }
          }

          // Do we need to reduce the size of our matrices / vectors
          //   based on the presence of non-finite values?
          if (row_mask.full()) {

            // Make sure the data from the additional columns is appropriately permuted
            //   (i.e. in the same way as what the fixed portion of the design matrix experienced)
            for (ssize_t row = 0; row != X.rows(); ++row)
              SX.block(row, X.cols(), 1, importers.size()) = extra_data.row (perm_labelling[row]);

            ttest (tvalues, SX, y.row(element), betas, residuals);

          } else {

            const ssize_t new_num_rows = row_mask.count();
            vector_type y_masked (new_num_rows);
            matrix_type SX_masked (new_num_rows, X.cols() + importers.size());
            ssize_t new_row = 0;
            for (ssize_t old_row = 0; old_row != X.rows(); ++old_row) {
              if (row_mask[old_row]) {
                y_masked[new_row] = y(old_row, element);
                SX_masked.block (new_row, 0, 1, X.cols()) = SX.block (old_row, 0, 1, X.cols());
                SX_masked.block (new_row, X.cols(), 1, importers.size()) = extra_data.row (perm_labelling[old_row]);
                ++new_row;
              }
            }
            assert (new_row == new_num_rows);

            ttest (tvalues, SX_masked, y_masked, betas, residuals);

          }

          // FIXME
          // Currently output only the first contrast, as is done in GLMTTestFixed
          // tvalues should have one row only (since we're only testing a single row), and
          //   number of columns equal to the number of contrasts
          value_type val = tvalues[0];
          if (!std::isfinite (val))
            val = value_type(0);
          output[element] = val;

        }
      }



      void GLMTTestVariable::ttest (vector_type& tvalues,
                                    const matrix_type& design,
                                    const vector_type& measurements,
                                    vector_type& betas,
                                    vector_type& residuals) const
      {
        //std::cerr << "Design: " << design.rows() << " x " << design.cols() << ", max " << design.array().maxCoeff() << "\n";
        //std::cerr << "Measurements: " << measurements.rows() << " x " << measurements.cols() << ", max " << measurements.array().maxCoeff() << "\n";
        matrix_type pinv_design = Math::pinv (design);
        //std::cerr << "PINV Design: " << pinv_design.rows() << " x " << pinv_design.cols() << ", max " << pinv_design.array().maxCoeff() << "\n";
        const matrix_type XtX = design.transpose() * design;
        //std::cerr << "XtX: " << XtX.rows() << " x " << XtX.cols() << ", max " << XtX.array().maxCoeff() << "\n";
        const matrix_type pinv_XtX = (XtX.transpose() * XtX).fullPivLu().solve (XtX.transpose());
        //std::cerr << "PINV XtX: " << pinv_XtX.rows() << " x " << pinv_XtX.cols() << ", max " << pinv_XtX.array().maxCoeff() << "\n";
        betas = pinv_design * measurements.matrix();
        //std::cerr << "Betas: " << betas.rows() << " x " << betas.cols() << ", max " << betas.array().maxCoeff() << "\n";
        residuals = measurements - (design * betas.matrix()).array();
        //std::cerr << "Residuals: " << residuals.rows() << " x " << residuals.cols() << ", max " << residuals.array().maxCoeff() << "\n";
        tvalues = c * betas.matrix();
        //std::cerr << "T-values: " << tvalues.rows() << " x " << tvalues.cols() << ", max " << tvalues.array().maxCoeff() << "\n";
        //VAR (Math::rank (design));
        const default_type variance = residuals.matrix().squaredNorm() / default_type(design.rows() - Math::rank(design));
        //VAR (variance);
        // The fact that we're only able to test one element at a time here should be
        //   placing a restriction on the dimensionality of tvalues
        // Previously, could be (number of elements) * (number of contrasts);
        //   now can only reflect the number of contrasts
        for (size_t n = 0; n != size_t(tvalues.size()); ++n) {
          const default_type ct_pinv_XtX_c = c.row(n).dot (pinv_XtX * c.row(n).transpose());
          //VAR (ct_pinv_XtX_c);
          tvalues[n] /= std::sqrt (variance * ct_pinv_XtX_c);
        }
        //std::cerr << "T-values: " << tvalues.rows() << " x " << tvalues.cols() << ", max " << tvalues.array().maxCoeff() << "\n";
      }



      matrix_type GLMTTestVariable::default_design (const size_t index) const
      {
        matrix_type output (X.rows(), X.cols() + importers.size());
        output.block (0, 0, X.rows(), X.cols()) = X;
        for (size_t i = 0; i != importers.size(); ++i)
          output.col (X.cols() + i) = importers[i] (index);
        return output;
      }










      GLMFTestFixed::GLMFTestFixed (const matrix_type& measurements, const matrix_type& design, const matrix_type& contrasts, const matrix_type& ftests) :
        GLMTestBase (measurements, design, contrasts),
        ftests (ftests) { }



      void GLMFTestFixed::operator() (const vector<size_t>& perm_labelling, vector_type& output) const
      {

      }









    }
  }
}

