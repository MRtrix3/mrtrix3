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

#include "surface/filter/base.h"

#include <memory>
#include <mutex>

#include "thread_queue.h"

namespace MR
{
  namespace Surface
  {
    namespace Filter
    {



      void Base::operator() (const MeshMulti& in, MeshMulti& out) const
      {
        std::unique_ptr<ProgressBar> progress;
        if (message.size())
          progress.reset (new ProgressBar (message, in.size()));
        out.assign (in.size(), Mesh());

        std::mutex mutex;
        auto loader = [&] (size_t& index) { static size_t i = 0; index = i++; return (index != in.size()); };
        auto worker = [&] (const size_t& index) { (*this) (in[index], out[index]); if (progress) { std::lock_guard<std::mutex> lock (mutex); ++(*progress); } return true; };
        Thread::run_queue (loader, size_t(), Thread::multi (worker));
      }



    }
  }
}

