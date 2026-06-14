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

#include "dwi/tractography/formats/trx.h"

#include "dwi/tractography/nonfinite.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <nlohmann/json.hpp>
#include <system_error>
#include <type_traits>
#include <zip.h>

#include "app.h"
#include "exception.h"
#include "file/mmap.h"
#include "file/ofstream.h"
#include "file/path.h"
#include "file/temp.h"
#include "half.h"
#include "match_variant.h"
#include "raw.h"
#include "signal_handler.h"

#include "dwi/tractography/sidecar_value.h"

/* ************************************************************************ */
/*                       Filename grammar / dtype                          */
/* ************************************************************************ */

namespace MR::DWI::Tractography::Formats::TRXUtils {

namespace {

//! \brief Map a TRX dtype extension string to the corresponding MR::DataType.
std::optional<DataType> dtype_from_extension(std::string_view ext) {
  if (ext == "bit")
    return DataType(DataType::Bit);
  if (ext == "uint8")
    return DataType(DataType::UInt8);
  if (ext == "int8")
    return DataType(DataType::Int8);
  if (ext == "uint16")
    return DataType::native(DataType(DataType::UInt16));
  if (ext == "int16")
    return DataType::native(DataType(DataType::Int16));
  if (ext == "uint32")
    return DataType::native(DataType(DataType::UInt32));
  if (ext == "int32")
    return DataType::native(DataType(DataType::Int32));
  if (ext == "uint64")
    return DataType::native(DataType(DataType::UInt64));
  if (ext == "int64")
    return DataType::native(DataType(DataType::Int64));
  if (ext == "float16")
    return DataType::native(DataType(DataType::Float16));
  if (ext == "float32")
    return DataType::native(DataType(DataType::Float32));
  if (ext == "float64")
    return DataType::native(DataType(DataType::Float64));
  return std::nullopt;
}

//! \brief whether a string is a non-empty run of decimal digits.
bool is_integer(std::string_view s) {
  if (s.empty())
    return false;
  for (const char c : s)
    if (c < '0' || c > '9')
      return false;
  return true;
}

} // namespace

//! \brief Map an MR::DataType element type to its TRX dtype extension string.
std::string extension_from_dtype(const DataType dtype) {
  switch (dtype() & (DataType::Type | DataType::Signed)) {
  case DataType::Bit:
    return "bit";
  case DataType::UInt8:
    return "uint8";
  case DataType::Int8:
    return "int8";
  case DataType::UInt16:
    return "uint16";
  case DataType::Int16:
    return "int16";
  case DataType::UInt32:
    return "uint32";
  case DataType::Int32:
    return "int32";
  case DataType::UInt64:
    return "uint64";
  case DataType::Int64:
    return "int64";
  case DataType::Float16:
    return "float16";
  case DataType::Float32:
    return "float32";
  case DataType::Float64:
    return "float64";
  default:
    throw Exception("datatype \"" + dtype.specifier() + "\" has no TRX representation");
  }
}

std::optional<FilenameGrammar> parse_filename(std::string_view filename) {
  // Split on '.' into [name].[optional integer M].[dtype].
  const size_t last_dot = filename.rfind('.');
  if (last_dot == std::string_view::npos || last_dot + 1 == filename.size())
    return std::nullopt;
  const std::string_view ext = filename.substr(last_dot + 1);
  const std::optional<DataType> dtype = dtype_from_extension(ext);
  if (!dtype.has_value())
    return std::nullopt;

  std::string_view remainder = filename.substr(0, last_dot);
  size_t columns = 1;
  const size_t mid_dot = remainder.rfind('.');
  if (mid_dot != std::string_view::npos) {
    const std::string_view mid = remainder.substr(mid_dot + 1);
    if (is_integer(mid)) {
      columns = static_cast<size_t>(std::stoull(std::string(mid)));
      remainder = remainder.substr(0, mid_dot);
    }
  }
  FilenameGrammar out;
  out.name = std::string(remainder);
  out.columns = std::max<size_t>(1, columns);
  out.dtype = *dtype;
  return out;
}

namespace {

//! \brief classify a member's relative path into a TRX role and field basename.
struct MemberClass {
  MemberRole role;
  std::string basename; //!< field name without directory or dtype suffix
};

MemberClass classify(std::string_view relpath) {
  // Normalise to '/'-separated POSIX components.
  std::string path(relpath);
  std::replace(path.begin(), path.end(), '\\', '/');
  // Strip a leading "./".
  if (path.rfind("./", 0) == 0)
    path = path.substr(2);

  const size_t slash = path.rfind('/');
  const std::string leaf = (slash == std::string::npos) ? path : path.substr(slash + 1);
  const std::string folder = (slash == std::string::npos) ? std::string() : path.substr(0, slash);

  if (folder.empty() && (leaf == "header.json" || leaf == "header"))
    return {MemberRole::Header, "header"};

  const std::optional<FilenameGrammar> grammar = parse_filename(leaf);
  const std::string base = grammar.has_value() ? grammar->name : leaf;

  if (folder.empty() && base == "positions")
    return {MemberRole::Positions, "positions"};
  if (folder.empty() && base == "offsets")
    return {MemberRole::Offsets, "offsets"};
  if (folder == "dps")
    return {MemberRole::DPS, base};
  if (folder == "dpv")
    return {MemberRole::DPV, base};
  if (folder == "groups")
    return {MemberRole::Groups, base};
  if (folder.rfind("dpg", 0) == 0)
    return {MemberRole::DPG, base};
  return {MemberRole::Unknown, base};
}

} // namespace

/* ************************************************************************ */
/*                       Header parse / summarise                          */
/* ************************************************************************ */

void TrxSource::parse_header(DatasetSummary &summary) {
  std::string header_name;
  for (const auto &name : member_names()) {
    const MemberClass mc = classify(name);
    if (mc.role == MemberRole::Header) {
      header_name = name;
      break;
    }
  }
  if (header_name.empty())
    throw Exception("TRX dataset contains no \"header.json\" member");

  const MemberRange range = member(header_name);
  nlohmann::json json;
  try {
    json = nlohmann::json::parse(std::string(reinterpret_cast<const char *>(range.data), range.size));
  } catch (std::exception &e) {
    throw Exception(std::string("failed to parse TRX header.json: ") + e.what());
  }

  // Validate that all four compulsory header fields are present (steps 2/6).
  for (const std::string_view key : {key_voxel_to_rasmm, key_dimensions, key_nb_streamlines, key_nb_vertices}) {
    if (!json.contains(std::string(key)))
      throw Exception("TRX header.json is missing required field \"" + std::string(key) + "\"");
  }

  // VOXEL_TO_RASMM: a 4×4 list of lists.
  summary.voxel_to_rasmm.setIdentity();
  {
    const auto &matrix = json.at(std::string(key_voxel_to_rasmm));
    if (!matrix.is_array() || matrix.size() != 4)
      throw Exception("TRX header.json \"VOXEL_TO_RASMM\" is not a 4×4 matrix");
    for (size_t row = 0; row != 4; ++row) {
      const auto &cols = matrix.at(row);
      if (!cols.is_array() || cols.size() != 4)
        throw Exception("TRX header.json \"VOXEL_TO_RASMM\" is not a 4×4 matrix");
      for (size_t col = 0; col != 4; ++col) {
        if (row != 3)
          summary.voxel_to_rasmm(row, col) = cols.at(col).get<double>();
      }
    }
  }

  // DIMENSIONS: a list of 3 integers.
  {
    const auto &dims = json.at(std::string(key_dimensions));
    if (!dims.is_array() || dims.size() < 3)
      throw Exception("TRX header.json \"DIMENSIONS\" is not a length-3 list");
    for (size_t axis = 0; axis != 3; ++axis)
      summary.dimensions[axis] = dims.at(axis).get<uint16_t>();
  }

  summary.nb_streamlines = json.at(std::string(key_nb_streamlines)).get<uint64_t>();
  summary.nb_vertices = json.at(std::string(key_nb_vertices)).get<uint64_t>();
}

DatasetSummary TrxSource::summarise() {
  DatasetSummary summary{};
  parse_header(summary);

  bool have_positions = false;
  bool have_offsets = false;

  std::vector<std::string> names = member_names();
  std::sort(names.begin(), names.end());

  for (const auto &name : names) {
    const MemberClass mc = classify(name);
    if (mc.role == MemberRole::Header || mc.role == MemberRole::Unknown)
      continue;

    const size_t slash = name.rfind('/');
    const std::string leaf = (slash == std::string::npos) ? name : name.substr(slash + 1);
    const std::optional<FilenameGrammar> grammar = parse_filename(leaf);
    if (!grammar.has_value())
      continue;

    switch (mc.role) {
    case MemberRole::Positions: {
      if (!grammar->dtype.is_floating_point())
        throw Exception("TRX \"positions\" must be a float16/32/64 array");
      summary.positions_dtype = grammar->dtype;
      summary.positions = member(name);
      have_positions = true;
    } break;
    case MemberRole::Offsets: {
      summary.offsets_dtype = grammar->dtype;
      summary.offsets = member(name);
      const size_t bytes = grammar->dtype.bytes();
      summary.offsets_count = (bytes != 0) ? summary.offsets.size / bytes : 0;
      have_offsets = true;
    } break;
    case MemberRole::DPS:
    case MemberRole::DPV: {
      SidecarSummary sc;
      sc.name = grammar->name;
      sc.role = (mc.role == MemberRole::DPS) ? FieldRole::DPS : FieldRole::DPV;
      sc.dtype = grammar->dtype;
      sc.columns = grammar->columns;
      sc.rows = (sc.role == FieldRole::DPS) ? summary.nb_streamlines : summary.nb_vertices;
      sc.range = member(name);
      summary.sidecars.push_back(std::move(sc));
    } break;
    case MemberRole::Groups: {
      // A "groups/<name>.<dtype>" index table (Stage 17). The TRX spec mandates
      //   uint32; any integer dtype is accepted defensively.
      GroupSummary gs;
      gs.name = grammar->name;
      gs.dtype = grammar->dtype;
      gs.range = member(name);
      const size_t bytes = grammar->dtype.bytes();
      gs.count = (bytes != 0) ? gs.range.size / bytes : 0;
      summary.groups.push_back(std::move(gs));
    } break;
    case MemberRole::DPG: {
      // A "dpg/<group>/<field>[.M].<dtype>" per-group metadatum (Stage 17). The
      //   group is the directory component immediately below "dpg/".
      DPGSummary ds;
      ds.field = grammar->name;
      ds.dtype = grammar->dtype;
      ds.columns = grammar->columns;
      ds.range = member(name);
      // Recover the group name (the sub-folder under "dpg/").
      std::string normalised(name);
      std::replace(normalised.begin(), normalised.end(), '\\', '/');
      if (normalised.rfind("./", 0) == 0)
        normalised = normalised.substr(2);
      const size_t leaf_slash = normalised.rfind('/');
      const std::string folder = (leaf_slash == std::string::npos) ? std::string() : normalised.substr(0, leaf_slash);
      // folder == "dpg/<group>" (or, degenerately, "dpg"); take the trailing path
      //   component as the group name.
      const size_t group_slash = folder.rfind('/');
      ds.group = (group_slash == std::string::npos) ? std::string() : folder.substr(group_slash + 1);
      if (!ds.group.empty())
        summary.dpg.push_back(std::move(ds));
    } break;
    default:
      break;
    }
  }

  if (!have_positions)
    throw Exception("TRX dataset is missing the required \"positions\" array");
  if (!have_offsets)
    throw Exception("TRX dataset is missing the required \"offsets\" array");

  return summary;
}

/* ************************************************************************ */
/*                      Directory-backed TrxSource                         */
/* ************************************************************************ */

namespace {

//! \brief A TrxSource backed by a filesystem directory; each member is mmap'd.
class DirectorySource : public TrxSource {
public:
  explicit DirectorySource(std::filesystem::path root) : root(std::move(root)) {
    for (const auto &entry : std::filesystem::recursive_directory_iterator(this->root)) {
      if (!entry.is_regular_file())
        continue;
      const std::string rel = std::filesystem::relative(entry.path(), this->root).generic_string();
      files.push_back(rel);
    }
  }

  std::vector<std::string> member_names() const override { return files; }

  bool has_member(std::string_view name) const override {
    return std::find(files.begin(), files.end(), name) != files.end();
  }

  MemberRange member(std::string_view name) override {
    const std::string key(name);
    auto existing = maps.find(key);
    if (existing != maps.end())
      return {existing->second->address(), static_cast<size_t>(existing->second->size())};
    const std::filesystem::path full = root / key;
    auto mmap = std::make_shared<File::MMap>(File::Entry(full), false, true);
    const MemberRange range{mmap->address(), static_cast<size_t>(mmap->size())};
    maps.emplace(key, std::move(mmap));
    return range;
  }

private:
  std::filesystem::path root;
  std::vector<std::string> files;
  std::map<std::string, std::shared_ptr<File::MMap>> maps;
};

//! \brief A TrxSource backed by an uncompressed (ZIP_STORE) archive, mmap'd in
//!   place (D5): each member points into the archive's own memory map, with no
//!   extraction. Member data offsets are recovered by walking the local file
//!   headers; only ZIP_STORE members can be pointed at directly.
class ZipStoreSource : public TrxSource {
public:
  ZipStoreSource(std::shared_ptr<File::MMap> archive, std::map<std::string, MemberRange> ranges)
      : archive(std::move(archive)), ranges(std::move(ranges)) {}

  std::vector<std::string> member_names() const override {
    std::vector<std::string> out;
    out.reserve(ranges.size());
    for (const auto &pair : ranges)
      out.push_back(pair.first);
    return out;
  }

  bool has_member(std::string_view name) const override { return ranges.find(std::string(name)) != ranges.end(); }

  MemberRange member(std::string_view name) override {
    auto it = ranges.find(std::string(name));
    if (it == ranges.end())
      throw Exception("TRX archive has no member \"" + std::string(name) + "\"");
    return it->second;
  }

private:
  std::shared_ptr<File::MMap> archive;
  std::map<std::string, MemberRange> ranges;
};

//! \brief A TrxSource backed by a temporary directory into which a compressed
//!   (ZIP_DEFLATE) archive has been extracted (D5). The temp dir is registered
//!   with the MRtrix signal handler for cleanup on unexpected termination, and
//!   removed in the destructor.
class ExtractedSource : public TrxSource {
public:
  explicit ExtractedSource(std::filesystem::path tempdir) : tempdir(std::move(tempdir)), inner(this->tempdir) {}

  ~ExtractedSource() override {
    std::error_code ec;
    std::filesystem::remove_all(tempdir, ec);
    SignalHandler::unmark_file_for_deletion(tempdir);
  }

  std::vector<std::string> member_names() const override { return inner.member_names(); }
  bool has_member(std::string_view name) const override { return inner.has_member(name); }
  MemberRange member(std::string_view name) override { return inner.member(name); }

private:
  std::filesystem::path tempdir;
  DirectorySource inner;
};

constexpr uint32_t zip_local_signature = 0x04034b50; //!< "PK\3\4"
constexpr uint16_t zip_method_store = 0;

//! \brief Walk an uncompressed zip archive's local headers; map STORE members to
//!   in-place byte ranges. Returns nullopt if any member is not ZIP_STORE
//!   (signalling that the archive must instead be extracted).
std::optional<std::map<std::string, MemberRange>> scan_zip_store(const std::byte *base, size_t size) {
  std::map<std::string, MemberRange> ranges;
  size_t offset = 0;
  while (offset + 30 <= size) {
    const uint32_t signature = Raw::fetch_LE<uint32_t>(base + offset);
    if (signature != zip_local_signature)
      break; // reached the central directory (or end)
    const uint16_t method = Raw::fetch_LE<uint16_t>(base + offset + 8);
    const uint32_t comp_size = Raw::fetch_LE<uint32_t>(base + offset + 18);
    const uint16_t name_length = Raw::fetch_LE<uint16_t>(base + offset + 26);
    const uint16_t extra_length = Raw::fetch_LE<uint16_t>(base + offset + 28);
    const size_t name_offset = offset + 30;
    if (name_offset + name_length > size)
      return std::nullopt;
    std::string name(reinterpret_cast<const char *>(base + name_offset), name_length);
    const size_t data_offset = name_offset + name_length + extra_length;
    if (data_offset + comp_size > size)
      return std::nullopt;
    if (method != zip_method_store)
      return std::nullopt; // a compressed member; cannot point in place
    // Skip directory entries (trailing slash, zero length).
    if (!name.empty() && name.back() != '/')
      ranges.emplace(name, MemberRange{base + data_offset, comp_size});
    offset = data_offset + comp_size;
  }
  if (ranges.empty())
    return std::nullopt;
  return ranges;
}

//! \brief Extract every member of a zip archive into \a dest using libzip.
void extract_archive(const std::filesystem::path &archive_path, const std::filesystem::path &dest) {
  int err = 0;
  zip_t *za = zip_open(archive_path.string().c_str(), ZIP_RDONLY, &err);
  if (za == nullptr)
    throw Exception("failed to open TRX archive \"" + archive_path.string() + "\" as a zip file");

  const zip_int64_t num = zip_get_num_entries(za, 0);
  try {
    for (zip_int64_t i = 0; i != num; ++i) {
      zip_stat_t st;
      zip_stat_init(&st);
      if (zip_stat_index(za, static_cast<zip_uint64_t>(i), 0, &st) != 0)
        throw Exception("failed to stat entry in TRX archive");
      const std::string name(st.name);
      if (!name.empty() && name.back() == '/')
        continue; // directory entry
      const std::filesystem::path out = dest / name;
      std::filesystem::create_directories(out.parent_path());
      zip_file_t *zf = zip_fopen_index(za, static_cast<zip_uint64_t>(i), 0);
      if (zf == nullptr)
        throw Exception("failed to open archive member \"" + name + "\"");
      File::OFStream stream(out, std::ios::out | std::ios::binary | std::ios::trunc);
      std::vector<char> chunk(1 << 20);
      zip_uint64_t remaining = st.size;
      while (remaining != 0) {
        const zip_int64_t got = zip_fread(zf, chunk.data(), std::min<zip_uint64_t>(remaining, chunk.size()));
        if (got <= 0) {
          zip_fclose(zf);
          throw Exception("error inflating archive member \"" + name + "\"");
        }
        stream.write(chunk.data(), got);
        remaining -= static_cast<zip_uint64_t>(got);
      }
      zip_fclose(zf);
    }
  } catch (...) {
    zip_close(za);
    throw;
  }
  zip_close(za);
}

} // namespace

bool path_is_trx(const std::filesystem::path &path) { return path.extension() == ".trx"; }

std::unique_ptr<TrxSource> open_source(const std::filesystem::path &path) {
  std::error_code ec;
  if (std::filesystem::is_directory(path, ec))
    return std::make_unique<DirectorySource>(path);

  if (!std::filesystem::exists(path, ec))
    throw Exception("TRX dataset \"" + path.string() + "\" does not exist");

  // A file: probe whether the whole archive is ZIP_STORE (mmap in place; D5),
  //   otherwise extract the compressed archive to a temp dir.
  auto archive = std::make_shared<File::MMap>(File::Entry(path), false, true);
  const std::byte *const base = archive->address();
  const size_t bytes = static_cast<size_t>(archive->size());
  if (bytes < 4 || Raw::fetch_LE<uint32_t>(base) != zip_local_signature)
    throw Exception("TRX file \"" + path.string() + "\" is not a zip archive");

  if (std::optional<std::map<std::string, MemberRange>> ranges = scan_zip_store(base, bytes))
    return std::make_unique<ZipStoreSource>(archive, std::move(*ranges));

  // Compressed (or mixed) archive: extract to a temp dir registered for cleanup.
  archive.reset();
  const std::filesystem::path tempdir = File::create_tempdir(".trxdir");
  SignalHandler::mark_file_for_deletion(tempdir);
  extract_archive(path, tempdir);
  return std::make_unique<ExtractedSource>(tempdir);
}

Backing classify_backing(const std::filesystem::path &path) {
  std::error_code ec;
  if (std::filesystem::is_directory(path, ec))
    return Backing::Directory;
  if (!std::filesystem::exists(path, ec))
    return Backing::Missing;
  // A regular file: probe whether every member is ZIP_STORE (uncompressed) or
  //   at least one is deflate-compressed.
  try {
    File::MMap archive(File::Entry(path), false, true);
    const std::byte *const base = archive.address();
    const size_t bytes = static_cast<size_t>(archive.size());
    if (bytes < 4 || Raw::fetch_LE<uint32_t>(base) != zip_local_signature)
      return Backing::CompressedArchive; // not a recognised store layout; treat conservatively
    if (scan_zip_store(base, bytes).has_value())
      return Backing::UncompressedArchive;
    return Backing::CompressedArchive;
  } catch (Exception &) {
    return Backing::CompressedArchive;
  }
}

bool augment_requires_force(const std::filesystem::path &path) {
  switch (classify_backing(path)) {
  case Backing::Missing:
  case Backing::Directory:
  case Backing::UncompressedArchive:
    // Directory: a new member is a new file alongside the existing ones.
    // Uncompressed archive: libzip 1.7.3 atomically replaces the archive on
    //   zip_close(), so a crash leaves the original intact (non-destructive).
    return false;
  case Backing::CompressedArchive:
    // Re-packing a deflate archive rewrites the whole file.
    return true;
  }
  return true;
}

} // namespace MR::DWI::Tractography::Formats::TRXUtils

namespace MR::DWI::Tractography {

namespace TRXUtils = Formats::TRXUtils;

/* ************************************************************************ */
/*                       Properties <-> header geometry                    */
/* ************************************************************************ */

namespace {

//! \brief stamp the TRX header geometry into Properties for round-trip fidelity.
void summary_to_properties(const TRXUtils::DatasetSummary &summary, Properties &properties) {
  // transform_type is a 3×4 AffineCompact matrix (the implicit 4th row is
  //   [0 0 0 1]); serialise the full 4×4 affine row-major, emitting that final
  //   row explicitly.
  std::string transform;
  for (size_t row = 0; row != 4; ++row) {
    for (size_t col = 0; col != 4; ++col) {
      if (!transform.empty())
        transform += ",";
      const double value = (row == 3) ? ((col == 3) ? 1.0 : 0.0) : summary.voxel_to_rasmm(row, col);
      transform += str(value);
    }
  }
  properties["trx_voxel_to_rasmm"] = transform;
  properties["trx_dimensions"] =
      str(summary.dimensions[0]) + "," + str(summary.dimensions[1]) + "," + str(summary.dimensions[2]);
}

//! \brief recover the TRX header geometry from Properties (writer side).
void properties_to_geometry(const Properties &properties,
                            transform_type &voxel_to_rasmm,
                            std::array<uint16_t, 3> &dimensions) {
  voxel_to_rasmm.setIdentity();
  dimensions = {1, 1, 1};
  auto t = properties.find("trx_voxel_to_rasmm");
  if (t != properties.end()) {
    const auto values = MR::parse_floats(t->second);
    if (values.size() == 16) {
      // Only the first three rows are stored in the AffineCompact matrix.
      for (size_t row = 0; row != 3; ++row)
        for (size_t col = 0; col != 4; ++col)
          voxel_to_rasmm(row, col) = values[row * 4 + col];
    }
  }
  auto d = properties.find("trx_dimensions");
  if (d != properties.end()) {
    const auto values = MR::parse_ints<int64_t>(d->second);
    for (size_t axis = 0; axis != 3 && axis != values.size(); ++axis)
      dimensions[axis] = static_cast<uint16_t>(values[axis]);
  }
}

//! \brief Functor that builds a DPSValue for a dps field row in its native dtype.
/*! Dispatched through dispatch_sidecar_datatype against the on-disk datatype so
 * that the ScalarOrVector alternative matches the field's native element type
 * (D7); the row is read in place from the memory-mapped member. */
struct ReadDPS {
  const std::byte *rowbase; //!< head of row `index` within the (N, M) array
  size_t columns;
  size_t elem; //!< bytes per element
  template <typename T> DPSValue operator()() const {
    ScalarOrVector<T> value(1, static_cast<Eigen::Index>(columns));
    for (size_t c = 0; c != columns; ++c)
      value(0, static_cast<Eigen::Index>(c)) = Raw::fetch_native<T>(rowbase + c * elem);
    return make_dps(std::move(value));
  }
};

//! \brief Functor that builds a DPVValue for a streamline's dpv block.
struct ReadDPV {
  const std::byte *blockbase; //!< head of the streamline's first vertex row
  size_t npoints;
  size_t columns;
  size_t elem;
  template <typename T> DPVValue operator()() const {
    VectorOrMatrix<T> value(static_cast<Eigen::Index>(npoints), static_cast<Eigen::Index>(columns));
    for (size_t v = 0; v != npoints; ++v)
      for (size_t c = 0; c != columns; ++c)
        value(static_cast<Eigen::Index>(v), static_cast<Eigen::Index>(c)) =
            Raw::fetch_native<T>(blockbase + (v * columns + c) * elem);
    return make_dpv(std::move(value));
  }
};

} // namespace

/* ************************************************************************ */
/*                          TRXReader<ValueType>                           */
/* ************************************************************************ */

template <class ValueType>
TRXReader<ValueType>::TRXReader(const std::filesystem::path &path, Properties &properties, FieldRegistry &registry)
    : registry(registry), current_streamline(0) {
  source = TRXUtils::open_source(path);
  summary = source->summarise();
  summary_to_properties(summary, properties);

  // Register each sidecar (dps/dpv) field; preserve native on-disk dtype (D7).
  for (const TRXUtils::SidecarSummary &sc : summary.sidecars) {
    FieldDescriptor descriptor;
    descriptor.name = sc.name;
    descriptor.role = sc.role;
    descriptor.dtype = sc.dtype;
    descriptor.columns = sc.columns;
    descriptor.source = FieldSource::Internal;
    descriptor.ordinal = 0;
    sidecar_ordinals.push_back(registry.add(std::move(descriptor)));
  }
}

template <class ValueType> TRXReader<ValueType>::~TRXReader() = default;

template <class ValueType> uint64_t TRXReader<ValueType>::offset_at(const size_t i) const {
  // The offsets array gives the starting vertex of each streamline. If a
  //   trailing total is stored (NB_STREAMLINES+1 entries) index it directly;
  //   otherwise the final streamline runs to NB_VERTICES.
  if (i < summary.offsets_count) {
    const std::byte *const base = summary.offsets.data;
    if (summary.offsets_dtype.bytes() == 8)
      return Raw::fetch_LE<uint64_t>(base, i);
    return static_cast<uint64_t>(Raw::fetch_LE<uint32_t>(base, i));
  }
  return summary.nb_vertices;
}

template <class ValueType>
bool TRXReader<ValueType>::read_streamline(const size_t index,
                                           Streamline<ValueType> &tck,
                                           TractogramItem<ValueType> *item) {
  tck.clear();
  if (index >= summary.nb_streamlines)
    return false;

  const uint64_t start = offset_at(index);
  const uint64_t end = offset_at(index + 1);
  if (end < start || end > summary.nb_vertices)
    throw Exception("malformed TRX offsets: streamline " + str(index) + " has an invalid vertex span");
  const size_t npoints = static_cast<size_t>(end - start);

  tck.set_index(index);
  if (item != nullptr)
    item->streamline.set_index(index);

  // Positions are a contiguous (NB_VERTICES, 3) row-major array in world (RASMM)
  //   space (≡ MRtrix scanner-space), in float16/32/64 (D7).
  const std::byte *const pos_base = summary.positions.data;
  const size_t pos_elem = summary.positions_dtype.bytes();
  for (size_t v = 0; v != npoints; ++v) {
    const size_t row = static_cast<size_t>(start) + v;
    Eigen::Matrix<ValueType, 3, 1> point;
    for (size_t axis = 0; axis != 3; ++axis) {
      const std::byte *const addr = pos_base + (row * 3 + axis) * pos_elem;
      double value = 0.0;
      if (pos_elem == 2)
        value = static_cast<double>(Raw::fetch_native<Eigen::half>(addr));
      else if (pos_elem == 4)
        value = static_cast<double>(Raw::fetch_LE<float>(addr));
      else
        value = Raw::fetch_LE<double>(addr);
      point[axis] = static_cast<ValueType>(value);
    }
    tck.push_back(point);
  }

  if (item == nullptr)
    return true;

  item->dps.resize(registry.dps_count());
  item->dpv.resize(registry.dpv_count());

  for (size_t s = 0; s != summary.sidecars.size(); ++s) {
    const TRXUtils::SidecarSummary &sc = summary.sidecars[s];
    const size_t ordinal = sidecar_ordinals[s];
    const size_t elem = sc.dtype.bytes();
    if (sc.role == FieldRole::DPS) {
      // One 1×M row, sourced from row `index` of the (NB_STREAMLINES, M) array.
      const std::byte *const rowbase = sc.range.data + index * sc.columns * elem;
      item->dps[ordinal] = dispatch_sidecar_datatype(sc.dtype, ReadDPS{rowbase, sc.columns, elem});
    } else {
      // An (npoints, M) block, sourced from rows [start, end) of the dpv array.
      const std::byte *const blockbase = sc.range.data + static_cast<size_t>(start) * sc.columns * elem;
      item->dpv[ordinal] = dispatch_sidecar_datatype(sc.dtype, ReadDPV{blockbase, npoints, sc.columns, elem});
    }
  }
  return true;
}

template <class ValueType> bool TRXReader<ValueType>::operator()(Streamline<ValueType> &tck) {
  if (current_streamline >= summary.nb_streamlines)
    return false;
  const bool ok = read_streamline(current_streamline, tck, nullptr);
  ++current_streamline;
  return ok;
}

template <class ValueType> bool TRXReader<ValueType>::operator()(TractogramItem<ValueType> &item) {
  item.clear();
  if (current_streamline >= summary.nb_streamlines)
    return false;
  const bool ok = read_streamline(current_streamline, item.streamline, &item);
  ++current_streamline;
  return ok;
}

template <class ValueType> void TRXReader<ValueType>::read_grouping(Grouping &grouping) {
  // Groups: each "groups/<name>" is a flat array of streamline indices (uint32
  //   per spec; any integer dtype is decoded). Stored verbatim so the on-disk
  //   ordering and (permitted) overlap round-trip (§2.3).
  for (const TRXUtils::GroupSummary &gs : summary.groups) {
    std::vector<uint32_t> members;
    members.reserve(gs.count);
    const std::byte *const base = gs.range.data;
    const size_t elem = gs.dtype.bytes();
    for (size_t i = 0; i != gs.count; ++i) {
      const std::byte *const addr = base + i * elem;
      uint64_t value = 0;
      switch (elem) {
      case 1:
        value = static_cast<uint64_t>(Raw::fetch_native<uint8_t>(addr));
        break;
      case 2:
        value = static_cast<uint64_t>(Raw::fetch_native<uint16_t>(addr));
        break;
      case 4:
        value = static_cast<uint64_t>(Raw::fetch_native<uint32_t>(addr));
        break;
      default:
        value = Raw::fetch_native<uint64_t>(addr);
        break;
      }
      members.push_back(static_cast<uint32_t>(value));
    }
    grouping.set_group(gs.name, std::move(members));
  }

  // dpg: each "dpg/<group>/<field>" is a single (1, M) row of per-group metadata
  //   in its native on-disk dtype (D7), read via the same functor as a dps row.
  for (const TRXUtils::DPGSummary &ds : summary.dpg) {
    const size_t elem = ds.dtype.bytes();
    DPSValue value = dispatch_sidecar_datatype(ds.dtype, ReadDPS{ds.range.data, ds.columns, elem});
    grouping.set_dpg(ds.group, ds.field, std::move(value));
  }

  grouping.validate(summary.nb_streamlines);
}

/* ************************************************************************ */
/*                          TRXWriter<ValueType>                           */
/* ************************************************************************ */

namespace {

//! \brief make a WriteBuffer that appends its bytes to \a path.
std::shared_ptr<Formats::WriteBuffer> make_appender(const std::filesystem::path &path, const size_t element_size) {
  auto buffer = std::make_shared<Formats::WriteBuffer>(16777216, element_size);
  buffer->set_flush_callback([path](const std::byte *data, size_t size, const Formats::WriteBuffer::Counts &) {
    File::OFStream out(path, std::ios::out | std::ios::binary | std::ios::app);
    out.write(reinterpret_cast<const char *>(data), size);
  });
  return buffer;
}

} // namespace

template <class ValueType>
TRXWriter<ValueType>::TRXWriter(const std::filesystem::path &path,
                                const Properties &properties,
                                const FieldRegistry &registry)
    : path(path), registry(registry), num_streamlines(0), num_vertices(0) {
  if (path.extension() != ".trx" && !path.has_extension())
    throw Exception("output TRX dataset must use the .trx suffix");

  // -force interaction (step 11): a new TRX directory or archive is a fresh
  //   write, so the standard overwrite check applies. (Appending a member to an
  //   existing dataset is handled at the Tractogram level; see trx.h.)
  App::check_overwrite(path);

  properties_to_geometry(properties, voxel_to_rasmm, dimensions);

  // Positions are written as float32 (lossless for the float/double processing
  //   precision and the recommended interchange dtype).
  positions_tempfile = File::create_tempfile(0, ".trxpos");
  positions_buffer = make_appender(positions_tempfile, 3 * sizeof(float));

  // One accumulating temp file per registered dps/dpv field, in its native dtype.
  for (const FieldDescriptor &field : registry) {
    if (field.role != FieldRole::DPS && field.role != FieldRole::DPV)
      continue;
    SidecarOutput output;
    output.descriptor = field;
    output.ordinal = field.ordinal;
    output.tempfile = File::create_tempfile(0, ".trxsc");
    output.buffer = make_appender(output.tempfile, field.dtype.bytes());
    if (field.role == FieldRole::DPS)
      dps_fields.push_back(std::move(output));
    else
      dpv_fields.push_back(std::move(output));
  }

  offsets.push_back(0);
}

template <class ValueType>
void TRXWriter<ValueType>::append(const Streamline<ValueType> &tck, const TractogramItem<ValueType> *item) {
  enforce_vertices(tck, trx_vertex_tolerance);
  const size_t npoints = tck.size();

  // Positions: float32, row-major (npoints, 3).
  std::vector<std::byte> pos_bytes(npoints * 3 * sizeof(float));
  for (size_t v = 0; v != npoints; ++v) {
    for (size_t axis = 0; axis != 3; ++axis)
      Raw::store_LE<float>(static_cast<float>(tck[v][axis]), pos_bytes.data() + (v * 3 + axis) * sizeof(float));
  }
  positions_buffer->add(pos_bytes.data(), pos_bytes.size());

  // dpv fields: append this streamline's (npoints, M) block in native dtype.
  for (SidecarOutput &output : dpv_fields) {
    const size_t elem = output.descriptor.dtype.bytes();
    const size_t cols = output.descriptor.columns;
    std::vector<std::byte> block(npoints * cols * elem, std::byte{0});
    if (item != nullptr && output.ordinal < item->dpv.size()) {
      MR::match_v(item->dpv[output.ordinal], [&](const auto &value) {
        using T = typename std::decay_t<decltype(value)>::element_type;
        for (size_t v = 0; v != npoints && static_cast<Eigen::Index>(v) < value.rows(); ++v)
          for (size_t c = 0; c != cols && static_cast<Eigen::Index>(c) < value.cols(); ++c)
            Raw::store_native<T>(value(static_cast<Eigen::Index>(v), static_cast<Eigen::Index>(c)),
                                 block.data() + (v * cols + c) * elem);
      });
    }
    output.buffer->add(block.data(), block.size());
  }

  // dps fields: append this streamline's single 1×M row in native dtype.
  for (SidecarOutput &output : dps_fields) {
    const size_t elem = output.descriptor.dtype.bytes();
    const size_t cols = output.descriptor.columns;
    std::vector<std::byte> row(cols * elem, std::byte{0});
    if (item != nullptr && output.ordinal < item->dps.size()) {
      MR::match_v(item->dps[output.ordinal], [&](const auto &value) {
        using T = typename std::decay_t<decltype(value)>::element_type;
        for (size_t c = 0; c != cols && static_cast<Eigen::Index>(c) < value.cols(); ++c)
          Raw::store_native<T>(value(0, static_cast<Eigen::Index>(c)), row.data() + c * elem);
      });
    }
    output.buffer->add(row.data(), row.size());
  }

  num_vertices += npoints;
  ++num_streamlines;
  offsets.push_back(num_vertices);
}

template <class ValueType> bool TRXWriter<ValueType>::operator()(const Streamline<ValueType> &tck) {
  append(tck, nullptr);
  return true;
}

template <class ValueType> bool TRXWriter<ValueType>::operator()(const TractogramItem<ValueType> &item) {
  append(item.streamline, &item);
  return true;
}

template <class ValueType> void TRXWriter<ValueType>::write_grouping(const Grouping &g) { grouping = g; }

namespace {

//! \brief slurp a file's bytes into a vector.
std::vector<std::byte> slurp(const std::filesystem::path &path) {
  std::ifstream in(path, std::ios::binary | std::ios::ate);
  std::vector<std::byte> out;
  if (!in)
    return out;
  const std::streamsize size = in.tellg();
  in.seekg(0);
  out.resize(static_cast<size_t>(std::max<std::streamsize>(0, size)));
  if (!out.empty())
    in.read(reinterpret_cast<char *>(out.data()), size);
  return out;
}

//! \brief write a byte buffer to \a path (creating parent directories).
void write_member(const std::filesystem::path &path, const std::byte *data, const size_t size) {
  std::filesystem::create_directories(path.parent_path());
  File::OFStream out(path, std::ios::out | std::ios::binary | std::ios::trunc);
  if (size != 0)
    out.write(reinterpret_cast<const char *>(data), size);
}

} // namespace

template <class ValueType> void TRXWriter<ValueType>::finalise() {
  positions_buffer->commit();
  for (SidecarOutput &output : dps_fields)
    output.buffer->commit();
  for (SidecarOutput &output : dpv_fields)
    output.buffer->commit();

  // Build the JSON header.
  nlohmann::json header;
  nlohmann::json matrix = nlohmann::json::array();
  for (size_t row = 0; row != 4; ++row) {
    nlohmann::json line = nlohmann::json::array();
    for (size_t col = 0; col != 4; ++col) {
      // The AffineCompact matrix stores only the first three rows; the implicit
      //   fourth row is [0 0 0 1].
      const double value = (row == 3) ? ((col == 3) ? 1.0 : 0.0) : voxel_to_rasmm(row, col);
      line.push_back(value);
    }
    matrix.push_back(line);
  }
  header[std::string(TRXUtils::key_voxel_to_rasmm)] = matrix;
  header[std::string(TRXUtils::key_dimensions)] = {dimensions[0], dimensions[1], dimensions[2]};
  header[std::string(TRXUtils::key_nb_streamlines)] = num_streamlines;
  header[std::string(TRXUtils::key_nb_vertices)] = num_vertices;
  const std::string header_text = header.dump(4);

  // Offsets array: uint64, NB_STREAMLINES+1 entries (final == NB_VERTICES).
  std::vector<std::byte> offsets_bytes(offsets.size() * sizeof(uint64_t));
  for (size_t i = 0; i != offsets.size(); ++i)
    Raw::store_LE<uint64_t>(offsets[i], offsets_bytes.data() + i * sizeof(uint64_t));

  const std::vector<std::byte> positions_bytes = slurp(positions_tempfile);

  // Assemble the list of (member-name, bytes) pairs.
  std::vector<std::pair<std::string, std::vector<std::byte>>> members;
  members.emplace_back(
      "header.json",
      std::vector<std::byte>(reinterpret_cast<const std::byte *>(header_text.data()),
                             reinterpret_cast<const std::byte *>(header_text.data()) + header_text.size()));
  members.emplace_back("positions.3.float32", positions_bytes);
  members.emplace_back("offsets.uint64", std::move(offsets_bytes));
  for (SidecarOutput &output : dps_fields) {
    const std::string ext = TRXUtils::extension_from_dtype(output.descriptor.dtype);
    const std::string mid = (output.descriptor.columns == 1) ? "" : "." + str(output.descriptor.columns);
    members.emplace_back("dps/" + output.descriptor.name + mid + "." + ext, slurp(output.tempfile));
  }
  for (SidecarOutput &output : dpv_fields) {
    const std::string ext = TRXUtils::extension_from_dtype(output.descriptor.dtype);
    const std::string mid = (output.descriptor.columns == 1) ? "" : "." + str(output.descriptor.columns);
    members.emplace_back("dpv/" + output.descriptor.name + mid + "." + ext, slurp(output.tempfile));
  }

  // groups: one "groups/<name>.uint32" index table per group (Stage 17). Indices
  //   are written little-endian uint32 (the spec-recommended dtype), preserving
  //   the in-memory order (and any overlap across groups).
  for (const auto &group : grouping.groups()) {
    const std::vector<uint32_t> &indices = group.second;
    std::vector<std::byte> bytes(indices.size() * sizeof(uint32_t));
    for (size_t i = 0; i != indices.size(); ++i)
      Raw::store_LE<uint32_t>(indices[i], bytes.data() + i * sizeof(uint32_t));
    members.emplace_back("groups/" + group.first + ".uint32", std::move(bytes));
  }

  // dpg: one "dpg/<group>/<field>[.M].<dtype>" member per per-group metadatum
  //   (Stage 17), serialised in its native on-disk dtype (D7).
  for (const auto &group : grouping.dpg()) {
    for (const auto &field : group.second) {
      const DataType dtype = MR::match_v(field.second, [](const auto &value) {
        using T = typename std::decay_t<decltype(value)>::element_type;
        return sidecar_datatype<T>();
      });
      const size_t cols =
          MR::match_v(field.second, [](const auto &value) { return static_cast<size_t>(value.cols()); });
      const size_t elem = dtype.bytes();
      std::vector<std::byte> bytes(cols * elem, std::byte{0});
      MR::match_v(field.second, [&](const auto &value) {
        using T = typename std::decay_t<decltype(value)>::element_type;
        for (size_t c = 0; c != cols && static_cast<Eigen::Index>(c) < value.cols(); ++c)
          Raw::store_native<T>(value(0, static_cast<Eigen::Index>(c)), bytes.data() + c * elem);
      });
      const std::string ext = TRXUtils::extension_from_dtype(dtype);
      const std::string mid = (cols == 1) ? "" : "." + str(cols);
      members.emplace_back("dpg/" + group.first + "/" + field.first + mid + "." + ext, std::move(bytes));
    }
  }

  if (TRXUtils::path_is_trx(path) && !path.empty()) {
    // Emit an uncompressed (ZIP_STORE) ".trx" archive via libzip. The overwrite
    //   permission was already resolved at construction (App::check_overwrite).
    std::error_code ec;
    std::filesystem::remove(path, ec);
    int err = 0;
    zip_t *za = zip_open(path.string().c_str(), ZIP_CREATE | ZIP_EXCL, &err);
    if (za == nullptr)
      throw Exception("failed to create TRX archive \"" + path.string() + "\"");
    try {
      for (auto &member : members) {
        zip_source_t *src = zip_source_buffer(za, member.second.data(), member.second.size(), 0);
        if (src == nullptr)
          throw Exception("failed to create zip source for member \"" + member.first + "\"");
        const zip_int64_t idx = zip_file_add(za, member.first.c_str(), src, ZIP_FL_OVERWRITE);
        if (idx < 0) {
          zip_source_free(src);
          throw Exception("failed to add member \"" + member.first + "\" to TRX archive");
        }
        zip_set_file_compression(za, static_cast<zip_uint64_t>(idx), ZIP_CM_STORE, 0);
      }
    } catch (...) {
      zip_discard(za);
      throw;
    }
    if (zip_close(za) != 0)
      throw Exception("failed to finalise TRX archive \"" + path.string() + "\"");
  } else {
    // Emit a TRX directory.
    std::filesystem::create_directories(path);
    for (auto &member : members)
      write_member(path / member.first, member.second.data(), member.second.size());
  }

  // Clean up temp files.
  std::error_code ec;
  std::filesystem::remove(positions_tempfile, ec);
  for (SidecarOutput &output : dps_fields)
    std::filesystem::remove(output.tempfile, ec);
  for (SidecarOutput &output : dpv_fields)
    std::filesystem::remove(output.tempfile, ec);
}

template <class ValueType> TRXWriter<ValueType>::~TRXWriter() {
  try {
    finalise();
  } catch (Exception &e) {
    Exception(e, "TRX tractography dataset not properly finalised").display();
  }
}

/* ************************************************************************ */
/*               Explicit instantiation for float and double              */
/* ************************************************************************ */

template class TRXReader<float>;
template class TRXReader<double>;
template class TRXWriter<float>;
template class TRXWriter<double>;

namespace Formats {

bool TRX::handles(const std::filesystem::path &path) const {
  if (path.extension() == ".trx")
    return true;
  // A directory whose name carries no recognised extension but which contains a
  //   TRX "header.json" is also a TRX dataset.
  std::error_code ec;
  if (std::filesystem::is_directory(path, ec))
    return std::filesystem::exists(path / "header.json", ec);
  return false;
}

std::unique_ptr<ReaderInterface<float>> TRX::read_float(const std::filesystem::path &path,
                                                        Properties &properties,
                                                        FieldRegistry &registry,
                                                        const OptionalHeader &) const {
  return std::make_unique<TRXReader<float>>(path, properties, registry);
}

std::unique_ptr<ReaderInterface<double>> TRX::read_double(const std::filesystem::path &path,
                                                          Properties &properties,
                                                          FieldRegistry &registry,
                                                          const OptionalHeader &) const {
  return std::make_unique<TRXReader<double>>(path, properties, registry);
}

std::unique_ptr<WriterInterface<float>> TRX::create_float(const std::filesystem::path &path,
                                                          const Properties &properties,
                                                          const FieldRegistry &registry,
                                                          const OptionalHeader &,
                                                          const WriteOptions &options) const {
  return std::make_unique<TRXWriter<float>>(path, properties, registry);
}

std::unique_ptr<WriterInterface<double>> TRX::create_double(const std::filesystem::path &path,
                                                            const Properties &properties,
                                                            const FieldRegistry &registry,
                                                            const OptionalHeader &,
                                                            const WriteOptions &options) const {
  return std::make_unique<TRXWriter<double>>(path, properties, registry);
}

} // namespace Formats

} // namespace MR::DWI::Tractography
