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
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

#include "dwi/tractography/field_registry.h"
#include "dwi/tractography/grouping.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/streamline.h"
#include "dwi/tractography/tractogram_item.h"

namespace MR {
class Header;
}

namespace MR::DWI::Tractography {

//! \brief An optional grid reference supplied to a format handler.
/*! Some tractography formats encode vertex positions relative to an image grid
 * rather than in MRtrix scanner-space (e.g. the DSI Studio ".tt" format stores
 * 1/32-voxel integer coordinates; ".trk" and TRX likewise carry grid metadata).
 * Handlers for such formats require the voxel↔scanner transform of an
 * MR::Header to convert between the on-disk and the in-memory coordinate
 * systems. The reference is optional: the in-house ".tck"/".vtk"/".vtx"
 * handlers store scanner-space coordinates directly and require no grid, so
 * they leave it unset (std::nullopt). The reference wrapper keeps the Header
 * non-owned by the handler; the caller retains ownership for the I/O lifetime. */
using OptionalHeader = std::optional<std::reference_wrapper<const Header>>;

//! \brief The contract for streaming reads of tractography data.
/*! A reader yields one item per invocation of operator(), returning false once
 * the dataset is exhausted. This type-erased interface lets a command consume
 * any supported format through a single handle, irrespective of the concrete
 * handler backing it (cf. ImageIO::Base in the image subsystem).
 *
 * Two read overloads coexist (§2.1): the Streamline overload (vertices + weight
 * + index) is the primitive every handler implements; the TractogramItem
 * overload additionally fills the dps/dpv sidecar payloads. The latter has a
 * default implementation that forwards to the former and leaves the sidecar
 * vectors empty, so a vertices-only handler (.tck/.vtk/.vtx/.tt) needs no
 * change while a sidecar-carrying handler (the pipe; later TRX) overrides it. */
template <class ValueType> class ReaderInterface {
public:
  virtual bool operator()(Streamline<ValueType> &) = 0;
  //! \brief fill the composite item; defaults to the vertices-only read.
  virtual bool operator()(TractogramItem<ValueType> &item) {
    item.dps.clear();
    item.dpv.clear();
    return (*this)(item.streamline);
  }
  //! \brief populate the dataset-level streamline grouping, if the format carries one (§2.3/§2.7).
  /*! Grouping (TRX groups/dpg; connectome assignments) is tractogram-level, not
   * per-item: it is reconciled once at the Tractogram/Grouping boundary rather
   * than threaded through the per-streamline queue. A handler that carries no
   * grouping (the default) leaves \a grouping untouched; the TRX reader fills it
   * from the on-disk groups/ and dpg/ members. */
  virtual void read_grouping(Grouping &) {}
  virtual ~ReaderInterface() {}
};

//! \brief The contract for streaming writes of tractography data.
/*! A writer appends one item per invocation of operator(); see ReaderInterface
 * for the rationale behind the type-erased interface and the two overloads. The
 * TractogramItem overload defaults to writing only the streamline vertices, so
 * a vertices-only handler is unaffected; a sidecar-carrying handler overrides it
 * to serialise the dps/dpv payloads. */
template <class ValueType> class WriterInterface {
public:
  virtual bool operator()(const Streamline<ValueType> &) = 0;
  //! \brief append the composite item; defaults to the vertices-only write.
  virtual bool operator()(const TractogramItem<ValueType> &item) { return (*this)(item.streamline); }
  //! \brief register the dataset-level streamline grouping to be serialised (§2.3/§2.7).
  /*! Grouping is tractogram-level (TRX groups/dpg), so the caller supplies the
   * whole Grouping once, before the writer is finalised, rather than per item.
   * A vertices-only handler (the default) ignores it; the TRX writer stores it
   * and emits the groups/ and dpg/ members at finalisation. */
  virtual void write_grouping(const Grouping &) {}
  virtual ~WriterInterface() {}
};

namespace Formats {

//! \brief I/O directions a tractography format handler can service.
/*! Broadcast by every handler so that the framework can reject an
 * unsupported operation (e.g. attempting to create a read-only format)
 * with a clean error rather than a backend failure. */
enum class IO {
  ReadOnly,  //!< the format can only be read
  WriteOnly, //!< the format can only be written
  ReadWrite  //!< the format supports both reading and writing
};

//! \brief The access model offered by a tractography format handler.
/*! Mirrors the streaming-vs-random-access distinction of the image
 * subsystem; the framework consults this to decide whether a command's
 * access pattern is directly serviceable or requires the random-access
 * wrapper (a later stage). */
enum class Access {
  //! sequential streaming of ordered data only (e.g. the .tck plaintext stream)
  Streaming,
  //! random access permitted iff the streamline count and the per-streamline
  //!   vertex counts are not altered
  RandomAccessFixed,
  //! complete random access, including streamline deletion and resizing
  RandomAccessFull
};

//! \brief Whether a format can grow an existing dataset in place.
enum class Augment {
  //! an existing dataset may be augmented with new data in place
  Append,
  //! any modification requires the whole dataset to be rewritten
  Rewrite
};

//! \brief The capabilities a tractography format handler broadcasts.
/*! Encapsulates the three orthogonal axes of §2.6: I/O direction, access
 * model, and in-place augmentation, so that the framework can match a
 * command's requirements against a handler without probing the backend. */
struct Capabilities {
  IO io;
  Access access;
  Augment augment;
};

//! \brief The interface for classes that support the various tractography formats.
/*! All tractography formats supported by %MRtrix are handled by a class
 * derived from Formats::Base; this is the direct analogue of
 * MR::Formats::Base in the image subsystem. An instance of each derived
 * class is added to the handler list in formats/list.cpp.
 *
 * A handler is responsible for recognising a path/extension, advertising its
 * capabilities, and manufacturing the reader/writer backend that performs the
 * byte-level I/O. The factory methods return the type-erased
 * ReaderInterface / WriterInterface so that a command can stream any format
 * through one handle, preserving the filesystem-vs-compute separation of the
 * image subsystem.
 *
 * Streamline data are processed in a floating-point precision selected by the
 * command (float or double). Because a virtual function cannot itself be a
 * template, the precision is dispatched through the templated read() / create()
 * helpers to the corresponding float/double virtual. */
class Base {
public:
  Base(std::string_view description, const Capabilities &capabilities)
      : description(description), capabilities(capabilities) {}
  virtual ~Base() = default;

  //! a short human-readable description of the tractography format
  const std::string description;

  //! the capabilities advertised by this format handler (§2.6)
  const Capabilities capabilities;

  //! \brief test whether this handler is responsible for the given path.
  /*! Selection loops the handler list (formats/list.cpp); the first handler
   * that recognises \a path (typically by file extension) wins. This is a
   * pure path test and does not open the file. */
  virtual bool handles(const std::filesystem::path &path) const = 0;

  bool can_read() const { return capabilities.io != IO::WriteOnly; }
  bool can_write() const { return capabilities.io != IO::ReadOnly; }

  //! \brief whether a streaming-only handler may be wrapped for random access (Stage 15).
  /*! When a command requires random access against a handler that advertises
   * Access::Streaming, the framework wraps the handler in the in-RAM
   * random-access wrapper (Formats::RAMWrapper) — load-once into RAM, write-once
   * out — rather than raising the streaming-only error. This is possible for any
   * file-backed format (the whole dataset can be loaded once and flushed once),
   * but NOT for the inter-command pipe: a pipe is a one-pass sequential stdin/
   * stdout stream that cannot be re-opened to flush a mutated dataset, and
   * silently buffering an unbounded stream is rejected by design (§2.6 /
   * pipe_design.md §2.2). The pipe therefore overrides this to false so the clean
   * streaming-only error is preserved for it. Defaults to true. */
  virtual bool can_ram_wrap() const { return true; }

  //! \brief open \a path for streaming reads in the requested precision.
  /*! Dispatches to the float/double factory virtual. \a properties is
   * populated from the dataset header. \a registry is populated with the sidecar
   * field descriptors the format carries (§2.5): a vertices-only format leaves
   * it empty, while a sidecar-carrying format (the pipe; later TRX) registers a
   * descriptor per field so the per-item dps/dpv payloads can be addressed by
   * ordinal. \a grid is the optional image-grid reference (§ OptionalHeader):
   * handlers for grid-relative formats use its voxel↔scanner transform;
   * scanner-space handlers ignore it. */
  template <class ValueType>
  std::unique_ptr<ReaderInterface<ValueType>> read(const std::filesystem::path &path,
                                                   Properties &properties,
                                                   FieldRegistry &registry,
                                                   const OptionalHeader &grid = std::nullopt) const {
    static_assert(std::is_same<ValueType, float>::value || std::is_same<ValueType, double>::value,
                  "tractography I/O is supported in float or double precision only");
    if constexpr (std::is_same<ValueType, float>::value)
      return read_float(path, properties, registry, grid);
    else
      return read_double(path, properties, registry, grid);
  }

  //! \brief create \a path for streaming writes in the requested precision.
  /*! \a registry declares the sidecar field set the output is to carry (§2.5/
   * §2.7): a sidecar-aware format (the pipe; later TRX) serialises it so a
   * receiver can reconstruct the field ordinals, whereas a vertices-only format
   * ignores it. \a grid is the optional image-grid reference (§ OptionalHeader);
   * see read(). */
  template <class ValueType>
  std::unique_ptr<WriterInterface<ValueType>> create(const std::filesystem::path &path,
                                                     const Properties &properties,
                                                     const FieldRegistry &registry,
                                                     const OptionalHeader &grid = std::nullopt) const {
    static_assert(std::is_same<ValueType, float>::value || std::is_same<ValueType, double>::value,
                  "tractography I/O is supported in float or double precision only");
    if constexpr (std::is_same<ValueType, float>::value)
      return create_float(path, properties, registry, grid);
    else
      return create_double(path, properties, registry, grid);
  }

protected:
  //! \brief manufacture a single-precision streaming reader for \a path.
  virtual std::unique_ptr<ReaderInterface<float>> read_float(const std::filesystem::path &path,
                                                             Properties &properties,
                                                             FieldRegistry &registry,
                                                             const OptionalHeader &grid) const = 0;
  //! \brief manufacture a double-precision streaming reader for \a path.
  virtual std::unique_ptr<ReaderInterface<double>> read_double(const std::filesystem::path &path,
                                                               Properties &properties,
                                                               FieldRegistry &registry,
                                                               const OptionalHeader &grid) const = 0;
  //! \brief manufacture a single-precision streaming writer for \a path.
  virtual std::unique_ptr<WriterInterface<float>> create_float(const std::filesystem::path &path,
                                                               const Properties &properties,
                                                               const FieldRegistry &registry,
                                                               const OptionalHeader &grid) const = 0;
  //! \brief manufacture a double-precision streaming writer for \a path.
  virtual std::unique_ptr<WriterInterface<double>> create_double(const std::filesystem::path &path,
                                                                 const Properties &properties,
                                                                 const FieldRegistry &registry,
                                                                 const OptionalHeader &grid) const = 0;
};

} // namespace Formats

} // namespace MR::DWI::Tractography
