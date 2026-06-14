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

#include <memory>

#include "dwi/tractography/formats/list.h"
#include "dwi/tractography/formats/ram.h"
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

//! \brief whether the framework must interpose the RAM wrapper (Stage 15, step 2).
/*! The wrapper is selected whenever the chosen \a handler offers streaming only
 * (Access::Streaming) yet the command requested random access. A handler that
 * already provides a random-access model needs no wrapping; a streaming handler
 * that genuinely cannot be wrapped (the inter-command pipe) is excluded here so
 * the clean streaming-only error is preserved for it. */
bool requires_ram_wrapper(const Formats::Base *handler, const AccessRequest access) {
  return access == AccessRequest::RandomAccess && handler->capabilities.access == Formats::Access::Streaming &&
         handler->can_ram_wrap();
}

} // namespace

template <class ValueType>
Tractogram<ValueType> Tractogram<ValueType>::open(const std::filesystem::path &path,
                                                  Properties &properties,
                                                  AccessRequest access,
                                                  const OptionalHeader &grid) {
  const Formats::Base *handler = select_handler(path);
  if (!handler->can_read())
    throw Exception("tractography format \"" + handler->description + "\" does not support reading (file \"" +
                    path.string() + "\")");

  // When random access is requested against a streaming-only format, transparently
  //   wrap the chosen handler in the in-RAM random-access wrapper (Stage 15, step 2).
  if (requires_ram_wrapper(handler, access)) {
    auto store = std::make_shared<RAMStore<ValueType>>();
    auto wrapper = std::make_shared<Formats::RAMWrapper<ValueType>>(handler, store);
    Tractogram tractogram(wrapper.get());
    tractogram.store = store;
    tractogram.ram_wrapper = wrapper;
    tractogram.reader = wrapper->template read<ValueType>(path, properties, *tractogram.registry, grid);
    tractogram.reader->read_grouping(tractogram.grouping_);
    return tractogram;
  }

  if (access == AccessRequest::RandomAccess)
    Tractogram(handler).require_random_access("indexed access to the data");

  Tractogram tractogram(handler);
  tractogram.reader = handler->template read<ValueType>(path, properties, *tractogram.registry, grid);
  // Reconcile any dataset-level grouping at the Tractogram/Grouping boundary
  //   (§2.7): the format fills the Grouping once, here, not per item.
  tractogram.reader->read_grouping(tractogram.grouping_);
  return tractogram;
}

template <class ValueType>
Tractogram<ValueType> Tractogram<ValueType>::create(const std::filesystem::path &path,
                                                    const Properties &properties,
                                                    const FieldRegistry &registry,
                                                    AccessRequest access,
                                                    const OptionalHeader &grid,
                                                    const Formats::WriteOptions &options) {
  const Formats::Base *handler = select_handler(path);
  if (!handler->can_write())
    throw Exception("tractography format \"" + handler->description + "\" does not support writing (file \"" +
                    path.string() + "\")");

  // When random access is requested against a streaming-only format, transparently
  //   wrap the chosen handler in the in-RAM random-access wrapper (Stage 15, step 2).
  if (requires_ram_wrapper(handler, access)) {
    auto store = std::make_shared<RAMStore<ValueType>>();
    auto wrapper = std::make_shared<Formats::RAMWrapper<ValueType>>(handler, store);
    Tractogram tractogram(wrapper.get());
    *tractogram.registry = registry;
    tractogram.store = store;
    tractogram.ram_wrapper = wrapper;
    tractogram.writer = wrapper->template create<ValueType>(path, properties, *tractogram.registry, grid, options);
    return tractogram;
  }

  if (access == AccessRequest::RandomAccess)
    Tractogram(handler).require_random_access("indexed access to the data");

  Tractogram tractogram(handler);
  *tractogram.registry = registry;
  tractogram.writer = handler->template create<ValueType>(path, properties, *tractogram.registry, grid, options);
  return tractogram;
}

template class Tractogram<float>;
template class Tractogram<double>;

} // namespace MR::DWI::Tractography
