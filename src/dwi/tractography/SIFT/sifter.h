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

#ifndef __dwi_tractography_sift_sifter_h__
#define __dwi_tractography_sift_sifter_h__



#include "image.h"
#include "types.h"

#include "math/rng.h"
#include "math/sphere/set/adjacency.h"

#include "dwi/tractography/SIFT/gradient_sort.h"
#include "dwi/tractography/SIFT/model.h"
#include "dwi/tractography/SIFT/track_index_range.h"
#include "dwi/tractography/SIFT/types.h"



namespace MR
{
  namespace DWI
  {
    namespace Tractography
    {
      namespace SIFT
      {


      class SIFTer : public Model
      {

        public:

        class Fixel : public ModelBase::FixelBase
        {
          public:
            using BaseType = ModelBase::FixelBase;
            using BaseType::BaseType;

            value_type get_d_cost_d_mu    (const value_type mu)                             const { return get_d_cost_d_mu_unweighted    (mu) * weight(); }
            value_type get_cost_wo_track  (const value_type mu, const value_type length)    const { return get_cost_wo_track_unweighted  (mu, length)    * weight(); }
            value_type get_cost_manual_TD (const value_type mu, const value_type manual_TD) const { return get_cost_manual_TD_unweighted (mu, manual_TD) * weight(); }
            value_type calc_quantisation  (const value_type mu, const value_type length)    const { return get_cost_manual_TD            (mu, (fd()/mu) + length); }

          private:
            value_type get_d_cost_d_mu_unweighted    (const value_type mu) const { return (2.0 * td() * get_diff (mu)); }
            value_type get_cost_wo_track_unweighted  (const value_type mu, const value_type length)    const { return (Math::pow2 ((std::max (td()-length, 0.0) * mu) - fd())); }
            value_type get_cost_manual_TD_unweighted (const value_type mu, const value_type manual_TD) const { return  Math::pow2 ((        manual_TD           * mu) - fd()); }
        };






        SIFTer (const std::string& fd_path) :
            Model (fd_path),
            term_number (0),
            term_ratio (0.0),
            term_mu (0.0),
            enforce_quantisation (true) { }

        SIFTer (const SIFTer& that) = delete;

        ~SIFTer() { }


        // CORE OPERATIONS
        void perform_filtering();
        void output_filtered_tracks (const std::string&, const std::string&) const;
        void output_selection (const std::string&) const;

        // CONFIGURATION OPTIONS
        void set_term_number (const track_t i)      { term_number = i; }
        void set_term_ratio  (const value_type i)   { term_ratio = i; }
        void set_term_mu     (const value_type i)   { term_mu = i; }
        void set_csv_path    (const std::string& i) { csv_path = i; }

        void set_regular_outputs (const vector<uint32_t>&, const std::string&);


        // DEBUGGING
        void test_sorting_block_size (const size_t) const;


        protected:

        // User-controllable settings
        vector<track_t> output_at_counts;
        std::string     debug_dir;
        track_t         term_number;
        value_type      term_ratio;
        value_type      term_mu;
        bool            enforce_quantisation;
        std::string     csv_path;


        // Convenience functions
        // TODO Would ideally be const, but currently require instantiating a Fixel that is not marked const
        value_type calc_roc_cost_function();
        value_type calc_gradient (const track_t, const value_type, const value_type);

        // TODO Revise once we have a working implementation and can go back to trying to get a
        //   const-qualified Fixel class and associated looping constuct
        //ConstFixel operator[] (const MR::Fixel::index_type f) const { return ConstFixel (*this, f); }
        Fixel operator[] (const MR::Fixel::index_type f) { return Fixel (*this, f); }



        // For calculating the streamline removal gradients in a multi-threaded fashion
        class TrackGradientCalculator
        {
          public:
            // TODO Fix const-ness of SIFTer
            TrackGradientCalculator (SIFTer& sifter, vector<Cost_fn_gradient_sort>& v, const value_type mu, const value_type r) :
                master (sifter), gradient_vector (v), current_mu (mu), current_roc_cost (r) { }
            // Fix const-ness
            bool operator() (const TrackIndexRange&);
          private:
            SIFTer& master;
            vector<Cost_fn_gradient_sort>& gradient_vector;
            const value_type current_mu, current_roc_cost;
        };


      };




      }
    }
  }
}


#endif


