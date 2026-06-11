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

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

#include "dwi/tractography/field_registry.h"
#include "dwi/tractography/formats/base.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/tractogram_item.h"
#include "exception.h"

namespace MR::DWI::Tractography {

//! \brief Standardised accessor for streamlines data, analogous to MR::Image<>.
/*! Tractogram is to tractography what MR::Image<> is to image intensity data:
 * the public handle through which a command reads or writes streamlines without
 * caring which on-disk format backs them. It selects the appropriate format
 * handler from the handler list (formats/list.cpp) by file extension, then owns
 * that handler's streaming reader or writer backend, the dataset Properties, and
 * the sidecar field registry (§2.5).
 *
 * A Tractogram is constructed through the static open() / create() factories
 * (mirroring Image::open() / Image::create()); the resulting object is then
 * called as a functor to stream items, fitting the
 * Reader→queue→worker→queue→Writer paradigm (§1.4):
 *   - a read Tractogram fills a TractogramItem per operator() call, returning
 *     false at end-of-data;
 *   - a write Tractogram appends a TractogramItem per operator() call.
 *
 * The read/write entry points take/return the composite TractogramItem (§2.1)
 * rather than a bare Streamline, even though Stage 1 populates only the
 * streamline part, so the sidecar machinery of later stages slots in without
 * re-plumbing the commands.
 *
 * Streamlines are processed in a floating-point precision (float or double)
 * fixed by the ValueType template parameter. */
template <class ValueType = float> class Tractogram {
public:
  using value_type = ValueType;
  using item_type = TractogramItem<ValueType>;

  //! \brief open an existing tractography dataset for streaming reads.
  /*! Selects the handler for \a path by extension, verifies it supports
   * reading, and populates \a properties from the dataset header. Throws a
   * user-interpretable Exception if no handler recognises the extension or the
   * recognised handler cannot read. */
  static Tractogram
  open(const std::filesystem::path &path, Properties &properties, const OptionalHeader &grid = std::nullopt);

  //! \brief create a new tractography dataset for streaming writes.
  /*! Selects the handler for \a path by extension and verifies it supports
   * writing. The output sidecar field set is declared up-front via \a registry
   * (§2.5/§2.7): a sidecar-aware handler serialises it (the pipe; later TRX),
   * while a vertices-only handler ignores it. \a registry defaults to an empty
   * registry, so existing vertices-only callers are unaffected. Throws a
   * user-interpretable Exception if no handler recognises the extension or the
   * recognised handler cannot write. */
  static Tractogram create(const std::filesystem::path &path,
                           const Properties &properties,
                           const FieldRegistry &registry = FieldRegistry(),
                           const OptionalHeader &grid = std::nullopt);

  //! \brief read the next item from the dataset (read mode only).
  /*! \returns true and fills \a item while data remain; false once the dataset
   * is exhausted. This is the explicit, unambiguous read entry point; the
   * operator() overload below forwards to it for use as a queue source. */
  bool read(item_type &item) {
    assert(reader != nullptr);
    return (*reader)(item);
  }

  //! \brief append an item to the dataset (write mode only).
  /*! Explicit, unambiguous write entry point; the operator() overload below
   * forwards to it for use as a queue sink. */
  bool write(const item_type &item) {
    assert(writer != nullptr);
    return (*writer)(item);
  }

  //! \brief queue-source convenience: read the next item (read mode only).
  /*! \note A read Tractogram is the source of a thread queue, so its functor
   * call fills a (non-const) item. Because a single Tractogram is either a
   * reader or a writer, the const-qualification of the argument selects the
   * intended role unambiguously: a non-const lvalue reads, a const lvalue
   * writes. Prefer the named read() / write() methods where clarity matters. */
  bool operator()(item_type &item) { return read(item); }

  //! \brief queue-sink convenience: append an item (write mode only).
  bool operator()(const item_type &item) { return write(item); }

  //! \brief the capabilities advertised by the selected handler (§2.6).
  const Formats::Capabilities &capabilities() const { return handler->capabilities; }
  //! \brief a human-readable description of the selected format.
  std::string format() const { return handler->description; }

  //! \brief whether the selected handler permits random access to the data.
  /*! True iff the handler advertises one of the random-access models (§2.6);
   * false for a streaming-only handler such as the inter-command pipe ("-"),
   * whose backing byte stream is sequential by nature. */
  bool is_random_access() const { return handler->capabilities.access != Formats::Access::Streaming; }

  //! \brief assert that the selected handler supports random access, or throw.
  /*! A command that requires random access to the tractogram (e.g. multiple
   * passes, indexed retrieval, in-place mutation) calls this before doing so.
   * A streaming-only handler — the inter-command pipe being the canonical case —
   * cannot service such a request: a pipe is a one-pass sequential stream that
   * cannot be rewound, and silently buffering the whole (unbounded) tractogram
   * to fake random access is rejected by design (§2.2 of the pipe design note).
   * The error names the operation \a context so the message is
   * user-interpretable. The random-access wrapper of a later stage may instead
   * be selected automatically for formats that can be wrapped; the pipe cannot,
   * so it always raises here. */
  void require_random_access(std::string_view context = "this operation") const {
    if (!is_random_access())
      throw Exception("tractography format \"" + handler->description + "\" provides sequential streaming access only" +
                      " and cannot service " + std::string(context) + " (which requires random access to the data)");
  }

  //! \brief the sidecar field registry for this dataset (§2.5).
  const FieldRegistry &fields() const { return registry; }
  FieldRegistry &fields() { return registry; }

  bool is_read() const { return reader != nullptr; }
  bool is_write() const { return writer != nullptr; }

private:
  explicit Tractogram(const Formats::Base *handler) : handler(handler) {}

  //! the selected format handler (a non-owning pointer into the static handler list)
  const Formats::Base *handler;
  //! the streaming read backend (non-null in read mode)
  std::unique_ptr<ReaderInterface<ValueType>> reader;
  //! the streaming write backend (non-null in write mode)
  std::unique_ptr<WriterInterface<ValueType>> writer;
  //! the sidecar field registry (empty in Stage 1)
  FieldRegistry registry;
};

} // namespace MR::DWI::Tractography
