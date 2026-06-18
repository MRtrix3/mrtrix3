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

#define IMAGE_H

#include <cerrno>
#include <cstddef>
#include <functional>
#include <optional>
#include <tuple>
#include <type_traits>

#include "algo/copy.h"
#include "algo/threaded_copy.h"
#include "debug.h"
#include "directio.h"
#include "exception.h"
#include "fetch_store.h"
#include "file/ofstream.h"
#include "file/temp.h"
#include "formats/mrtrix_utils.h"
#include "header.h"
#include "image_helpers.h"

namespace MR {

template <typename ValueType> class Image : public ImageBase<Image<ValueType>, ValueType> {
public:
  using value_type = ValueType;
  class Buffer;

  Image();
  FORCE_INLINE Image(const Image &) = default;
  FORCE_INLINE Image(Image &&) = default;
  FORCE_INLINE Image &operator=(const Image &image) = default;
  FORCE_INLINE Image &operator=(Image &&) = default;
  ~Image();

  //! used internally to instantiate Image objects
  Image(const std::shared_ptr<Buffer> &, const Stride::List & = Stride::List());

  FORCE_INLINE bool valid() const { return bool(buffer); }
  FORCE_INLINE bool operator!() const { return !valid(); }

  //! get generic key/value text attributes
  FORCE_INLINE const KeyValues &keyval() const { return buffer->keyval(); }

  FORCE_INLINE std::string name() const { return buffer->name(); }
  FORCE_INLINE const std::filesystem::path &path() const {
    static const std::filesystem::path empty;
    return valid() ? static_cast<const Header &>(*buffer).path() : empty;
  }
  FORCE_INLINE const transform_type &transform() const { return buffer->transform(); }

  FORCE_INLINE size_t ndim() const { return buffer->ndim(); }
  FORCE_INLINE ssize_t size(size_t axis) const { return buffer->size(axis); }
  FORCE_INLINE default_type spacing(size_t axis) const { return buffer->spacing(axis); }
  FORCE_INLINE ssize_t stride(size_t axis) const { return strides[axis]; }

  //! offset to current voxel from start of data
  FORCE_INLINE size_t offset() const { return data_offset; }

  //! reset index to zero (origin)
  FORCE_INLINE void reset() {
    for (size_t n = 0; n < ndim(); ++n)
      this->index(n) = 0;
  }

  //! get position of current voxel location along \a axis
  FORCE_INLINE ssize_t get_index(size_t axis) const { return x[axis]; }
  //! move position of current voxel location along \a axis
  FORCE_INLINE void move_index(size_t axis, ssize_t increment) {
    data_offset += stride(axis) * increment;
    x[axis] += increment;
  }

  FORCE_INLINE bool is_direct_io() const { return data_pointer; }

  //! get voxel value at current location
  FORCE_INLINE ValueType get_value() const {
    if (data_pointer)
      return Raw::fetch_native<ValueType>(data_pointer, data_offset);
    return buffer->get_value(data_offset);
  }
  //! set voxel value at current location
  FORCE_INLINE void set_value(ValueType val) {
    if (data_pointer)
      Raw::store_native<ValueType>(val, data_pointer, data_offset);
    else
      buffer->set_value(data_offset, val);
  }

  //! use for debugging
  friend std::ostream &operator<<(std::ostream &stream, const Image &V) {
    stream << "\"" << V.path().string() << "\", datatype " << DataType::from<Image::value_type>().specifier()
           << ", index [ ";
    for (size_t n = 0; n < V.ndim(); ++n)
      stream << V.index(n) << " ";
    stream << "], current offset = " << V.offset() << ", ";
    if (is_out_of_bounds(V))
      stream << "outside FoV";
    else
      stream << "value = " << V.value();
    if (!V.data_pointer)
      stream << " (using indirect IO)";
    else
      stream << " (using direct IO, data at " << V.data_pointer << ")";
    return stream;
  }

  //! write out the contents of a direct IO image to file
  /*!
   * returns the name of the image - needed by display() to get the
   * name of the temporary file to supply to MRView.
   *
   * \note this is \e not the recommended way to save an image - only use
   * this function when you absolutely need to minimise RAM usage on
   * write-out (this avoids any further buffering before write-out).
   *
   * \note this will only work for images accessed using direct IO (i.e.
   * opened as a scratch image, or by passing a DirectIO request to one of
   * the Image factory functions), and only supports output to MRtrix format
   * images (*.mif / *.mih). There is a chance that images opened in other
   * ways may also use direct IO (e.g. if the datatype & strides match, and
   * the image is single-file); you can check using the is_direct_io()
   * method. If there is any possibility that this image might use indirect
   * IO, you should use the save() function instead (and even then, it
   * should only be used for debugging purposes). */
  std::filesystem::path dump_to_mrtrix_file(const std::filesystem::path &filepath) const;

  //! return RAM address of current voxel
  /*! \note this will only work if image access is direct (i.e. for a
   * scratch image, with preloading, or when the data type is native and
   * without scaling. */
  ValueType *address() const {
    assert(data_pointer != nullptr && "Image::address() can only be used when image access is via direct RAM access");
    return data_pointer ? static_cast<ValueType *>(data_pointer) + data_offset : nullptr;
  }

  //! open an existing image; pass \a direct_io to demand direct RAM access, see DirectIO.
  static Image open(const std::filesystem::path &image_path,
                    std::optional<DirectIO> direct_io = std::nullopt,
                    bool read_write_if_existing = false) {
    return Header::open(image_path).get_image<ValueType>(direct_io, read_write_if_existing);
  }
  //! create a new image; pass \a direct_io to demand direct RAM access, see DirectIO.
  static Image create(const std::filesystem::path &image_path,
                      const Header &template_header,
                      std::optional<DirectIO> direct_io = std::nullopt,
                      bool add_to_command_history = true) {
    return Header::create(image_path, template_header, add_to_command_history).get_image<ValueType>(direct_io);
  }
  //! allocate a scratch image; pass \a direct_io to constrain the in-RAM stride layout, see DirectIO.
  static Image scratch(const Header &template_header,
                       std::string_view label = "scratch image",
                       std::optional<DirectIO> direct_io = std::nullopt) {
    return Header::scratch(template_header, label).get_image<ValueType>(direct_io);
  }

  //! shared reference to header/buffer
  std::shared_ptr<Buffer> buffer;

protected:
  //! pointer to data address whether in RAM or MMap
  void *data_pointer;
  //! voxel indices
  std::vector<ssize_t> x;
  //! voxel indices
  Stride::List strides;
  //! offset to currently pointed-to voxel
  size_t data_offset;
};

template <typename ValueType> class Image<ValueType>::Buffer : public Header {
public:
  //! construct a Buffer object to access the data in the image specified
  /*! If \a direct_io is provided, the data will be preloaded into RAM during
   * construction (if required to satisfy the request); the ::ram member is
   * thereafter immutable until destruction. */
  Buffer(Header &H, bool read_write_if_existing = false, std::optional<DirectIO> direct_io = std::nullopt);
  Buffer(Buffer &&) = default;
  Buffer &operator=(const Buffer &) = delete;
  Buffer &operator=(Buffer &&) = default;
  Buffer(const Buffer &b) : Header(b), fetch_func(b.fetch_func), store_func(b.store_func) {}
  ~Buffer();

  FORCE_INLINE ValueType get_value(size_t offset) const {
    ssize_t nseg = offset / io->segment_size();
    return fetch_func(io->segment(nseg), offset - nseg * io->segment_size(), intensity_offset(), intensity_scale());
  }

  FORCE_INLINE void set_value(size_t offset, ValueType val) const {
    ssize_t nseg = offset / io->segment_size();
    store_func(val, io->segment(nseg), offset - nseg * io->segment_size(), intensity_offset(), intensity_scale());
  }

  void *get_data_pointer();

  FORCE_INLINE ImageIO::Base *get_io() const { return io.get(); }

  class RAM {
  public:
    RAM(const size_t bytes, const Stride::List &with_strides, const size_t with_offset)
        : data(std::make_unique<std::byte[]>(bytes)), strides(with_strides), offset(with_offset) {}
    std::unique_ptr<std::byte[]> data;
    Stride::List strides;
    size_t offset;
  };
  //! Holds the preloaded RAM buffer when direct IO was requested at construction.
  /*! \note Set only during Buffer construction (before any shared_ptr<Buffer>
   * exists); never mutated thereafter except by the destructor as part of writeback.
   * Concurrent reads from copies of an Image referencing this buffer therefore
   * require no synchronisation. */
  std::optional<RAM> ram;

protected:
  std::function<ValueType(const void *, size_t, default_type, default_type)> fetch_func;
  std::function<void(ValueType, void *, size_t, default_type, default_type)> store_func;

  void set_fetch_store_functions() { _set_fetch_store_scale_functions(fetch_func, store_func, datatype()); }
};

//! \cond skip

namespace {

// lightweight struct to copy data into:
template <typename ValueType> struct TmpImage : public ImageBase<TmpImage<ValueType>, ValueType> {
  using value_type = ValueType;

  TmpImage(const typename Image<ValueType>::Buffer &b,
           void *const data,
           std::vector<ssize_t> x,
           const Stride::List &strides,
           size_t offset)
      : b(b), data(data), x(x), strides(strides), offset(offset) {}

  const typename Image<ValueType>::Buffer &b;
  void *const data;
  std::vector<ssize_t> x;
  const Stride::List &strides;
  size_t offset;

  bool valid() const { return true; }
  const std::string name() const { return "direct IO buffer"; }
  FORCE_INLINE size_t ndim() const { return b.ndim(); }
  FORCE_INLINE ssize_t size(size_t axis) const { return b.size(axis); }
  FORCE_INLINE ssize_t stride(size_t axis) const { return strides[axis]; }

  FORCE_INLINE ssize_t get_index(size_t axis) const { return x[axis]; }
  FORCE_INLINE void move_index(size_t axis, ssize_t increment) {
    offset += stride(axis) * increment;
    x[axis] += increment;
  }

  FORCE_INLINE value_type get_value() const { return Raw::fetch_native<ValueType>(data, offset); }
  FORCE_INLINE void set_value(ValueType val) { Raw::store_native<ValueType>(val, data, offset); }
};

} // namespace

template <typename ValueType>
Image<ValueType>::Buffer::Buffer(Header &H, bool read_write_if_existing, std::optional<DirectIO> direct_io)
    : Header(H) {
  assert(H.valid() && "IO handler must be set when creating an Image");
  assert((H.is_file_backed() ? is_data_type<ValueType>::value : true) &&
         "class types cannot be stored on file using the Image class");

  acquire_io(H);
  io->set_readwrite_if_existing(read_write_if_existing);
  io->open(*this, footprint<ValueType>(voxel_count(*this)));
  if (io->is_file_backed())
    set_fetch_store_functions();

  if (!direct_io.has_value())
    return;

  // Resolve requested layout against this buffer; an empty list means "any layout".
  Stride::List desired = direct_io->resolve(*this);
  bool preload = (datatype() != DataType::from<ValueType>()) || (io->files.size() > 1);
  if (!desired.empty()) {
    auto new_strides = Stride::get_actual(Stride::get_nearest_match(*this, desired), *this);
    preload |= (new_strides != Stride::get(*this));
    desired = new_strides;
  } else {
    desired = Stride::get(*this);
  }

  if (!preload)
    return;

  // Allocate aside; only commit to ::ram once the copy has completed, so that
  // get_data_pointer() invoked by the source Image below still reflects
  // pre-preload state (and therefore reads via the file-backed segments).
  const auto buffer_size = footprint<ValueType>(voxel_count(*this));
  RAM staging(buffer_size, desired, Stride::offset(desired, *this));

  if (io->is_image_new()) {
    memset(staging.data.get(), 0, buffer_size);
  } else {
    // Wrap *this in a no-op-deleter shared_ptr purely to source-iterate.
    // Safe because no other shared_ptr<Buffer> exists yet (we are still in
    // construction and the unique_ptr in the factory has not been released).
    std::shared_ptr<Buffer> self(this, [](Buffer *) {});
    Image<ValueType> src(self);
    TmpImage<ValueType> dest{
        *this, staging.data.get(), std::vector<ssize_t>(ndim(), 0), staging.strides, staging.offset};
    threaded_copy_with_progress_message("preloading data for \"" + name() + "\"", src, dest);
  }

  ram.emplace(std::move(staging));
}

template <typename ValueType> void *Image<ValueType>::Buffer::get_data_pointer() {
  if (ram.has_value()) // preloaded by Buffer ctor in response to a DirectIO request
    return ram->data.get();

  assert(io && "data pointer will only be set for valid Images");
  if (!io->is_file_backed()) // this is a scratch image
    return io->segment(0);

  // check whether we can still do direct IO
  // if so, return address where mapped
  if (io->nsegments() == 1 && datatype() == DataType::from<ValueType>() && intensity_offset() == 0.0 &&
      intensity_scale() == 1.0)
    return io->segment(0);

  // can't do direct IO
  return nullptr;
}

template <typename ValueType>
Image<ValueType> Header::get_image(std::optional<DirectIO> direct_io, bool read_write_if_existing) {
  if (!valid())
    throw Exception("FIXME: don't invoke get_image() with invalid Header!");
  // Build the buffer in a unique_ptr while it has no observers, so any
  // direct-IO preload completes before the shared_ptr is published. After this
  // point, Buffer::ram is immutable until destruction (writeback).
  auto raw = std::make_unique<typename Image<ValueType>::Buffer>(*this, read_write_if_existing, std::move(direct_io));
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  const Stride::List image_strides = raw->ram.has_value() ? raw->ram->strides : Stride::List();
  std::shared_ptr<typename Image<ValueType>::Buffer> buffer(std::move(raw));
  return Image<ValueType>(buffer, image_strides);
}

template <typename ValueType> FORCE_INLINE Image<ValueType>::Image() : data_pointer(nullptr), data_offset(0) {}

template <typename ValueType>
Image<ValueType>::Image(const std::shared_ptr<Image<ValueType>::Buffer> &buffer_p, const Stride::List &desired_strides)
    : buffer(buffer_p),
      data_pointer(buffer->get_data_pointer()),
      x(ndim(), 0),
      strides(!desired_strides.empty() ? desired_strides : Stride::get(*buffer)),
      data_offset(Stride::offset(*this)) {
  assert(buffer);
  assert(data_pointer || buffer->get_io());
  DEBUG("image \"" + name() + "\" initialised with strides = " + str(strides) + ", start = " + str(data_offset) +
        ", using " + (is_direct_io() ? "" : "in") + "direct IO");
}

template <typename ValueType> Image<ValueType>::~Image() {}

template <typename ValueType> Image<ValueType>::Buffer::~Buffer() {
  if (!get_io())
    return;
  if (!get_io()->is_image_readwrite())
    return;
  if (!ram.has_value())
    return;
  try {
    auto local_data = std::move(ram->data);
    // Construct a temporary shared_ptr with a no-op deleter so that Image can be
    // used as a write destination without triggering a second deletion of this.
    std::shared_ptr<Buffer> self(this, [](Buffer *) {});
    TmpImage<ValueType> src = {*this, local_data.get(), std::vector<ssize_t>(ndim(), 0), ram->strides, ram->offset};
    Image<ValueType> dest(self);
    threaded_copy_with_progress_message("writing back direct IO buffer for \"" + name() + "\"", src, dest);
  } catch (Exception &e) {
    Exception(e, "Error during writeback of image \"" + name() + "\"; image may be corrupt").display();
  } catch (std::exception &e) {
    WARN("Error during writeback of image \"" + name() + "\"---" + e.what() + "---image may be corrupt");
  }
}

template <typename ValueType>
std::filesystem::path Image<ValueType>::dump_to_mrtrix_file(const std::filesystem::path &filepath) const {
  if (!data_pointer || !Path::has_suffix(filepath, {".mih", ".mif"}))
    throw Exception("FIXME: image not suitable for use with 'Image::dump_to_mrtrix_file()'");

  // try to dump file to mrtrix format if possible (direct IO)
  std::filesystem::path resolved_path(filepath);
  if (is_dash(filepath.string()))
    resolved_path = File::create_tempfile(0, ".mif");

  DEBUG("dumping image \"" + name() + "\" to file \"" + resolved_path.string() + "\"...");

  File::OFStream out(resolved_path, std::ios::out | std::ios::binary);
  out << "mrtrix image\n";
  Formats::write_mrtrix_header(*buffer, out);

  const bool single_file = resolved_path.extension() == ".mif";

  int64_t offset = 0;
  out << "file: ";
  std::filesystem::path data_path = resolved_path;
  if (single_file) {
    offset = static_cast<int64_t>(out.tellp()) + int64_t(18);
    offset += ((4 - (offset % 4)) % 4);
    out << ". " << offset << "\nEND\n";
  } else {
    data_path.replace_extension(".dat");
    out << data_path.filename().string() << "\n";
    out.close();
    out.open(data_path, std::ios::out | std::ios::binary);
  }

  const int64_t data_size = footprint(*buffer);
  out.seekp(offset, out.beg);
  out.write((const char *)data_pointer, data_size);
  if (!out.good())
    throw Exception("error writing back contents of file \"" + data_path.string() + "\": " + MR::C_strerror(errno));
  out.close();

  // If data_size exceeds some threshold, ostream artificially increases the file size beyond that required at close()
  // TODO check whether this is still needed...?
  std::filesystem::resize_file(data_path, offset + data_size);

  return resolved_path;
}

template <class ImageType>
std::filesystem::path _save_generic(ImageType &x, const std::filesystem::path &filepath, bool use_multi_threading) {
  auto out = Image<typename ImageType::value_type>::create(filepath, x);
  if (use_multi_threading)
    threaded_copy(x, out);
  else
    copy(x, out);
  return out.path();
}

//! \endcond

//! save contents of an existing image to file (for debugging only)
template <class ImageType>
typename std::enable_if<is_adapter_type<typename std::remove_reference<ImageType>::type>::value,
                        std::filesystem::path>::type
save(ImageType &&x, const std::filesystem::path &filepath, bool use_multi_threading = true) {
  return _save_generic(x, filepath, use_multi_threading);
}

//! save contents of an existing image to file (for debugging only)
template <class ImageType>
typename std::enable_if<is_pure_image<typename std::remove_reference<ImageType>::type>::value,
                        std::filesystem::path>::type
save(ImageType &&x, const std::filesystem::path &filepath, bool use_multi_threading = true) {
  try {
    return x.dump_to_mrtrix_file(filepath);
  } catch (Exception &) {
    return _save_generic(x, filepath, use_multi_threading);
  }
}

//! display the contents of an image in MRView (for debugging only)
template <class ImageType> typename enable_if_image_type<ImageType, void>::type display(ImageType &x) {
  const std::filesystem::path filepath = save(x, "-");
  CONSOLE("displaying image \"" + filepath.string() + "\"");
  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  if (system(("bash -c \"mrview " + filepath.string() + "\"").c_str()))
    WARN(std::string("error invoking viewer: ") + MR::C_strerror(errno));
}
// Explicit instantiations in image.cpp:
extern template MR::Image<bool>::Buffer::~Buffer();
extern template MR::Image<int8_t>::Buffer::~Buffer();
extern template MR::Image<uint8_t>::Buffer::~Buffer();
extern template MR::Image<int16_t>::Buffer::~Buffer();
extern template MR::Image<uint16_t>::Buffer::~Buffer();
extern template MR::Image<int32_t>::Buffer::~Buffer();
extern template MR::Image<uint32_t>::Buffer::~Buffer();
extern template MR::Image<int64_t>::Buffer::~Buffer();
extern template MR::Image<uint64_t>::Buffer::~Buffer();
extern template MR::Image<Eigen::half>::Buffer::~Buffer();
extern template MR::Image<float>::Buffer::~Buffer();
extern template MR::Image<double>::Buffer::~Buffer();
extern template MR::Image<cfloat>::Buffer::~Buffer();
extern template MR::Image<cdouble>::Buffer::~Buffer();
} // namespace MR
