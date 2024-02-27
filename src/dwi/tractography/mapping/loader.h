/* Copyright (c) 2008-2024 the MRtrix3 contributors.
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

#ifndef __dwi_tractography_mapping_loader_h__
#define __dwi_tractography_mapping_loader_h__

#include "dwi/tractography/file.h"
#include "dwi/tractography/streamline.h"
#include "memory.h"
#include "progressbar.h"
#include "thread_queue.h"

namespace MR::DWI::Tractography::Mapping {

class TrackLoader {

public:
  TrackLoader(Reader<> &file, const size_t to_load = 0, const std::string &msg = "mapping tracks to image")
      : reader(file),
        tracks_to_load(to_load),
        progress(!msg.empty() ? new ProgressBar(msg, tracks_to_load) : nullptr) {}

  virtual ~TrackLoader() {}
  virtual bool operator()(Streamline<> &out) {
    if (!reader(out)) {
      progress.reset();
      return false;
    }
    if (tracks_to_load && out.get_index() >= tracks_to_load) {
      out.clear();
      progress.reset();
      return false;
    }
    if (progress)
      ++(*progress);
    return true;
  }

protected:
  Reader<> &reader;
  const size_t tracks_to_load;
  std::unique_ptr<ProgressBar> progress;
};

} // namespace MR::DWI::Tractography::Mapping

#endif
