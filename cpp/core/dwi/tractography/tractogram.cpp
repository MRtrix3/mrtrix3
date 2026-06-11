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

#include "dwi/tractography/tractogram.h"

#include "dwi/tractography/formats/list.h"
#include "exception.h"

namespace MR::DWI::Tractography {

namespace {

//! \brief select the handler for \a path or raise a user-interpretable error.
const Formats::Base *select_handler(const std::filesystem::path &path) {
  const Formats::Base *handler = Formats::get_handler(path);
  if (handler == nullptr)
    throw Exception("unsupported tractography file format for \"" + path.string() + "\" (unrecognised file extension)");
  return handler;
}

} // namespace

template <class ValueType>
Tractogram<ValueType>
Tractogram<ValueType>::open(const std::filesystem::path &path, Properties &properties, const OptionalHeader &grid) {
  const Formats::Base *handler = select_handler(path);
  if (!handler->can_read())
    throw Exception("tractography format \"" + handler->description + "\" does not support reading (file \"" +
                    path.string() + "\")");
  Tractogram tractogram(handler);
  tractogram.reader = handler->template read<ValueType>(path, properties, tractogram.registry, grid);
  return tractogram;
}

template <class ValueType>
Tractogram<ValueType> Tractogram<ValueType>::create(const std::filesystem::path &path,
                                                    const Properties &properties,
                                                    const FieldRegistry &registry,
                                                    const OptionalHeader &grid) {
  const Formats::Base *handler = select_handler(path);
  if (!handler->can_write())
    throw Exception("tractography format \"" + handler->description + "\" does not support writing (file \"" +
                    path.string() + "\")");
  Tractogram tractogram(handler);
  tractogram.registry = registry;
  tractogram.writer = handler->template create<ValueType>(path, properties, tractogram.registry, grid);
  return tractogram;
}

template class Tractogram<float>;
template class Tractogram<double>;

} // namespace MR::DWI::Tractography
