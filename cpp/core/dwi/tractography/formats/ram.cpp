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

#include "dwi/tractography/formats/ram.h"

#include <memory>
#include <utility>

namespace MR::DWI::Tractography::Formats {

namespace {

//! \brief Drive the inner streaming handler once to load the whole dataset into RAM.
/*! The "load-once" half of the load-once / write-once contract: the inner
 * streaming reader is consumed sequentially exactly once, here at construction,
 * filling the shared store's resident item vector (vertices + dps/dpv sidecar)
 * and adopting the field registry the inner handler populated. After this the
 * inner handler is discarded and never touches the filesystem again. */
template <class ValueType>
std::unique_ptr<ReaderInterface<ValueType>> load_all(const Base *inner,
                                                     const std::filesystem::path &path,
                                                     Properties &properties,
                                                     FieldRegistry &registry,
                                                     const OptionalHeader &grid,
                                                     const std::shared_ptr<RAMStore<ValueType>> &store) {
  std::unique_ptr<ReaderInterface<ValueType>> reader =
      inner->template read<ValueType>(path, properties, store->registry, grid);
  TractogramItem<ValueType> item;
  while ((*reader)(item))
    store->items.push_back(item);
  // Expose the field set the inner handler discovered to the owning Tractogram.
  registry = store->registry;
  return std::make_unique<RAMReader<ValueType>>(store);
}

} // namespace

template <class ValueType>
std::unique_ptr<ReaderInterface<float>> RAMWrapper<ValueType>::read_float(const std::filesystem::path &path,
                                                                          Properties &properties,
                                                                          FieldRegistry &registry,
                                                                          const OptionalHeader &grid) const {
  if constexpr (std::is_same<ValueType, float>::value)
    return load_all<float>(inner, path, properties, registry, grid, store);
  else
    throw Exception("RAM wrapper precision mismatch (float requested from a double-precision wrapper)");
}

template <class ValueType>
std::unique_ptr<ReaderInterface<double>> RAMWrapper<ValueType>::read_double(const std::filesystem::path &path,
                                                                            Properties &properties,
                                                                            FieldRegistry &registry,
                                                                            const OptionalHeader &grid) const {
  if constexpr (std::is_same<ValueType, double>::value)
    return load_all<double>(inner, path, properties, registry, grid, store);
  else
    throw Exception("RAM wrapper precision mismatch (double requested from a single-precision wrapper)");
}

template <class ValueType>
std::unique_ptr<WriterInterface<float>> RAMWrapper<ValueType>::create_float(const std::filesystem::path &path,
                                                                            const Properties &properties,
                                                                            const FieldRegistry &registry,
                                                                            const OptionalHeader &grid) const {
  if constexpr (std::is_same<ValueType, float>::value) {
    store->registry = registry;
    return std::make_unique<RAMWriter<float>>(inner, path, properties, store, grid);
  } else {
    throw Exception("RAM wrapper precision mismatch (float requested from a double-precision wrapper)");
  }
}

template <class ValueType>
std::unique_ptr<WriterInterface<double>> RAMWrapper<ValueType>::create_double(const std::filesystem::path &path,
                                                                              const Properties &properties,
                                                                              const FieldRegistry &registry,
                                                                              const OptionalHeader &grid) const {
  if constexpr (std::is_same<ValueType, double>::value) {
    store->registry = registry;
    return std::make_unique<RAMWriter<double>>(inner, path, properties, store, grid);
  } else {
    throw Exception("RAM wrapper precision mismatch (double requested from a single-precision wrapper)");
  }
}

template class RAMWrapper<float>;
template class RAMWrapper<double>;

} // namespace MR::DWI::Tractography::Formats

namespace MR::DWI::Tractography {

template <class ValueType> RAMWriter<ValueType>::~RAMWriter() {
  // The "write-once" half of the load-once / write-once contract: drive the
  //   inner streaming writer over every resident item exactly once, here at
  //   destruction. The sidecar (dps/dpv, native dtype, M) rides along in each
  //   resident item and is serialised by the inner writer per the field registry.
  try {
    std::unique_ptr<WriterInterface<ValueType>> writer =
        inner->template create<ValueType>(path, properties, store->registry, grid);
    for (const auto &item : store->items)
      (*writer)(item);
  } catch (Exception &e) {
    e.display();
  }
}

template class RAMReader<float>;
template class RAMReader<double>;
template class RAMWriter<float>;
template class RAMWriter<double>;

} // namespace MR::DWI::Tractography
