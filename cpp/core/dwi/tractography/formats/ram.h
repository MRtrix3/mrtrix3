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

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "dwi/tractography/field_registry.h"
#include "dwi/tractography/formats/base.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/tractogram_item.h"

namespace MR::DWI::Tractography {

//! \brief The in-RAM store backing the random-access wrapper (Stage 15).
/*! Holds the whole tractogram — every TractogramItem (vertices + dps/dpv
 * sidecar) plus the field registry — resident in heap memory so that any
 * streamline is addressable at any time, in any order. This is the
 * tractography analogue of the image RAM/scratch backend (ImageIO::RAM): the
 * underlying filesystem format handler touches the disk only once (a single
 * sequential pass to populate the store on read, or to flush it on write), and
 * all access in between is pure RAM.
 *
 * The store is shared (std::shared_ptr) between the owning Tractogram and the
 * wrapper's reader/writer backend, so that the random-access accessors exposed
 * on the Tractogram operate directly on the same resident items the backend
 * loads/flushes. */
template <class ValueType> class RAMStore {
public:
  using item_type = TractogramItem<ValueType>;

  //! the whole tractogram, resident in RAM and randomly addressable
  std::vector<item_type> items;
  //! the sidecar field registry carried by the resident items (§2.5)
  FieldRegistry registry;
};

namespace Formats {

//! \brief Random-access wrapper around a streaming-only format handler (Stage 15).
/*! This handler INHERITS from the format base class, yet does not service a file
 * extension of its own: instead it stores a (non-owning) pointer to the actual
 * underlying format handler — the streaming handler selected from the handler
 * list by extension — by which the streamlines and sidecar data are really
 * loaded from / saved to the filesystem.
 *
 * The wrapper holds the WHOLE tractogram and sidecar data in RAM (the shared
 * RAMStore), providing random access to any streamline at any time. The
 * underlying handler is used ONLY on construction (a single load-all pass) and
 * on destruction (a single write-all pass); all access in between is pure RAM.
 * It therefore advertises Access::RandomAccessFull (everything is resident, so
 * deletion/resize is possible) while delegating the real filesystem I/O to the
 * inner streaming handler.
 *
 * This is the direct analogue of the image RAM/scratch backend (§1.2):
 * cpp/core/formats/ram.cpp + cpp/core/image_io/ram.cpp.
 *
 * Unlike the format handlers in the static handler list, a RAMWrapper is
 * constructed on demand by the Tractogram framework: when a command requests
 * random access (§2.6) against a streaming-only inner handler, the framework
 * wraps that handler here rather than raising the streaming-only error. The
 * wrapper's read()/create() factories build RAM reader/writer backends that
 * share the supplied RAMStore. */
template <class ValueType> class RAMWrapper : public Base {
public:
  //! \brief wrap \a inner, populating/flushing \a store on construction/destruction.
  /*! \a inner is the streaming-only handler that performs the real filesystem
   * I/O; it is borrowed, not owned (it is a static handler-list instance). \a
   * store is the shared in-RAM store the reader/writer backends operate on and
   * the owning Tractogram exposes for random access. */
  RAMWrapper(const Base *inner, std::shared_ptr<RAMStore<ValueType>> store)
      : Base(std::string("RAM-wrapped ") + std::string(inner->description),
             {inner->capabilities.io, Access::RandomAccessFull, Augment::Rewrite}),
        inner(inner),
        store(std::move(store)) {}

  //! \brief the wrapper recognises whatever path the inner handler recognises.
  bool handles(const std::filesystem::path &path) const override { return inner->handles(path); }

protected:
  std::unique_ptr<ReaderInterface<float>> read_float(const std::filesystem::path &path,
                                                     Properties &properties,
                                                     FieldRegistry &registry,
                                                     const OptionalHeader &grid) const override;
  std::unique_ptr<ReaderInterface<double>> read_double(const std::filesystem::path &path,
                                                       Properties &properties,
                                                       FieldRegistry &registry,
                                                       const OptionalHeader &grid) const override;
  std::unique_ptr<WriterInterface<float>> create_float(const std::filesystem::path &path,
                                                       const Properties &properties,
                                                       const FieldRegistry &registry,
                                                       const OptionalHeader &grid) const override;
  std::unique_ptr<WriterInterface<double>> create_double(const std::filesystem::path &path,
                                                         const Properties &properties,
                                                         const FieldRegistry &registry,
                                                         const OptionalHeader &grid) const override;

private:
  //! the borrowed streaming handler that performs the real filesystem I/O
  const Base *inner;
  //! the shared in-RAM store populated on read / flushed on write
  std::shared_ptr<RAMStore<ValueType>> store;
};

} // namespace Formats

//! \brief Streaming-source view over a populated RAMStore (Stage 15).
/*! Presented to a command as a ReaderInterface so that the
 * Reader→queue→worker→queue→Writer paradigm (§1.4) still functions even when the
 * data are resident in RAM: each operator() hands back the next resident item in
 * order. The store itself is loaded once, up front, by the wrapper from the
 * inner streaming handler; this view merely walks it. Random (out-of-order)
 * access is provided by the owning Tractogram directly against the shared store,
 * not through this sequential view. */
template <class ValueType> class RAMReader : public ReaderInterface<ValueType> {
public:
  explicit RAMReader(std::shared_ptr<RAMStore<ValueType>> store) : store(std::move(store)), cursor(0) {}

  bool operator()(Streamline<ValueType> &tck) override {
    if (cursor == store->items.size())
      return false;
    tck = store->items[cursor++].streamline;
    return true;
  }

  bool operator()(TractogramItem<ValueType> &item) override {
    if (cursor == store->items.size())
      return false;
    item = store->items[cursor++];
    return true;
  }

private:
  std::shared_ptr<RAMStore<ValueType>> store;
  size_t cursor;
};

//! \brief Streaming-sink view over a RAMStore, flushed once on destruction (Stage 15).
/*! Presented to a command as a WriterInterface: each operator() appends the
 * supplied item to the resident store rather than to the filesystem. The store
 * is committed to disk exactly once — in this object's destructor — by driving
 * the inner streaming handler's writer over every resident item in turn (the
 * "write-once" half of the load-once / write-once contract). This preserves the
 * sidecar (dps/dpv, native dtype, M) intact, since the resident items carry it
 * and the inner writer serialises whatever the field registry declares. */
template <class ValueType> class RAMWriter : public WriterInterface<ValueType> {
public:
  //! \brief collect into \a store; flush via \a inner to \a path on destruction.
  /*! \a properties is borrowed by reference; the caller (the owning Tractogram's
   * client) must keep it alive until the RAMWriter is destroyed, since the
   * single write-all pass that consults it is deferred to destruction. This
   * matches the existing convention that the Properties outlive the Tractogram,
   * and is necessary because Properties is non-copyable (it embeds a
   * non-copyable Seeding::List). */
  RAMWriter(const Formats::Base *inner,
            std::filesystem::path path,
            const Properties &properties,
            std::shared_ptr<RAMStore<ValueType>> store,
            OptionalHeader grid)
      : inner(inner), path(std::move(path)), properties(properties), store(std::move(store)), grid(grid) {}

  ~RAMWriter() override;

  bool operator()(const Streamline<ValueType> &tck) override {
    store->items.emplace_back(tck);
    return true;
  }

  bool operator()(const TractogramItem<ValueType> &item) override {
    store->items.push_back(item);
    return true;
  }

private:
  const Formats::Base *inner;
  std::filesystem::path path;
  const Properties &properties;
  std::shared_ptr<RAMStore<ValueType>> store;
  OptionalHeader grid;
};

} // namespace MR::DWI::Tractography
