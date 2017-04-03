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

        matrix_type scale_contrasts (const matrix_type& contrasts, const matrix_type& design, const size_t degrees_of_freedom)
        {
          assert (contrasts.cols() == design.cols());
          const matrix_type XtX = design.transpose() * design;
          const matrix_type pinv_XtX = (XtX.transpose() * XtX).fullPivLu().solve (XtX.transpose());
          matrix_type scaled_contrasts (contrasts);

          for (size_t n = 0; n < size_t(contrasts.rows()); ++n) {
            auto pinv_XtX_c = pinv_XtX * contrasts.row(n).transpose();
            scaled_contrasts.row(n) *= std::sqrt (value_type(degrees_of_freedom) / contrasts.row(n).dot (pinv_XtX_c));
          }
          return scaled_contrasts;
        }



        void ttest_prescaled (matrix_type& tvalues,
                              const matrix_type& design,
                              const matrix_type& pinv_design,
                              const matrix_type& measurements,
                              const matrix_type& scaled_contrasts,
                              matrix_type& betas,
                              matrix_type& residuals)
        {
          betas.noalias() = measurements * pinv_design;
          residuals.noalias() = measurements - betas * design;
          tvalues.noalias() = betas * scaled_contrasts;
          for (size_t n = 0; n < size_t(tvalues.rows()); ++n)
            tvalues.row(n).array() /= residuals.row(n).norm();
        }



        void ttest (matrix_type& tvalues,
                    const matrix_type& design,
                    const matrix_type& measurements,
                    const matrix_type& contrasts,
                    matrix_type& betas,
                    matrix_type& residuals)
        {
          const matrix_type pinv_design = Math::pinv (design);
          betas.noalias() = measurements * pinv_design;
          residuals.noalias() = measurements - betas * design;
          const matrix_type XtX = design.transpose() * design;
          const matrix_type pinv_XtX = (XtX.transpose() * XtX).fullPivLu().solve (XtX.transpose());
          const size_t degrees_of_freedom = design.rows() - rank(design);
          tvalues.noalias() = betas * contrasts;
          for (size_t n = 0; n != size_t(tvalues.rows()); ++n) {
            const default_type variance = residuals.row(n).squaredNorm() / degrees_of_freedom;
            tvalues.row(n).array() /= sqrt(variance * contrasts.row(n).dot (pinv_XtX * contrasts.row(n).transpose()));
          }
        }




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
          scaled_contrasts (GLM::scale_contrasts (contrast, X, X.rows()-rank(X)).transpose()) { }



      void GLMTTestFixed::operator() (const vector<size_t>& perm_labelling, vector_type& output) const
      {
        output = vector_type::Zero (y.rows());
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
          const matrix_type tmp = y.block (i, 0, std::min (GLM_BATCH_SIZE, (int)(y.rows()-i)), y.cols());
          GLM::ttest_prescaled (tvalues, SX, pinvSX, tmp, scaled_contrasts, betas, residuals);
          for (ssize_t n = 0; n < tvalues.rows(); ++n) {
            value_type val = tvalues(n,0);
            if (!std::isfinite (val))
              val = value_type(0);
            output[i+n] = val;
          }
        }
      }



      GLMTTestVariable::GLMTTestVariable (vector<CohortDataImport>& importers, const matrix_type& measurements, const matrix_type& design, const matrix_type& contrasts) :
          GLMTestBase (measurements, design, contrasts),
          importers (importers)
      {
        // Make sure that the specified contrasts reflect the full design matrix (with additional
        //   data loaded)
        assert (contrasts.cols() == X.cols() + importers.size());
      }



      void GLMTTestVariable::operator() (const vector<size_t>& perm_labelling, vector_type& output) const
      {
        output = vector_type::Zero (y.rows());
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

          // Need to pre-scale contrasts if we want to use the ttest() function;
          //   otherwise, need to define a different function that doesn't rely on pre-scaling
          // Went for the latter option; this call doesn't need pre-scaling of contrasts,
          //   nor does it need a pre-computed pseudo-inverse of the design matrix
          GLM::ttest (tvalues, SX.transpose(), y.row(element), c, betas, residuals);

          // FIXME
          // Currently output only the first contrast, as is done in GLMTTestFixed
          // tvalues should have one row only (since we're only testing a single row), and
          //   number of columns equal to the number of contrasts
          value_type val = tvalues (element, 0);
          if (!std::isfinite (val))
            val = value_type(0);
          output[element] = val;

        }
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

