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

#ifndef __dwi_tractography_sift_model_base_h__
#define __dwi_tractography_sift_model_base_h__

#include "app.h"
#include "image.h"
#include "thread_queue.h"
#include "algo/copy.h"
#include "math/sphere/set/adjacency.h"
#include "fixel/fixel.h"
#include "fixel/helpers.h"
#include "file/path.h"
#include "file/utils.h"

#include "dwi/fmls.h"

#include "dwi/tractography/file.h"

#include "dwi/tractography/ACT/tissues.h"

#include "dwi/tractography/mapping/loader.h"
#include "dwi/tractography/mapping/mapper.h"
#include "dwi/tractography/mapping/mapping.h"
#include "dwi/tractography/mapping/voxel.h"

#include "dwi/tractography/SIFT/types.h"





//#define SIFT_MODEL_OUTPUT_SH_IMAGES
#define SIFT_MODEL_OUTPUT_FIXEL_IMAGES


namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT
      {



        extern const App::OptionGroup SIFTModelWeightsOption;



        class ModelBase : public MR::Fixel::Dataset
        {
          public:
            using BaseType = MR::Fixel::Dataset;
            using value_type = SIFT::value_type;
            using data_matrix_type = Eigen::Array<value_type, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;


            // TODO Redo using strongly typed enum?
            static const ssize_t model_weight_column = 0;
            static const ssize_t fd_column = 1;
            static const ssize_t td_column = 2;
            static const ssize_t track_count_column = 3;


            template <class RowType>
            class ConstFixelFunctions : public RowType
            {
              public:
                ConstFixelFunctions (const ModelBase& model, const MR::Fixel::index_type index) :
                    RowType (model.fixels.row(index)) { }
                ConstFixelFunctions (RowType&& row) :
                    RowType (row) { }

                value_type weight() const { return (*this)[model_weight_column]; }
                value_type fd()     const { return (*this)[fd_column]; }
                value_type td()     const { return (*this)[td_column]; }
                track_t    count()  const { return track_t((*this)[track_count_column]); }

                value_type get_diff (const value_type mu) const { return ((td() * mu) - fd()); }
                value_type get_cost (const value_type mu) const { return get_cost_unweighted (mu) * weight(); }

              private:
                value_type get_cost_unweighted (const value_type mu) const { return Math::pow2 (get_diff (mu)); }
            };

            template <class RowType>
            class MutableFixelFunctions : public ConstFixelFunctions<RowType>
            {
              public:
                MutableFixelFunctions (ModelBase& model, const MR::Fixel::index_type index) :
                    ConstFixelFunctions<RowType> (model.fixels.row(index)) { }
                MutableFixelFunctions (RowType&& row) :
                    ConstFixelFunctions<RowType> (row) { }
                using ConstFixelFunctions<RowType>::weight;
                using ConstFixelFunctions<RowType>::fd;
                using ConstFixelFunctions<RowType>::td;
                using ConstFixelFunctions<RowType>::count;
                void add (const value_type l, const track_t c) { (*this)[td_column] += l; (*this)[track_count_column] += value_type(c); }
                void reset() { (*this)[td_column] = 0.0; (*this)[track_count_column] = 0.0; }
                void set_fd (const value_type value) { (*this)[fd_column] = value; }
                void set_weight (const value_type value) { (*this)[model_weight_column] = value; }

                MutableFixelFunctions& operator+= (const value_type value) { (*this)[td_column] += value; (*this)[track_count_column] += 1.0; return *this; }
                MutableFixelFunctions& operator-= (const value_type value) { assert (count()); (*this)[td_column] = std::max(td() - value, 0.0); (*this)[track_count_column] -= 1.0; return *this; }
            };

            using ConstFixelBase = ConstFixelFunctions<data_matrix_type::ConstRowXpr>;
            using FixelBase = MutableFixelFunctions<data_matrix_type::RowXpr>;


            ModelBase (const std::string& fd_path);


            void map_streamlines (const std::string&);

            void scale_FDs_by_GM();

            value_type mu() const { return (FD_sum / TD_sum); }
            bool have_act_data() const { return act_5tt.valid(); }

            virtual bool operator() (const Mapping::Set<Mapping::Fixel>& in);

            value_type calc_cost_function() const;

            void output_5tt_image (const std::string&);

            // TODO Not yet sure what I want to do about the debugging images;
            //   don't like how they're currently handled, but need them to be there
            //   to make sure code conversion is correct
            void initialise_debug_image_output (const std::string&) const;
            void output_all_debug_images (const std::string&, const std::string&) const;

          protected:
            std::string tractogram_path;

            MR::Image<float> act_5tt;

            data_matrix_type fixels;

            value_type FD_sum, TD_sum;

            // TODO Dummy dixel mask to which fixels can point if the
            //   input fixel dataset does not include such
            Eigen::Array<bool, Eigen::Dynamic, Eigen::Dynamic> dummy_dixelmask;

            void load_5tt_image (const std::string&);
            void set_model_weights (const std::string&);


            void output_target_voxel (const std::string&) const;
            void output_target_sh (const std::string&) const;
            void output_target_fixel (const std::string&) const;
            void output_tdi_voxel (const std::string&) const;
            void output_tdi_sh (const std::string&) const;
            void output_tdi_fixel (const std::string&) const;
            void output_errors_voxel (const std::string&, const std::string&, const std::string&, const std::string&) const;
            void output_errors_fixel (const std::string&, const std::string&, const std::string&) const;
            void output_scatterplot (const std::string&) const;

            ConstFixelBase operator[] (const MR::Fixel::index_type index) const { return ConstFixelBase (*this, index); }
            FixelBase operator[] (const MR::Fixel::index_type index) { return FixelBase (*this, index); }

            // TODO Can we define a macro / template that will allow us to quickly define an iterator
            //   for any derivative class, with any fixel type?


        };




      }
    }
  }
}


#endif
