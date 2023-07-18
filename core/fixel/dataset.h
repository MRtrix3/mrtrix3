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


#ifndef __fixel_dataset_h__
#define __fixel_dataset_h__

#include "types.h"
#include "math/sphere/set/assigner.h"
#include "fixel/fixel.h"
#include "fixel/indeximage.h"

namespace MR
{
  namespace Fixel
  {



    class Dataset : public Fixel::IndexImage
    {

      public:
        using BaseType = Fixel::IndexImage;
        using directions_type = Eigen::Matrix<float, Eigen::Dynamic, 3, Eigen::RowMajor>;
        using direction_type = directions_type::ConstRowXpr;
        using fixel_1d_data_type = Eigen::Array<default_type, Eigen::Dynamic, 1>;
        using fixel_2d_data_type = Eigen::Array<default_type, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
        using dixelmasks_type = Eigen::Array<bool, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
        using dixelmask_type = dixelmasks_type::ConstRowXpr;

        Dataset (const std::string&);

        const std::string& name() const { return directory_path; }
        direction_type dir (const index_type index) const { assert (index < nfixels()); return directions.row (index); }
        dixelmask_type mask (const index_type index) const { assert (have_fixel_masks()); assert (index < nfixels()); return dixel_masks.row (index); }
        const Math::Sphere::Set::Assigner& dixels() const { return mask_directions; }
        bool have_fixel_masks() const { return dixel_masks.rows(); }

      protected:
        const std::string directory_path;
        directions_type directions;

        dixelmasks_type dixel_masks;
        Math::Sphere::Set::Assigner mask_directions;

    };







    // TODO Make use of this class in downstream applications
    // Between SIFT and SIFT2, there are different Fixel classes, which have different numbers of fixel-wise variables
    // Could these just be stored as a vector<class Fixel> within the Dataset class?
    //   (and that class could omit any direction / mask information, which would if anything improve cache performance)
    // Bear in mind that Dataset is intended to be thread-safe,
    //   and there are processes that generate those data rather than just reading from it
    // Options:
    // 1.  Store those data as a member variable of Dataset;
    //     will need to spawn derivative classes to perform multi-threaded processing and update accordingly
    // 2.  Store those data in a derivative Shared class;
    //     this would however incur a pointer dereference operation every time data are read
    //
    // Other thing to contemplate is the mechanism by which one assigns streamlines to fixels
    // Possibilities:
    // 1.  Fixel provides some form of tailored affinity function, for which a functor would need to be specified
    // 2.  Dixel mask data are available, in which case the tangent is assigned to the nearest dixel and those
    //     masks are then queried (note that some dixels may be attributed to multiple fixels)
    // 3.  Resort to nearest direction within threshold
    //
    //     (can the template Fixel class refer back to the dixel mask data from the master class within an affinity function?)





  }
}

#endif
