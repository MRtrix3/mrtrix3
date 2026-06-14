/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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

// The ".tck" Reader / Writer classes formerly defined here are now the
//   read/write backends of the ".tck" format handler and live in
//   dwi/tractography/formats/tck.h. This header is retained as a compatibility
//   shim so that the many existing callers of
//   MR::DWI::Tractography::Reader<> / Writer<> (and the ReaderInterface<> /
//   WriterInterface<> contracts) continue to compile unchanged.

#include "dwi/tractography/formats/tck.h"

// Preserve the transitive include surface that this header historically
//   exposed, so that callers relying on it continue to compile unchanged.
#include "app.h"
#include "file/config.h"
#include "file/key_value.h"
#include "file/matrix.h"
#include "file/ofstream.h"
#include "memory.h"
