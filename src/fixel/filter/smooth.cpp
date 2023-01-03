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


#include "fixel/filter/smooth.h"

#include "image_helpers.h"
#include "thread_queue.h"
#include "transform.h"
#include "fixel/helpers.h"

namespace MR
{
  namespace Fixel
  {
    namespace Filter
    {



      Smooth::Smooth (Image<index_type> index_image,
                      const Matrix::Reader& matrix,
                      const Image<bool>& mask_image,
                      const float smoothing_fwhm,
                      const float smoothing_threshold) :
          mask_image (mask_image),
          matrix (matrix),
          threshold (smoothing_threshold)
      {
        set_fwhm (smoothing_fwhm);
        // For smoothing, we need to be able to quickly
        //   calculate the distance between any pair of fixels
        fixel_positions.resize (matrix.size());
        const Transform transform (index_image);
        for (auto i = Loop (index_image, 0, 3) (index_image); i; ++i) {
          const Eigen::Vector3d vox ((default_type)index_image.index(0), (default_type)index_image.index(1), (default_type)index_image.index(2));
          const Eigen::Vector3f scanner = (transform.voxel2scanner * vox).cast<float>();
          index_image.index(3) = 0;
          const index_type count = index_image.value();
          index_image.index(3) = 1;
          const index_type offset = index_image.value();
          for (size_t fixel_index = 0; fixel_index != count; ++fixel_index)
            fixel_positions[offset + fixel_index] = scanner;
        }
      }

      Smooth::Smooth (Image<index_type> index_image,
                      const Matrix::Reader& matrix,
                      const Image<bool>& mask_image) :
          Smooth (index_image, matrix, mask_image, DEFAULT_FIXEL_SMOOTHING_FWHM, DEFAULT_FIXEL_SMOOTHING_MINWEIGHT) { }

      Smooth::Smooth (Image<index_type> index_image,
                      const Matrix::Reader& matrix,
                      const float smoothing_fwhm,
                      const float smoothing_threshold) :
          Smooth (index_image, matrix, Image<bool>(), smoothing_fwhm, smoothing_threshold)
      {
        Header mask_header = Fixel::data_header_from_index (index_image);
        mask_header.datatype() = DataType::Bit;
        assert (size_t(mask_header.size(0)) == matrix.size());
        mask_image = Image<bool>::scratch (mask_header, "full scratch fixel mask");
        for (auto l = Loop(0) (mask_image); l; ++l)
          mask_image.value() = true;
      }

      Smooth::Smooth (Image<index_type> index_image,
                      const Matrix::Reader& matrix) :
          Smooth (index_image, matrix, DEFAULT_FIXEL_SMOOTHING_FWHM, DEFAULT_FIXEL_SMOOTHING_MINWEIGHT) { }



      void Smooth::set_fwhm (const float fwhm)
      {
        stdev = fwhm / 2.3548f;
        gaussian_const1 = 1.0 / (stdev * std::sqrt (2.0 * Math::pi));
        gaussian_const2 = -1.0 / (2.0 * stdev * stdev);
      }



      // TODO Provide an alternative functor for when the input & output images contain more than one column
      //   (more efficient to construct the smoothing kernel once per fixel, then loop over image channels)
      void Smooth::operator() (Image<float>& input, Image<float>& output) const
      {
        Fixel::check_data_file (input);
        Fixel::check_data_file (output);

        check_dimensions (input, output);

        if (size_t (input.size(0)) != matrix.size())
          throw Exception ("Size of fixel data file \"" + input.name() + "\" (" + str(input.size(0)) +
                           ") does not match fixel connectivity matrix (" + str(matrix.size()) + ")");

        class Source
        { NOMEMALIGN
          public:
            Source (const size_t N) :
                number (N),
                counter (0) { }
            bool operator() (size_t& fixel)
            {
              if ((fixel = counter) == number)
                return false;
              ++counter;
              return true;
            }
          private:
            const size_t number;
            size_t counter;
        };

        class Worker
        { MEMALIGN(Worker)
          public:
            Worker (const Smooth& master, const Image<float>& input, const Image<float>& output) :
                master (master),
                matrix (master.matrix),
                input (input),
                output (output),
                mask (master.mask_image) { }

            bool operator() (const size_t fixel)
            {
              mask.index(0) = output.index(0) = fixel;
              if (mask.value()) {
                const Eigen::Vector3f& pos (master.fixel_positions[fixel]);
                const auto connectivity = matrix[fixel];
                default_type sum_weights (0.0);
                output.value() = 0.0;
                for (const auto& c : connectivity) {
                  mask.index (0) = input.index(0) = c.index();
                  if (mask.value() && std::isfinite (static_cast<float>(input.value()))) {
                    const Matrix::connectivity_value_type weight = c.value() * master.gaussian_const1 * std::exp (master.gaussian_const2 * (master.fixel_positions[c.index()] - pos).squaredNorm());
                    if (weight >= master.threshold) {
                      output.value() += weight * input.value();
                      sum_weights += weight;
                    }
                  }
                }
                if (sum_weights) {
                  output.value() /= sum_weights;
                } else if (connectivity.empty()) {
                  // Provide unsmoothed value if disconnected
                  input.index(0) = fixel;
                  output.value() = input.value();
                } else {
                  output.value() = std::numeric_limits<float>::quiet_NaN();
                }
              } else {
                output.value() = std::numeric_limits<float>::quiet_NaN();
              }
              return true;
            }

          private:
            const Smooth& master;
            // Need a local copy of each of these
            Matrix::Reader matrix;
            Image<float> input;
            Image<float> output;
            Image<bool> mask;
        };

        Thread::run_queue (Source (input.size (0)),
                           Thread::batch (size_t()),
                           Thread::multi (Worker (*this, input, output)));

      }



    }
  }
}

