/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#pragma once

#include "dwi/tractography/ACT/gmwmi.h"
#include "dwi/tractography/properties.h"
#include "memory.h"

namespace MR::DWI::Tractography::ACT {

class ACT_Shared_additions {

public:
  ACT_Shared_additions(std::string_view path, Properties &property_set) : voxel(Image<float>::open(path)), bt(false) {
    verify_5TT_image(voxel);
    property_set.set(bt, "backtrack");
    if (property_set.find("crop_at_gmwmi") != property_set.end())
      gmwmi_finder.reset(new GMWMI_finder(voxel));
  }

  bool backtrack() const { return bt; }

  bool crop_at_gmwmi() const { return bool(gmwmi_finder); }
  void crop_at_gmwmi(std::vector<Eigen::Vector3f> &tck) const {
    assert(gmwmi_finder);
    tck.back() = gmwmi_finder->find_interface(tck, true);
  }

private:
  Image<float> voxel;
  bool bt;

  std::unique_ptr<GMWMI_finder> gmwmi_finder;

protected:
  friend class ACT_Method_additions;
};

} // namespace MR::DWI::Tractography::ACT
