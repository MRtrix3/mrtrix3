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

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "dwi/tractography/field_registry.h"
#include "dwi/tractography/formats/base.h"
#include "dwi/tractography/formats/ram.h"
#include "dwi/tractography/grouping.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/sidecar.h"
#include "dwi/tractography/tractogram_item.h"
#include "exception.h"

namespace MR::DWI::Tractography {

//! \brief The access pattern a command requires of a Tractogram (Stage 15).
/*! A command declares, when it opens/creates a Tractogram, whether it will
 * stream the data sequentially (the default, fitting the
 * Reader→queue→worker→queue→Writer paradigm) or needs random access to any
 * streamline at any time (e.g. multiple passes, indexed retrieval, in-place
 * mutation). When RandomAccess is requested against a format whose native
 * handler offers streaming only, the framework transparently wraps that handler
 * in the in-RAM random-access wrapper (Formats::RAMWrapper) rather than raising
 * the streaming-only error. */
enum class AccessRequest {
  Streaming,   //!< sequential streaming suffices (the default)
  RandomAccess //!< random access to any streamline is required
};

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
  static Tractogram open(const std::filesystem::path &path,
                         Properties &properties,
                         AccessRequest access = AccessRequest::Streaming,
                         const OptionalHeader &grid = std::nullopt);

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
                           AccessRequest access = AccessRequest::Streaming,
                           const OptionalHeader &grid = std::nullopt,
                           const Formats::WriteOptions &options = {});

  //! \brief read the next item from the dataset (read mode only).
  /*! \returns true and fills \a item while data remain; false once the dataset
   * is exhausted. This is the explicit, unambiguous read entry point; the
   * operator() overload below forwards to it for use as a queue source. */
  bool read(item_type &item) {
    assert(reader != nullptr);
    if (!(*reader)(item)) {
      finalise_weight_input();
      return false;
    }
    // Inject any registered standalone input-sidecar data into the per-streamline
    //   payload prior to processing (§2.5; Stage 11, step 5).
    for (auto &loader : input_sidecars)
      (*loader)(item);
    // Route the explicitly-designated streamline weight into the privileged
    //   Streamline::weight (its single source of truth). An external weight file
    //   shorter than the tractogram truncates the stream here, mirroring the
    //   legacy ".tck" reader; an internal field is read from the dps payload.
    if (weight_in_external) {
      if (!(*weight_in_external)(item.streamline)) {
        finalise_weight_input();
        return false;
      }
    } else if (weight_in_internal_ordinal.has_value()) {
      item.streamline.weight = dps_scalar_to_float(item.dps[*weight_in_internal_ordinal]);
    }
    ++read_count_;
    return true;
  }

  //! \brief read the next streamline only (vertices + weight + index), ignoring sidecars.
  /*! The lightweight counterpart to read(item_type&): fills a bare Streamline and
   * skips the per-streamline (dps) and per-vertex (dpv) sidecar payloads entirely.
   * Intended for the call sites (Stages 2 and 4) that only ever consumed vertices,
   * the streamline weight, and the ordinal index — the contract of the legacy
   * TCKReader<>::operator()(Streamline&).
   *
   * \par Weight handling
   * The privileged streamline weight is honoured on this path; non-weight sidecar
   * data are not. Streamline::weight defaults to 1.0 and is overwritten only when an
   * explicit weight source was registered (register_weight_input_external /
   * register_weight_input_internal, driven by "-tck_weights_in"):
   *   - an EXTERNAL weight file is applied by ordinal index immediately after the
   *     bare handler read, on the fast path (no TractogramItem allocated);
   *   - an INTERNAL weight field must be extracted from the dps payload, so this
   *     path reads into a reused member item, routes the weight into
   *     item.streamline.weight, then moves the streamline out (dps/dpv discarded).
   *
   * The common no-source, no-sidecar case forwards straight to the handler with
   * zero overhead and allocates no TractogramItem. */
  bool read(Streamline<ValueType> &streamline) {
    assert(reader != nullptr);
    // Fast path: no per-streamline payload needed — no input sidecars and no
    //   internal weight field to extract from the dps payload. An external weight
    //   file (if any) is applied by ordinal index after the bare read.
    if (input_sidecars.empty() && !weight_in_internal_ordinal.has_value()) {
      if (!(*reader)(streamline)) {
        finalise_weight_input();
        return false;
      }
      if (weight_in_external) {
        if (!(*weight_in_external)(streamline)) {
          finalise_weight_input();
          return false;
        }
      }
      ++read_count_;
      return true;
    }
    // Sidecar / internal-weight path: read into the reused member item so the
    //   weight is routed (and dps/dpv discarded), then move the streamline out.
    if (!read(streamline_read_item))
      return false;
    streamline = std::move(streamline_read_item.streamline);
    return true;
  }

  //! \brief append an item to the dataset (write mode only).
  /*! Explicit, unambiguous write entry point; the operator() overload below
   * forwards to it for use as a queue sink. */
  bool write(const item_type &item) {
    assert(writer != nullptr);
    // Extract any registered standalone output-sidecar data from the processed
    //   per-streamline payload (§2.7; Stage 11, step 6).
    for (auto &exporter : output_sidecars)
      (*exporter)(item);
    // Route the privileged Streamline::weight to its explicit destination: an
    //   external scalar file, and/or (for an embedding format) re-emitted into the
    //   designated output dps field from Streamline::weight so it reflects any
    //   modification the command made.
    if (weight_out_external)
      (*weight_out_external)(item.streamline);
    ++count_;
    ++total_count_;
    if (weight_out_embed_ordinal.has_value()) {
      weight_embed_item = item;
      if (weight_embed_item.dps.size() <= *weight_out_embed_ordinal)
        weight_embed_item.dps.resize(*weight_out_embed_ordinal + 1);
      weight_embed_item.dps[*weight_out_embed_ordinal] = make_dps_scalar(item.streamline.weight);
      return (*writer)(weight_embed_item);
    }
    return (*writer)(item);
  }

  //! \brief record a streamline that was seen but not exported (write mode only).
  /*! Forwards to the handler's note_unexported(): advances the count of
   * streamlines seen without writing one, so a format that records both the
   * exported count and the total seen (e.g. ".tck") reflects the selection ratio
   * when a command (tckgen, tckedit) discards some of the streamlines it
   * processes. A format that does not track the distinction no-ops. */
  void note_unexported() {
    assert(writer != nullptr);
    writer->note_unexported();
    ++total_count_;
  }

  //! \brief the number of streamlines exported to the dataset so far (write mode).
  uint64_t count() const { return count_; }
  //! \brief the number of streamlines seen so far, including those not exported (write mode).
  /*! Equals count() plus the number of note_unexported() calls — the same
   * exported-vs-seen distinction the ".tck" header records, but maintained here so
   * it is available for any output format. */
  uint64_t total_count() const { return total_count_; }

  //! \brief queue-source convenience: read the next item (read mode only).
  /*! \note A read Tractogram is the source of a thread queue, so its functor
   * call fills a (non-const) item. Because a single Tractogram is either a
   * reader or a writer, the const-qualification of the argument selects the
   * intended role unambiguously: a non-const lvalue reads, a const lvalue
   * writes. Prefer the named read() / write() methods where clarity matters. */
  bool operator()(item_type &item) { return read(item); }

  //! \brief queue-source convenience: read the next streamline only (read mode only).
  /*! The lightweight Streamline counterpart to operator()(item_type&), forwarding to
   * read(Streamline&). A non-const Streamline& lvalue binds only to this overload —
   * not to operator()(item_type&), which would require a user-defined conversion to a
   * non-const reference — so the read source is unambiguous. */
  bool operator()(Streamline<ValueType> &streamline) { return read(streamline); }

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

  //! \brief whether this Tractogram is backed by the in-RAM random-access store.
  /*! True iff the dataset was opened/created with AccessRequest::RandomAccess and
   * the framework selected the RAM wrapper (Stage 15). When true, the indexed
   * accessors below operate directly on the resident items. */
  bool has_ram_store() const { return store != nullptr; }

  //! \brief the number of streamlines resident in the RAM store.
  /*! \pre has_ram_store(); the dataset must have been opened/created for random
   * access. */
  size_t size() const {
    require_ram_store("the streamline count");
    return store->items.size();
  }

  //! \brief random read of the streamline at ordinal \a n from the RAM store.
  /*! Any streamline is addressable at any time, in any order (the defining
   * property of the random-access wrapper). \pre has_ram_store(); n < size(). */
  const item_type &operator[](const size_t n) const {
    require_ram_store("indexed streamline retrieval");
    return store->items[n];
  }
  item_type &operator[](const size_t n) {
    require_ram_store("indexed streamline modification");
    return store->items[n];
  }

  //! \brief random overwrite of the streamline at ordinal \a n in the RAM store.
  void set(const size_t n, const item_type &item) {
    require_ram_store("indexed streamline assignment");
    store->items[n] = item;
  }

  //! \brief append an item to the RAM store (RandomAccessFull only).
  void append(const item_type &item) {
    require_ram_store("streamline append");
    store->items.push_back(item);
  }

  //! \brief erase the streamline at ordinal \a n from the RAM store (RandomAccessFull only).
  void erase(const size_t n) {
    require_ram_store("streamline deletion");
    store->items.erase(store->items.begin() + n);
  }

  //! \brief register a standalone input-sidecar reference for injection (step 5).
  /*! Parses \a arg (§2.4), constructs the appropriate loader (text/.csv/.npy
   * per-streamline, or .tsf per-vertex), and registers the loaded field in this
   * read Tractogram's field registry. The loader is then invoked per read() to
   * inject its value into the streaming item. A qualified "DATASET::NAME"
   * reference is rejected as not-yet-implemented. */
  void register_input_sidecar(std::string_view arg, Properties &properties) {
    assert(reader != nullptr);
    const SidecarReference reference = parse_sidecar_reference(arg);
    // De-duplicate against internally-carried fields (§2.5; Stage 16, step 9):
    //   when the dataset itself already provides a field of this name (e.g. a
    //   TRX dpv/dps member), the external file would load the same field twice.
    //   The internal data takes precedence and the external load is skipped.
    const std::string name = sidecar_field_name(reference);
    for (const FieldDescriptor &descriptor : *registry) {
      if (descriptor.name == name && descriptor.source == FieldSource::Internal) {
        INFO("sidecar field \"" + name + "\" is already carried by the input dataset; " +
             "ignoring the externally-specified copy to avoid duplication");
        return;
      }
    }
    input_sidecars.push_back(make_sidecar_loader<ValueType>(reference, properties, *registry));
  }

  //! \brief register a standalone output-sidecar reference for export (step 6).
  /*! Parses \a arg (§2.4) and constructs the appropriate exporter (a
   * per-streamline numerical text/.npy writer, or a per-vertex .tsf writer). The
   * exporter is invoked per write() to extract its field from the processed
   * item, and commits on destruction (or via finalise_sidecars()). A qualified
   * "DATASET::NAME" reference is rejected as not-yet-implemented. */
  void register_output_sidecar(std::string_view arg, const Properties &properties) {
    assert(writer != nullptr);
    output_sidecars.push_back(
        make_sidecar_exporter<ValueType>(parse_sidecar_reference(arg), properties, is_random_access()));
  }

  //! \brief route an external scalar weight file into Streamline::weight (read mode).
  /*! Implements the bare-path "-tck_weights_in <file>" form. Unlike a generic
   * input sidecar this targets the privileged Streamline::weight, never a dps
   * field. */
  void register_weight_input_external(const std::filesystem::path &path) {
    assert(reader != nullptr);
    weight_in_external = std::make_unique<ExternalWeightLoader<ValueType>>(path);
  }

  //! \brief route an internal dps field into Streamline::weight (read mode).
  /*! Implements the qualified "-tck_weights_in <dataset>::<field>" form, where
   * <dataset> is this input tractogram. \a ordinal is the dps role-local ordinal
   * (in this read Tractogram's registry) of the designated field; on each read its
   * scalar value is moved into Streamline::weight. The field remains present in the
   * dps payload — a consuming command excludes it from output pass-through via the
   * registry (FieldRegistry::copy_excluding) so it is never double-carried. */
  void register_weight_input_internal(const size_t ordinal) {
    assert(reader != nullptr);
    weight_in_internal_ordinal = ordinal;
  }

  //! \brief whether an internal field has been designated as the input weight.
  bool has_internal_weight_input() const { return weight_in_internal_ordinal.has_value(); }

  //! \brief route Streamline::weight to a standalone scalar file (write mode).
  /*! Implements the bare-path "-tck_weights_out <file>" form; works for any output
   * tractography format. \a initial_streamlines pre-sizes the accumulator. */
  void register_weight_output_external(const std::filesystem::path &path, const size_t initial_streamlines) {
    assert(writer != nullptr);
    weight_out_external = std::make_unique<ExternalWeightExporter<ValueType>>(path, initial_streamlines);
  }

  //! \brief re-emit Streamline::weight into the output dps field at \a ordinal (write mode).
  /*! Implements the qualified "-tck_weights_out <dataset>::<field>" embedded form
   * (and the internal-provenance propagation default). \a ordinal is the dps
   * role-local ordinal of the weight field declared in this write Tractogram's
   * registry; on each write Streamline::weight is injected there, so the embedded
   * value reflects any modification the command made. */
  void register_weight_output_embedded(const size_t ordinal) {
    assert(writer != nullptr);
    weight_out_embed_ordinal = ordinal;
  }

  //! \brief flush all registered output sidecars and any external weight file.
  /*! Called explicitly when deterministic finalisation order is required;
   * otherwise each exporter commits in its own destructor. Idempotent. */
  void finalise_sidecars() {
    for (auto &exporter : output_sidecars)
      exporter->finalise();
    if (weight_out_external)
      weight_out_external->finalise();
  }

  //! \brief the sidecar field registry for this dataset (§2.5).
  const FieldRegistry &fields() const { return *registry; }
  FieldRegistry &fields() { return *registry; }

  //! \brief the dataset-level streamline grouping (TRX groups/dpg; §2.3/§2.7).
  /*! Grouping is reconciled at the Tractogram/Grouping boundary, not per item.
   * For a read Tractogram the grouping is populated when the dataset is opened
   * (from the format's on-disk groups/dpg members, if any). For a write
   * Tractogram the caller populates this before finalisation (write_grouping
   * below, or directly), and it is serialised by the handler when the writer is
   * finalised. */
  const Grouping &grouping() const { return grouping_; }
  Grouping &grouping() { return grouping_; }

  //! \brief register the dataset-level grouping to be serialised (write mode).
  /*! Copies \a grouping into this write Tractogram and hands it to the writer
   * backend so the on-disk groups/dpg members are emitted at finalisation. A
   * vertices-only format silently ignores it. */
  void write_grouping(const Grouping &grouping) {
    assert(writer != nullptr);
    grouping_ = grouping;
    writer->write_grouping(grouping_);
  }

  bool is_read() const { return reader != nullptr; }
  bool is_write() const { return writer != nullptr; }

private:
  explicit Tractogram(const Formats::Base *handler) : handler(handler), registry(std::make_shared<FieldRegistry>()) {}

  //! \brief assert this Tractogram is RAM-backed before an indexed access, or throw.
  void require_ram_store(std::string_view context) const {
    if (store == nullptr)
      throw Exception("tractography dataset was not opened for random access" + std::string(" and cannot service ") +
                      std::string(context));
  }

  //! the selected format handler (a non-owning pointer into the static handler
  //!   list, or — when RAM-wrapped — the owned ram_wrapper below)
  const Formats::Base *handler;
  //! the in-RAM random-access store, non-null iff RAM-wrapped (Stage 15)
  std::shared_ptr<RAMStore<ValueType>> store;
  //! the owned RAM wrapper handler, non-null iff RAM-wrapped (Stage 15)
  std::shared_ptr<Formats::Base> ram_wrapper;
  //! the streaming read backend (non-null in read mode)
  std::unique_ptr<ReaderInterface<ValueType>> reader;
  //! the streaming write backend (non-null in write mode)
  std::unique_ptr<WriterInterface<ValueType>> writer;
  //! streamlines exported so far (write mode); see count() / total_count()
  uint64_t count_ = 0;
  //! streamlines seen so far including those not exported (write mode)
  uint64_t total_count_ = 0;
  //! \brief the sidecar field registry (empty in Stage 1), heap-owned for a stable address.
  /*! Allocated on the heap (and shared) so that the registry has an address that
   * survives a move of the owning Tractogram. The format-handler reader/writer
   * backends (owned by this same Tractogram) bind a FieldRegistry& to the pointee
   * via the open()/create() factories; were the registry an in-object member, the
   * move-out of the returned-by-value Tractogram would destroy the moved-from
   * local whose registry the backend referenced, dangling that reference. The
   * pointee does not move when the Tractogram is moved, so the reference stays
   * valid for the Tractogram's lifetime (the backends share that lifetime). */
  std::shared_ptr<FieldRegistry> registry;
  //! \brief the dataset-level streamline grouping (§2.3; Stage 17).
  /*! Populated from the format's on-disk groups/dpg on open() (read), or by the
   * caller before finalisation (write). Empty for formats without grouping. */
  Grouping grouping_;
  //! \brief reused scratch item for read(Streamline&) when input sidecars are registered.
  /*! Only touched on the sidecar-present branch of read(Streamline&), so the
   * common no-sidecar Streamline read allocates no TractogramItem. */
  item_type streamline_read_item;
  //! registered standalone input-sidecar loaders (§2.5; Stage 11, step 5)
  std::vector<std::unique_ptr<SidecarLoader<ValueType>>> input_sidecars;
  //! registered standalone output-sidecar exporters (§2.7; Stage 11, step 6)
  std::vector<std::unique_ptr<SidecarExporter<ValueType>>> output_sidecars;

  //! external scalar weight file -> Streamline::weight ("-tck_weights_in <file>"); null unless registered
  std::unique_ptr<ExternalWeightLoader<ValueType>> weight_in_external;
  //! dps ordinal of an internal field designated as the input weight ("-tck_weights_in <dataset>::<field>")
  std::optional<size_t> weight_in_internal_ordinal;
  //! Streamline::weight -> external scalar file ("-tck_weights_out <file>"); null unless registered
  std::unique_ptr<ExternalWeightExporter<ValueType>> weight_out_external;
  //! output dps ordinal into which Streamline::weight is re-emitted ("-tck_weights_out <dataset>::<field>")
  std::optional<size_t> weight_out_embed_ordinal;
  //! scratch item used to inject the weight on the embedded-output path
  item_type weight_embed_item;
  //! number of streamlines read so far (for the external-weight excess check)
  uint64_t read_count_ = 0;
  //! whether the input-weight end-of-stream finalisation has already run
  bool weight_input_finalised = false;

  //! \brief emit the external-weight excess warning once, at end-of-stream.
  void finalise_weight_input() {
    if (weight_input_finalised)
      return;
    weight_input_finalised = true;
    if (weight_in_external)
      weight_in_external->check_excess(read_count_);
  }
};

} // namespace MR::DWI::Tractography
