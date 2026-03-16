# TRX Integration Plan for MRtrix3

## Design Principles

**Format branching belongs in `trx_utils.h`, not in command code, if possible.**

Commands should not contain `if (is_trx_input)` branches. Any logic that differs between TCK and TRX should preferably be abstracted into a helper in `trx_utils.h` so that the command sees a single unified API. This keeps individual commands maintainable and keeps the integration surface contained.

**Sidecar data resolution: transparent embedded-or-external lookup.**

Commands that accept external sidecar files (e.g., `-tck_weights_in weights.txt`, `-tsf scalars.tsf`) should require no new CLI options for TRX. Instead, `trx_utils.h` provides resolution helpers that:
1. If the argument value is a valid file path, load from that external file as normal (full backward compatibility).
2. If the input tractogram is a TRX file and the argument value is a plain field name (no file extension / not a path that exists on disk), look up that name in `data_per_streamline` (for weight-like data) or `data_per_vertex` (for TSF-like data) and return the values directly.

This means users can write:
```
tckmap input.trx output.mif -tck_weights_in weights      # reads dps["weights"] from TRX
tckmap input.trx output.mif -tck_weights_in weights.txt  # reads external text file
tckmap input.tck output.mif -tck_weights_in weights.txt  # unchanged existing behaviour
```

The option argument type stays `type_file_in()` for external files. The resolution helper does the dispatch; the command code is unchanged.

Concretely, `trx_utils.h` should provide:
- `open_tractogram(path, properties)` → `unique_ptr<ReaderInterface<float>>`: opens TCK or TRX, populates `Properties["count"]`. For TRX, step size is left unset (so `determine_upsample_ratio` naturally returns 1).
- `resolve_dps_weights(trx, field_name_or_path)` → `vector<float>`: returns per-streamline weights from an embedded dps field or an external file.
- `resolve_dpv_scalars(trx, field_name_or_path)` → `vector<float>`: returns flat per-vertex scalars from an embedded dpv field or an external TSF file.
- Any other format-agnostic entry points needed by commands.

The goal is that a command's `run()` function looks virtually identical for TCK and TRX inputs — only output path (`.trx` vs `.tck`) or genuinely new capabilities cause visible branching.

## Core Architectural Insight

MRtrix3's tractography commands all work through `ReaderInterface<float>` / `WriterInterface<float>` and the `Streamline<float>` type. TRX's extra dimensions (dps, dpv, groups, dpg) have **no equivalent abstraction** in MRtrix3 today — data like TSF scalars and SIFT weights live in separate sidecar files.

The smoothest integration strategy is a **two-layer approach**:

1. **Layer 1 (Format layer)**: TRX as a first-class file format wherever TCK is accepted — reading/writing streamline geometry through the existing `ReaderInterface`/`WriterInterface` pattern.

2. **Layer 2 (Metadata layer)**: Per-command opt-in to TRX's richer data model (dps/dpv/groups/dpg), where each command that produces sidecar data gains a `-trx` option to embed that data directly.

## Phase 1: Foundation — Shared TRX Utilities

**File: `cpp/core/dwi/tractography/trx_utils.h`**

### Done

Shared utilities in `MR::DWI::Tractography::TRX` namespace:
- `TRXReader` class — streamline-by-streamline iteration over a TRX file
- `TRXWriter` class — accumulates streamlines into a preallocated TrxFile, with dps/dpv added via `get_trx()`
- `is_trx(path)` — detects `.trx` suffix or TRX directory
- `load_trx(path)` → `unique_ptr<trx::TrxFile<float>>` — handles float16/float32/float64 positions correctly:
  - float32: direct `trx::load<float>()`, zero-copy mmap for positions.
  - float16 or float64: double-load — first pass as float32 (correct for dps/dpv/groups which have their own dtype casting in trx-cpp); second pass via `trx::with_trx_reader` with the native dtype to copy-convert coordinates into `streamlines->_data_owned`; then `trx::detail::remap` rebinds `_data` to the owned buffer before any position access. The float32 pass leaves `_data` temporarily pointing at an undersized mmap but this is never dereferenced until after the remap. A WARN is issued to inform the user of the conversion.
  - Note: the free functions `trx::load_from_directory<DT>` / `trx::load_from_zip<DT>` declared in `trx.h` are never defined; only their `TrxFile<DT>::` static-member counterparts exist in `trx.tpp`. Use `trx::load<float>()` or `trx::with_trx_reader()` instead.
- `count_trx(path)` — returns `(nb_streamlines, nb_vertices)`
- `has_aux_data(trx)` — checks for groups, dps, dpv, or dpg
- `print_info(ostream, trx)` — prints full TRX metadata (header, groups, dps, dpv, dpg)
- `make_typed_array<T>(vector, cols)` — constructs a `trx::TypedArray` from a vector
- `append_dps<T>(path, name, values)` — appends dps to existing TRX (zip or directory)
- `append_dpv<T>(path, name, values)` — appends dpv to existing TRX (zip or directory)
- `TRXScalarReader(path, field_name)` — reads a named dpv field as a stream of `TrackScalar<float>`, one per streamline; eagerly copies data so the mmap is released on construction (enabling in-place append to the same file)
- `TRXScalarWriter(path, field_name)` — accumulates `TrackScalar<float>` values and appends them as a dpv field to an existing TRX on `finalize()`

Infrastructure fixes enabling command-level TRX support without branching (DONE):
- `loader.h`: `TrackLoader` constructor now accepts `ReaderInterface<float>&` instead of `Reader<>&`, so `TRXReader` can be passed directly.
- `mapping.cpp/h`: added `generate_header(Header&, ReaderInterface<float>&, voxel_size)` overload; the path-based version delegates to it.

### Done — helpers for command-level integration

- `open_tractogram(path, properties)` → `unique_ptr<ReaderInterface<float>>`: opens TCK or TRX transparently, populates `properties["count"]`. Single entry point replacing `Reader<float> file(path, properties)` in every command.
- `resolve_dps_weights(tractogram_path, field_name_or_path)` → `vector<float>`: if the string is an existing file path, loads as a text weight vector (tck_weights_in format); otherwise looks up the name in the TRX dps map.
- `resolve_dpv_scalars(tractogram_path, field_name_or_path)` → `vector<float>`: same pattern for per-vertex scalars. File-path case currently handles text format; proper binary TSF support is deferred (use `Tractography::ScalarFile` directly for TSF files for now).

With these three helpers, commands need only:
1. Swap `Reader<float> file(argument[0], properties)` → `auto reader = TRX::open_tractogram(argument[0], properties)` (one line)
2. Swap `TrackLoader loader(file, ...)` → `TrackLoader loader(*reader, ...)` (one line, already works via the `loader.h` fix)
3. Update `have_weights` to check both `tck_weights_in` and `resolve_dps_weights` as needed

## Phase 2: Command-by-Command Integration

### 2a. `tckinfo` — Print TRX metadata (DONE)

When input is TRX, `tckinfo` now:
- Detects TRX files via `TRX::is_trx()`
- Loads via `TRX::load_trx()`
- Prints header info (NB_STREAMLINES, NB_VERTICES, VOXEL_TO_RASMM, DIMENSIONS)
- Lists groups with member counts
- Lists dps fields with dimensions
- Lists dpv fields with dimensions
- Lists dpg fields per group

Also accepts `.trx` files and TRX directories as input arguments (via `type_file_in().type_directory_in()`).

### 2b. `fixel2tsf` → add `-trx_dpv` option (DONE)

Added `-trx_dpv trxpath name` option that appends per-vertex fixel values as a dpv entry to an existing TRX file.

Implementation:
- Added `append_dpv<T>()` utility to `trx_utils.h` (mirrors `append_dps`)
- During the main loop, when `-trx_dpv` is specified, per-vertex scalars are accumulated into a flat `vector<float>`
- After the loop completes, `TRX::append_dpv()` writes the dpv in one shot
- The TSF output (`argument[2]`) is still always produced; `-trx_dpv` is an additional output

Note: under the new design principle (no new CLI options; resolve from embedded TRX data transparently), the `-trx_dpv` option style is legacy. The write direction (appending dpv to an existing TRX) is still a valid pattern since there is no other way to embed new data. The read direction (consuming dpv instead of TSF) should use `resolve_dpv_scalars` once that helper exists.

### 2c. `tcksift2` → add `-trx_dps` option (DONE)

Added `-trx_dps trxpath name` option that appends SIFT2 weights as a dps entry to an existing TRX file.

Implementation:
- Added `get_factors()` method to `TckFactor` (returns `std::vector<float>` of exponentiated weights)
- Added `make_typed_array<T>()` and `append_dps<T>()` utilities to `trx_utils.h`
- Uses `trx::append_dps_to_zip()` or `trx::append_dps_to_directory()` (auto-detected) to append without rewriting
- The text weight output (`argument[2]`) is still always produced; `-trx_dps` is an additional output

### 2d. `trxlabel` + `trx2connectome` → TRX-native connectome pipeline (DONE)

Two new commands replace the `tck2connectome` workflow for TRX files.

#### `trxlabel` — assign streamlines to groups by parcellation atlas

**Usage**: `trxlabel tracks_in.trx nodes.mif tracks_out.trx [-lut lut.txt] [-prefix str] [assignment options]`

Assigns each streamline to one or two TRX groups based on the parcellation nodes touched by its endpoints. Uses the same `Tck2nodes_base` assignment machinery as `tck2connectome` (default: 4 mm radial search). Each node that receives at least one endpoint assignment becomes a group; the group name is the node index or the LUT name if `-lut` is provided.

- The output TRX can be the same file as the input (in-place labeling via `append_groups_to_zip/directory`).
- `-prefix str` namespaces groups from a given atlas (e.g. `-prefix dk` → groups named `dk_Left-Hippocampus`). Run multiple times with different atlases and prefixes to accumulate groups from multiple parcellation schemes.
- Supports all assignment options from `tck2connectome` (`-assignment_radial_search`, `-assignment_end_voxels`, etc.).
- Supports all LUT formats (FreeSurfer, AAL, ITK-SNAP, MRtrix).

Implementation:
- `load_trx()` + direct position iteration replaces `TRXReader` so the TrxFile can be saved before closing
- After computing assignments, `trx->save(output_path)` copies the full TRX (including any existing groups/dps/dpv)
- `trx->close(); trx.reset()` releases the mmap before appending
- `append_groups_to_zip/directory()` adds the new groups to the saved file

#### `trx2connectome` — build connectome matrix from TRX groups

**Usage**: `trx2connectome tracks_labeled.trx connectome.csv [-tck_weights_in weights] [-out_node_names names.txt] [-group_prefix prefix]`

Reads TRX groups, inverts the group→index mapping to build a per-streamline node pair list, then feeds these into the existing `Matrix<T>` accumulator. No parcellation image is needed — the assignments are already embedded.

- `-tck_weights_in` accepts either an external text file or a TRX dps field name (via `resolve_dps_weights`).
- `-group_prefix` restricts the matrix to groups matching a given prefix (useful when multiple atlases were labeled into the same TRX).
- `-out_node_names path` writes the alphabetically-ordered group names (one per line) corresponding to matrix rows/columns.
- Supports all `Matrix<T>` options: `-stat_edge`, `-symmetric`, `-zero_diagonal`, `-keep_unassigned`.
- Streamlines in exactly 2 groups contribute to one off-diagonal entry; streamlines in 3+ groups contribute to all unique pairs; streamlines in 0 or 1 groups are accumulated at the unassigned (0,0) or (0,node) cells (discarded by default).


### 2e. `connectome2tck` → read groups/dps from TRX (DONE)

When input is TRX, passing `"-"` as `assignments_in` triggers TRX group mode: node
assignments are derived from the embedded group membership (as written by `trxlabel`)
instead of a separate text file.  Existing text-file mode is fully preserved.

**CLI change**: `assignments_in` changed from `type_file_in()` to `type_text()` so the
argument is not validated as a file at parse time.  The `run()` function dispatches:
- `assignments_arg == "-"` or `is_trx(assignments_arg)` with TRX input → group mode
- otherwise → text file mode (validates file existence itself)

**New option**: `-group_prefix prefix` — filters to groups whose name starts with
`prefix_` (same semantics as in `trx2connectome`).  Useful when a TRX file has groups
from multiple atlases; has no effect in text-file mode.

**`assignments_from_trx_groups()` static helper** (before `run()`):
- Loads TRX, collects groups matching the prefix
- Tries to parse group names as integers after stripping the prefix — succeeds for the
  common case of `trxlabel` without `-lut`, so `-nodes 1,2` refers to the same atlas
  parcels as in the TCK/tck2connectome workflow
- Falls back to alphabetical 1-based ordering for LUT-based names (matches `trx2connectome`)
- Inverts the group → streamline mapping to produce per-streamline node lists, sorted
- Returns `vector<vector<node_t>>` compatible with the rest of `run()` unchanged

**Reader change**: `Reader<float> reader(path, properties)` replaced by
`open_tractogram(path, properties)` → `unique_ptr<ReaderInterface<float>>`. All four
call sites changed from `reader(out)` to `(*reader)(out)`.  The polymorphic reader
handles both TCK and TRX geometry transparently; `properties["count"]` is populated
correctly for both formats.

**What is NOT changed**: the output format stays as TCK files.  Writing per-edge TRX
groups in a single file would produce O(n²) groups for typical parcellations, which the
design doc explicitly discourages.  The extraction logic (pair optimisation,
`-nodes`/`-exclusive`/`-keep_self`, exemplar generation) is identical for both
input modes once `assignments_lists` is populated.

Typical workflow:
```
trxlabel tracks.trx nodes.mif labeled.trx
connectome2tck labeled.trx - edge-                      # derive from groups
connectome2tck labeled.trx - edge- -group_prefix dk     # one atlas from multi-atlas TRX
connectome2tck labeled.trx assignments.txt edge-        # still works with text file
```

### 2f. `tcksample` → add `-trx_dpv` and `-trx_dps` options (DONE — write pattern is correct)

Added `-trx_dpv trxpath name` and `-trx_dps trxpath name` options.

Implementation:
- **DPV** (`-trx_dpv`): When no `-stat_tck` is used (per-vertex output mode), `Receiver_NoStatistic` optionally accumulates all per-vertex scalars into a flat `vector<float>`. After the threaded queue completes, `TRX::append_dpv()` writes the dpv in one shot.
- **DPS** (`-trx_dps`): When `-stat_tck` is specified (per-streamline statistics mode), `Receiver_Statistic` exposes the result vector via `append_to_trx()`. After saving the text output, `TRX::append_dps()` appends the stats as a dps field.
- Both options validate the TRX path via `TRX::is_trx()` before running.
- The normal output (`argument[2]`) is always produced; the TRX options are additional outputs.

### 2g. `tcksift` → TRX-aware subsetting (DONE)

When input is TRX and output is `.trx`, `tcksift` now uses `trx::TrxFile::subset_streamlines()` to produce a metadata-preserving filtered TRX output (dps/dpv/groups all remapped to the surviving streamlines).

Implementation:
- `TRXReader::operator()` now calls `tck.set_index(current)` so the SIFT model's `contributions` array is indexed correctly
- `TRXReader::num_streamlines()` accessor added
- `Model<Fixel>::map_streamlines()` detects TRX input and uses `TRXReader` directly (no `TrackLoader` which requires `Reader<>`)
- `SIFTer::get_selected_indices()` returns `std::vector<uint32_t>` of kept streamline indices from `contributions`
- `SIFTer::output_filtered_tracks()` handles TRX input path (for TRX-in + TCK-out, e.g. `-output_at_counts`)
- `tcksift.cpp`: argument[0] accepts `.trx`/directory; argument[2] accepts `.trx`; when output is TRX, calls `load_trx` → `get_selected_indices` → `subset_streamlines` → `save`; otherwise calls existing `output_filtered_tracks`

### 2h. `tckstats` → add `-trx_dps` for lengths (DONE)

Implementation:
- `Reader<float>` replaced by `open_tractogram(argument[0], properties, weight_src)` — handles both TCK and TRX; dps weights injected automatically when `-tck_weights_in <field>` is used.
- `dump` vector moved outside the reader scope block so it is available for `-trx_dps` output after the reader closes.
- Added `-trx_dps path name` option: after the main loop, calls `append_dps<float>(path, name, dump)` to embed per-streamline lengths as a dps field. NaN values are stored for empty/degenerate streamlines, matching the `-dump` text output.
- `#include "dwi/tractography/file.h"` removed; `trx_utils.h` included instead.
- `weights_provided` check remains `!get_options("tck_weights_in").empty()` — correct for both TCK and TRX since the user must explicitly name a weight source.

### 2i. `tckgen` → streaming TRX output (DONE)

`tckgen` now writes `.trx` output natively via `trx::TrxStream`.

Implementation (zero changes to `tckgen.cpp`):
- `AbstractTrackWriter` interface added to `write_kernel.h`: owns `count`/`total_count`, `operator()(Streamline<float>)`, `skip()`.
- `WriteKernel::writer` changed from `Writer<float>` (concrete) to `unique_ptr<AbstractTrackWriter>`.
- `TCKWriterAdapter` wraps `Writer<float>` behind the interface.
- `TRXWriterAdapter` holds a `trx::TrxStream`; calls `push_streamline()` per track; calls `stream.finalize<float>(path)` in its destructor.
- `WriteKernel::create_writer()` static factory detects format from path (`.trx` suffix or `TRX::is_trx()`) and returns the appropriate adapter.
- `dynamic.cpp`: `writer.count` → `writer->count` (the only other file that touched `WriteKernel::writer` directly).
- `core/CMakeLists.txt`: added `trx-cpp::trx` to `mrtrix-core` public link libraries.
- `cmake/Dependencies.cmake`: FetchContent for trx-cpp uses local `file://${PROJECT_SOURCE_DIR}/../trx-cpp` at `GIT_TAG HEAD`.
- Thread safety: `TrxStream::push_streamline()` is called only from the single writer thread — matches `WriteKernel`'s existing design.
- **`-trx_float16` flag**: sets `properties["trx_positions_dtype"] = "float16"` in `tckgen::run()`. `WriteKernel::create_writer()` reads this property and passes it to `TRXWriterAdapter`, which forwards it to `trx::TrxStream(positions_dtype)`. Default is `"float32"` (MRtrix's internal precision); float16 is opt-in because it silently quantizes coordinates to ~0.02–0.05 mm precision.
- **`app.cpp` TracksIn/TracksOut validation**: relaxed from `.tck`-only to `.tck` or `.trx`, enabling tckgen to accept `.trx` as a valid output argument without an error at argument-parsing time.

## Phase 3: Geometry I/O — Accept TRX wherever TCK is accepted

These commands modify or pass through streamline geometry. TRX support means accepting `.trx` input and producing `.trx` output while preserving metadata where appropriate.

### 3a. `tckedit` — metadata-aware filtering (DONE)

When input is TRX and output is `.trx`, `tckedit` uses `trx::TrxFile::subset_streamlines()` to produce a metadata-preserving filtered TRX output (dps/dpv/groups all remapped to the surviving streamlines).

Implementation:
- Added `TRXIndexCollector` class (local to `tckedit.cpp`): receives passing streamlines from the existing `Worker`, records their original TRX index, respects `-number` and `-skip` limits
- `run()` detects TRX mode (`trx_in` and `trx_out`), errors for mixed TCK/TRX or multiple TRX inputs
- Properties/ROI setup is shared with TCK path; count comes from `TRX::count_trx()` instead of `Reader<>`
- `-mask` uses `TrxStream` instead of `subset_streamlines`: a bare `trx::TrxStream` is created (no affine/dimensions — TRX coordinates are already RAS+, the same as TCK, so `VOXEL_TO_RASMM` is never used in MRtrix), cropped geometry is pushed segment-by-segment, and `stream.finalize()` writes the output; a warning is emitted if the input had dps/dpv/groups (they cannot be remapped after vertex-level cropping)
- After the filter pipeline completes, calls `load_trx` → `subset_streamlines(collected_indices)` → `save`
- All existing TCK-mode behaviour is unchanged

### 3b. `tcktransform` — geometry transform with metadata preservation (DONE)

Implementation:
- `Loader` class: `Reader<value_type>` replaced with `unique_ptr<ReaderInterface<value_type>>` from `open_tractogram`. Handles TCK and TRX inputs uniformly.
- `Writer` class: extended to support TRX output alongside TCK. Constructor detects `is_trx(file)` and either creates a `trx::TrxStream` (TRX path) or `Tractography::Writer<value_type>` (TCK path). TrxStream is finalised in the destructor with `finalize(path, TrxSaveOptions{})`. `operator()` converts `Streamline<float>` to `vector<array<float,3>>` as required by `push_streamline()`.
- `Warper::pos()` promoted from `protected` to `public` so `run()` can call it directly for the in-place TRX path.
- `run()` branches on input/output format:

**TRX→TRX in-place path**: detects `is_trx(arg[0]) && is_trx(arg[2])`, loads the full TrxFile with `load_trx()`, iterates the flat `streamlines->_data` position matrix directly (same access pattern as TRXReader), calls `warper.pos()` per vertex and writes back into `_data`. Vertices outside the warp field (where `pos()` returns NaN) are left at their original coordinates with a WARN. After the loop, `trx->save(argument[2])` writes the output — because we modified the in-memory TrxFile, **all metadata (dps, dpv, groups, dpg) is preserved automatically with no extra infrastructure**.

**Stream path (all other combinations)**: TCK→TCK, TCK→TRX, TRX→TCK. Uses `Loader` + `Warper` + `Writer` via `Thread::run_ordered_queue`. TRX output in this path carries no metadata from a TCK input (there is none), and no metadata from a TRX input (use the TRX→TRX path instead if metadata preservation matters).

**Package-level observation**: The in-place approach requires direct access to `trx->streamlines->_data(v, 0/1/2)` and `_offsets(s, 0)` — the same pattern used in `TRXReader`. A helper `for_each_vertex(trx, fn)` in `trx_utils.h` that abstracts this access pattern would make future commands that need to iterate or modify TRX positions (e.g., `tckresample`) cleaner and less coupled to trx-cpp internals.

### 3c. `tckresample` — vertex count change (DONE)

**Effort**: Medium. **Value**: Medium.

Current flow: resamples each streamline to a new vertex set via one of several strategies (upsample ratio, downsample ratio, fixed point count, fixed step size, endpoints only, arc). Pure streaming: `Reader` → `Worker` (resampler) → `Receiver`/`Writer`.

**What changes and why**:

Because resampling changes vertex count, TRX→TRX cannot use the tcktransform in-place approach. Geometry must be streamed through `TrxStream` (same pattern as the tcktransform stream path). Streamline count is unchanged, so dps and groups can be preserved by copying them from the source TRX after the stream completes.

**Command-level changes** (all small):
- Input arg: add `.type_directory_in()`
- `run()`: replace `Reader<value_type>` with `open_tractogram` (2-arg, no weights here)
- `Receiver`: detect TRX output, use `TrxStream` in place of `Writer<value_type>` (same pattern as tcktransform `Writer` class). After `TrxStream` finalises, copy sidecar data.

**Metadata policy**:
- **dps** (per-streamline): streamline count unchanged → copy all fields from source TRX to output TRX via `append_dps` after streaming
- **groups** (per-streamline index lists): streamline indices unchanged → copy all groups from source TRX to output TRX
- **dpg** (per-group scalars): no geometry dependency → copy
- **dpv** (per-vertex): vertex count changes → **discard with WARN** if any dpv fields exist

**Note on dpv discard**: `tckresample` has no TSF resampling capability — the `Resampling::Base` interface is `operator()(const Streamline<>&, Streamline<>&)`, geometry-only. There is no scalar sidecar resampling anywhere in the resampling subsystem, and no existing mechanism in the TCK ecosystem (TSF files are simply incompatible with a resampled track file). Discarding dpv is therefore parity with the TCK+TSF workflow, not a regression. A future `tckresample -tsf_in/-tsf_out` feature would need to extend `Resampling::Base` to also resample a scalar vector alongside each streamline; the same extension would then make dpv resampling natural.

**Package-level requirement — `copy_trx_sidecar_data(src_path, dst_path, bool include_dpv)`**:

This utility does not yet exist in `trx_utils.h`. It would:
1. Load source TRX with `load_trx()`
2. For each dps field: call `append_dps<float>(dst_path, name, values)`
3. For each group: append the group (streamline index list) to dst TRX — requires exposing a `append_group(dst_path, name, indices)` utility (analogous to `append_dps` but for groups)
4. For each dpg field on copied groups: append alongside the group
5. If `include_dpv == true`: also copy dpv fields (not used by tckresample)
6. Warn if any dpv fields were present but not copied

Both `copy_trx_sidecar_data` and `append_group` have been added to `trx_utils.h`.  The tckresample implementation emits a WARN listing the count of dpv fields that will be discarded, then calls `copy_trx_sidecar_data(in, out, false)` after the pipeline to copy dps and groups.

### 3d. `tckmap` — consume dps/dpv for weighted imaging (DONE)

`tckmap` now accepts TRX input transparently. No new CLI options were needed.

Implementation:
- `Reader<float> file(argument[0], properties)` replaced by:
  ```cpp
  auto wt_opt = get_options("tck_weights_in");
  const std::string weight_src = wt_opt.empty() ? "" : std::string(wt_opt[0][0]);
  auto reader = open_tractogram(argument[0], properties, weight_src);
  ```
  The 3-arg `open_tractogram` (from `trx_utils.h`) dispatches: TCK → `Reader<float>` (handles weights itself); TRX → `TRXReader` with `resolve_dps_weights` applied at construction, injecting `tck.weight` during iteration.
- `TrackLoader loader(file, num_tracks)` → `TrackLoader loader(*reader, num_tracks)` — already works via the `loader.h` `ReaderInterface<float>&` fix.
- `generate_header(header, argument[0], voxel_size)` — the string-path overload was already fixed in `mapping.cpp` to open its own reader internally, so the caller's `reader` is never consumed by the header-generation pass.
- `determine_upsample_ratio(header, properties, ratio)` naturally returns 1 for TRX (no step size in properties). No branching needed.
- `#include "dwi/tractography/file.h"` removed (no longer directly used).

Transparent weight resolution: `-tck_weights_in weights` works identically whether `weights` is an external text file path or a TRX dps field name — `resolve_dps_weights` handles both cases.


## Phase 4: mrview Integration

This is the largest effort but also the most user-visible payoff.

### 4a. Basic TRX loading and group panel (DONE)

TRX loading already works via `TRXReader` in `Tractogram::load_tracks()`.

**Group panel** (`TrackGroupOptions` widget, new files `track_group_options.h/.cpp`):

A `QGroupBox` ("TRX Groups") shown below the scalar file options whenever a TRX tractogram with groups is selected.  The panel stays hidden for TCK files and TRX files with no groups.

UI layout:
- **Multi-group** combobox: "First match" / "Last match" (controls which group's color wins when a streamline belongs to multiple groups)
- **Scrollable group list** (max 160 px, ~8 rows): one row per group — checkbox (name + streamline count) + 20×20 color swatch button
- **"Show ungrouped"** checkbox: whether to render streamlines that belong to no group

Data model (stored in `Tractogram`):
- `GroupState` struct: `visible`, `color` (Eigen::Vector3f), `count`
- `group_states`: `std::map<std::string, GroupState>` — keyed by group name
- `group_order`: `std::vector<std::string>` — determines color priority for overlapping groups
- `show_ungrouped`: bool
- `group_multi_policy`: `GroupMultiPolicy` enum (`FirstMatch` / `LastMatch`)

The colour buffer (GPU) is rebuilt on any change via `Tractogram::reload_group_colours()`, which:
1. Initialises `scolours[ns]` to `HIDDEN = (-1,-1,-1)` (a sentinel never produced by valid RGB)
2. For ungrouped streamlines (not in any group), assigns `UNGROUPED = (0.3,0.3,0.3)` when `show_ungrouped` is true
3. Iterates `group_order` (forward for FirstMatch, reverse for LastMatch); only visible groups participate; a `claimed[]` bool prevents later groups from overwriting earlier winners
4. Uploads per-streamline colours in exactly the same way as `load_end_colours()`

The fragment shader discards fragments with `colour.r < 0.0` (the sentinel) when in Group mode, so hidden-group streamlines produce no pixels without needing a second render pass.

`Tractogram::init_group_states()` is called lazily (first time the panel is shown or "TRX Groups" colour mode is selected) and assigns palette colours in order.

`Tractography::update_scalar_options()` calls `group_options->set_tractogram(t)` / `update_UI()` alongside `scalar_file_options`, so the panel appears/disappears with selection changes.

### 4b. DPS/DPV coloring (DONE)

Coloring modes `ScalarFile` now covers TRX embedded fields via the "TRX field…" button in `TrackScalarFileOptions`.  Both dps (per-streamline) and dpv (per-vertex) fields are accessible.

### 4c. Future: dpg colors, group reordering drag-and-drop, dpv threshold source

- When a TRX file has a `dpg["color"]` field, use those colors as defaults in `init_group_states()` instead of the palette
- Add drag-to-reorder rows in the group panel to change `group_order` interactively
- The threshold combobox currently only supports "Separate scalar file" (TSF); a "TRX dpv field" threshold source would allow using an embedded binary mask (e.g. created by `tsfthreshold`) directly as a display threshold without first exporting to TSF

## Phase 5: Connectivity & Fixel Commands

### 5a. `tck2fixel` — fixel TDI from TRX input (DONE)

**Effort**: Very small. **Value**: Medium.

Current flow: opens `Reader<float>` from the tracks argument, creates `TrackLoader` + `TrackMapperBase` + `TrackProcessor`, accumulates per-fixel streamline counts.

**Command-level changes**:
- Input arg already has `.type_tracks_in()` — add `.type_directory_in()`
- Add `#include "dwi/tractography/trx_utils.h"` and `using namespace TRX`
- Replace `DWI::Tractography::Reader<float> track_file(track_filename, properties)` with `auto reader = open_tractogram(track_filename, properties)` (2-arg, no weights)
- Change `TrackLoader loader(track_file, num_tracks, ...)` → `TrackLoader loader(*reader, num_tracks, ...)`
- Remove `track_file.close()` (RAII via unique_ptr)
- `determine_upsample_ratio(index_header, properties, 0.333f)` is already TRX-safe (returns 1 when no step size in properties)

**No new CLI options needed.** TRX groups are not consumed here — the command produces a fixel TDI regardless of group structure.

### 5b. `tckdfc` — TRX input for dynamic functional connectivity (DONE)

**Effort**: Small-Medium (4 Reader instantiation sites, all mechanical). **Value**: Medium.

Current flow: complex multi-pass algorithm that re-opens the track file for each fMRI timepoint. A properties-only probe is done in a scope block first, then the static path makes one pass, and the dynamic path makes up to N+1 passes (TDI counts + one per timepoint).

**Pre-existing bug to fix while here**: The tracks argument uses `.type_file_in()` instead of `.type_tracks_in()`. Change to `.type_tracks_in().type_directory_in()`.

**All 6 Reader call sites** follow the same pattern — replace each with `auto reader = open_tractogram(tck_path, properties)` and `TrackLoader loader(*reader, ...)`:
1. Properties probe: `{ Tractography::Reader<float> tck_file(tck_path, properties); }` → `{ auto r = open_tractogram(tck_path, properties); }`
2. Static path reader + TrackLoader
3. Dynamic path count-pass reader + TrackLoader
4. Dynamic path per-timepoint reader + TrackLoader (called inside a for loop — N times)

`generate_header(header, argument[0], voxel_size)` and `determine_upsample_ratio(header, properties, ...)` are already TRX-safe.

**No new CLI options needed.** tckdfc doesn't use per-streamline weights or sidecar data — it only needs geometry.

**Multi-pass note**: Each `open_tractogram` re-loads the TRX (re-opens the zip or re-mmaps the directory). For large TRX files in dynamic mode this may be slower than TCK re-opening, but the bottleneck is the fMRI correlation computation not the file open. No caching infrastructure is needed.

### 5c. `fixelconnectivity` — fixel-fixel connectivity matrix from TRX input (DONE)

**Effort**: Small-Medium. **Value**: High (dps weights from TRX eliminates need for separate SIFT2 weight files).

Current flow: delegates to `Fixel::Matrix::generate_unweighted(argument[1], ...)` or `generate_weighted(argument[1], ...)`. Both functions are implemented via the `FIXEL_MATRIX_GENERATE_SHARED` macro in `fixel/matrix.cpp`:

```cpp
#define FIXEL_MATRIX_GENERATE_SHARED                        \
  ...                                                        \
  DWI::Tractography::Reader<float> track_file(track_filename, properties); \
  ...                                                        \
  DWI::Tractography::Mapping::TrackLoader loader(track_file, num_tracks, ...);
```

**Library change required** (`fixel/matrix.cpp`):
1. Add `#include "dwi/tractography/trx_utils.h"` and `using namespace MR::DWI::Tractography::TRX`
2. Update `FIXEL_MATRIX_GENERATE_SHARED` to use `open_tractogram`:
   ```cpp
   auto wt_opt = MR::App::get_options("tck_weights_in");
   const std::string weight_src = wt_opt.empty() ? "" : std::string(wt_opt[0][0]);
   auto track_reader = open_tractogram(track_filename, properties, weight_src);
   const uint32_t num_tracks = ...;
   DWI::Tractography::Mapping::TrackLoader loader(*track_reader, num_tracks, ...);
   ```
   The weight source is resolved from the option at this level because the macro is the only place the reader is constructed. For the unweighted case `weight_src` will be empty and `open_tractogram` 3-arg passes through to the 2-arg version.

**Command-level change**: add `.type_directory_in()` to the `argument[1]` tracks argument. `TrackWeightsInOption` already uses `type_text()`.

**Why the macro is the right place**: `generate_unweighted` and `generate_weighted` differ only in the matrix type; the reader construction is identical. Updating the macro updates both in one place. Alternatively the macro could be replaced with a template helper function, which would be cleaner but is a larger refactor.

### 5d. `afdconnectivity` — AFD-based connectivity from TRX input (DONE)

**Effort**: Small. **Value**: Medium.

Current flow: `AFDConnectivity` (extends `SIFT::ModelBase<AFDConnFixel>`) does two things with track files:
1. **`map_streamlines(wbft_path)`** — called in constructor when `-wbft` is provided; this calls `ModelBase::map_streamlines` which uses `Reader<>` internally
2. **`AFDConnectivity::get(path)`** — opens `Reader<value_type>` directly and creates `TrackLoader`

**Important discovery**: `SIFT::Model::map_streamlines` already overrides the base class with TRX support (it has `if (TRX::is_trx(path))` branch). However, `AFDConnectivity` inherits from `ModelBase`, not `Model`, so it uses the unpatched base class version.

**Changes needed**:

1. **Command** (`afdconnectivity.cpp`):
   - Add `.type_directory_in()` to `argument[1]` (pathway tracks, already has `.type_tracks_in()`)
   - Add `.type_directory_in()` to the `-wbft` option argument (already has `.type_tracks_in()`)
   - In `AFDConnectivity::get(path)`: replace `Reader<value_type> reader(path, properties)` with `auto reader = open_tractogram(path, properties)`; change `TrackLoader loader(reader, ...)` → `TrackLoader loader(*reader, ...)`
   - Add `#include "dwi/tractography/trx_utils.h"` and `using namespace TRX`

2. **Library** (`SIFT/model_base.h`, `map_streamlines` template method):
   - Update to use `open_tractogram` instead of `Reader<>` — same change as done in `SIFT::Model::map_streamlines`
   - OR: Since `Model::map_streamlines` already handles TRX, consider whether `AFDConnectivity` could inherit from `Model` instead of `ModelBase`. This would be a larger refactor but would avoid the duplication between `Model::map_streamlines` and `ModelBase::map_streamlines`.
   - Recommended minimal fix: update `ModelBase::map_streamlines` to use `open_tractogram`, removing the duplication now present between base and derived class.

`determine_upsample_ratio(fod_buffer, tck_path, 0.1)` uses the string-path overload which is already TRX-safe.

## Phase 6: Convenience & Polish

- **`tckconvert`**: Already done, but add TrxStream path to avoid the pre-counting second pass for non-TRX→TRX conversions.
- **TSF utilities** (`tsfinfo`, `tsfvalidate`, `tsfdivide`, `tsfmult`, `tsfsmooth`, `tsfthreshold`): Can now operate on TRX dpv entries (DONE). Each command accepts TRX files as input/output alongside `.tsf` files, using `-field_in`/`-field_out`/`-field1`/`-field2` options to name the dpv fields.

## Complete Command Inventory

### High Value — Produce/consume sidecar data mapping to TRX dps/dpv/groups

| Command | Current sidecar data | TRX mapping | Status |
|---------|---------------------|-------------|--------|
| **tckinfo** | — | Print TRX metadata | DONE |
| **tckconvert** | DPS/DPV/DPG passthrough | Full TRX I/O | DONE |
| **tcksift2** | Text weights file | Weights → dps | DONE |
| **fixel2tsf** | TSF (per-vertex) | Fixel values → dpv | DONE |
| **tcksample** | TSF or per-streamline stats | Per-vertex → dpv, stats → dps | DONE |
| **trxlabel** | — (new command) | Assigns streamlines to groups by parcellation atlas | DONE |
| **trx2connectome** | — (new command) | Builds connectome matrix from TRX groups | DONE |
| **connectome2tck** | Reads assignments, extracts per-edge TCKs | Derive assignments from TRX groups via `"-"` arg; output stays TCK | DONE |
| **tcksift** | Filtered TCK + optional selection text | TRX-in + TRX-out via `subset_streamlines`; dps/dpv/groups preserved | DONE |
| **tckstats** | Per-streamline lengths (optional dump) | Lengths → dps | DONE |

### Medium Value — Geometry I/O with metadata handling

| Command | Notes | Status |
|---------|-------|--------|
| **tckedit** | Filter/concatenate with metadata-aware subsetting of dps/dpv/groups | DONE |
| **tckmap** | Consume dps/dpv for weighted track-density imaging | DONE |
| **tcktransform** | Transform geometry, preserve all metadata unchanged | DONE |
| **tckresample** | Preserve dps/groups; dpv needs interpolation or discard | DONE |
| **tckgen** | TrxStream writer for direct TRX generation | DONE |
| **tck2fixel** | Accept TRX input; geometry-only (no group awareness) | DONE |
| **tckdfc** | Accept TRX input; multi-pass re-open pattern (4 sites) | DONE |
| **fixelconnectivity** | Accept TRX input; dps weights via `resolve_dps_weights` in library macro | DONE |
| **afdconnectivity** | Accept TRX input; fix `ModelBase::map_streamlines` | DONE |

### Low Value — TSF utilities and inspection

| Command | Notes | Status |
|---------|-------|--------|
| **tsfinfo** | Lists TRX dpv fields; `-field` for -ascii export | DONE |
| **tsfvalidate** | Validates TRX dpv vertex count with `-field` | DONE |
| **tsfdivide/tsfmult** | Arithmetic on dpv entries via `-field1`/`-field2`/`-field_out` | DONE |
| **tsfsmooth** | Smooth dpv along streamlines via `-field_in`/`-field_out` | DONE |
| **tsfthreshold** | Threshold dpv to binary mask via `-field_in`/`-field_out` | DONE |
| **tckglobal** | Generation only; output format swap | TODO |

## Recommended Implementation Order

| Priority | Task | What | Status |
|----------|------|------|--------|
| 1 | **Foundation** | `trx_utils.h` shared utilities | DONE |
| 2 | **Infrastructure** | `loader.h` accepts `ReaderInterface<float>&`; `mapping.cpp` `generate_header` overload | DONE |
| 3 | **Foundation** | `open_tractogram()`, `resolve_dps_weights()`, `resolve_dpv_scalars()` in `trx_utils.h` | DONE |
| 4 | **tckinfo** | Print TRX metadata | DONE |
| 5 | **tcksift2** | `-trx_dps` for weights | DONE |
| 6 | **fixel2tsf** | `-trx_dpv` for fixel data | DONE |
| 7 | **tcksample** | `-trx_dpv` and `-trx_dps` for sampled values | DONE |
| 8 | **tcksift** | TRX-aware subsetting via `subset_streamlines` | DONE |
| 9 | **tckedit** | Metadata-aware TRX filtering | DONE |
| 10 | **tckmap** | `open_tractogram` + `resolve_dps_weights`; no new CLI args | DONE |
| 11 | **trxlabel + trx2connectome** | new TRX-native connectome pipeline | DONE |
| 12 | **connectome2tck** | Read groups/dps from TRX | DONE |
| 13 | **tckstats** | lengths as dps via `append_dps` | DONE |
| 14 | **tckgen** | TrxStream writer | DONE |
| 15 | **tcktransform** | Geometry transform + metadata preservation | DONE |
| 16 | **mrview basic** | Load TRX, group panel with per-group visibility/color | DONE |
| 17 | **mrview advanced** | dpg-driven default colors, drag-to-reorder group priority | TODO |

## Testing Strategy

### How MRtrix tests work

Tests live in `testing/binaries/tests/{command}/` as plain bash scripts. Each script runs with its working directory set to the root of the binary test data repository (`mrtrix3/test_data`, fetched at CMake time via ExternalProject_Add). Temporary output files must be prefixed `tmp` — CMake cleans them up automatically. The key comparison tools are:

- `testing_diff_tck tck1 tck2 [-distance mm] [-maxfail n] [-unordered]` — symmetric Hausdorff distance between streamline pairs
- `testing_diff_tsf tsf1 tsf2 [-frac f] [-abs a]` — per-vertex scalar file comparison
- `testing_diff_matrix mat1 mat2 [-frac f] [-abs a]` — CSV / matrix comparison

A new command `testing_diff_trx` does not yet exist; see below.

### Core challenge: test data lives in a separate repo

New test data files (`.trx` inputs or reference outputs) must be committed to the `mrtrix3/test_data` repository, and the `GIT_TAG` in `testing/CMakeLists.txt` must be updated to point to the new commit. Until then, any test that requires those files will fail during CI.

**The workaround that avoids this for most tests**: construct TRX inputs inside the test script itself using `tckconvert` on existing `.tck` data (already in the test data repo), then verify output by converting back to `.tck` with `tckconvert` and running `testing_diff_tck`. This sidesteps the need for pre-generated `.trx` reference files in many cases.

### What needs to go into the test data repo

The following files need to be added to `mrtrix3/test_data` before the corresponding tests can be written:

| File | Generated by | Used by |
|------|-------------|---------|
| `SIFT_phantom/tracks.trx` | `tckconvert SIFT_phantom/tracks.tck SIFT_phantom/tracks.trx` | tcksift, tcksift2, tckedit, tckgen TRX tests |
| `tcksample/fa_trx_dpv.trx` | reference dpv output | tcksample `-trx_dpv` test |
| `tcksample/fa_trx_dps.trx` | reference dps output | tcksample `-trx_dps` test |
| `fixel2tsf/tracks.trx` | `tckconvert tracks.tck fixel2tsf/tracks.trx` + append dpv | fixel2tsf `-trx_dpv` test |

For now, the simplest path is to **generate these files once manually** (in a local build), commit them to a branch of `test_data`, and then update `GIT_TAG` in `testing/CMakeLists.txt`.

### Per-command test plan

#### `tckconvert` (tests already exist)
- `trx_read`: TRX zip → TCK, compare with `testing_diff_tck` ✓
- `trx_uncompressed_read`: TRX directory → TCK ✓
- `trx_write_dps_float{16,32,64}`: TCK → TRX with dps, verify file presence ✓
- **New**: `trx_roundtrip` — TCK → TRX → TCK, compare with `testing_diff_tck -distance 1e-5`
- **New**: `trx_float16_roundtrip` — TCK → TRX float16 → TCK, compare with `testing_diff_tck -distance 0.1` (looser tolerance for float16 quantization)

#### `tckinfo`
- **New**: `trx_metadata` — `tckinfo gs.trx` and check exit code + presence of expected header strings (NB_STREAMLINES, NB_VERTICES) in output via pipe to `grep`

#### `tckgen`
- **New**: `trx_output` — generate 100 streamlines to `tmp.trx`, convert back to TCK and count; verify count matches (no geometry comparison since probabilistic)
  ```bash
  tckgen SIFT_phantom/fods.mif -algo ifod2 -seed_image SIFT_phantom/mask.mif \
    -mask SIFT_phantom/mask.mif -minlength 4 -select 100 tmp.trx -force
  tckconvert tmp.trx tmp_check.tck -force
  tckinfo tmp_check.tck | grep -q "count: 100"
  ```
- **New**: `trx_float16` — same as above but with `-trx_float16`; convert to TCK and compare geometry with normal float32 output using `testing_diff_tck -distance 0.1`

#### `tckedit`
- **New**: `trx_length` — create TRX from existing `tckedit/in.tck`, filter by length, convert result to TCK, compare with existing `tckedit/upper.tck` using `testing_diff_tck`:
  ```bash
  tckconvert tckedit/in.tck tmp_in.trx -force
  tckedit tmp_in.trx -minlength 10 tmp_out.trx -force
  tckconvert tmp_out.trx tmp_out.tck -force
  testing_diff_tck tmp_out.tck tckedit/upper.tck
  ```
- **New**: `trx_preserves_dps` — round-trip test that dps fields survive `tckedit`: create TRX with dps, filter, verify dps count matches streamline count in output

#### `tcksift`
- **New**: `trx_subset` — run SIFT on `SIFT_phantom/tracks.trx`, get TRX output; convert both outputs (TCK-mode and TRX-mode) to a common format and verify same streamlines are selected (or compare density maps as in the existing `default` test)

#### `tcksift2`
- **New**: `trx_dps` — run SIFT2 on `SIFT_phantom/tracks.trx` with `-trx_dps tmp_sift2.trx weights`, verify the `.trx` was created and that extracting the `weights` dps produces values equivalent to the text output `tmp.csv`. Extract dps with `tckconvert` and compare with `testing_diff_matrix`.

#### `fixel2tsf`
- **New**: `trx_dpv` — run `fixel2tsf` with `-trx_dpv tracks.trx fixel_afd` alongside the normal TSF output; extract the dpv as a TSF with `tckconvert`, compare with `testing_diff_tsf`

#### `tcksample`
- **New**: `trx_dpv` — sample with no `-stat_tck` (per-vertex mode), append to TRX with `-trx_dpv`; extract dpv with `tckconvert` to a TSF, compare with existing reference `tcksample/*.tsf`
- **New**: `trx_dps_mean` — sample with `-stat_tck mean`, append to TRX with `-trx_dps`; extract dps to CSV with `tckconvert`, compare with `tcksample/mean.csv` using `testing_diff_matrix`

### Verification tools needed

#### Short term — use `tckconvert` as a shim
`tckconvert` can already extract dps to text (`-extract_weights`) and write TRX directory format. This lets tests verify dps/dpv values by:
1. Appending dps/dpv with the command under test
2. Extracting it as text with `tckconvert -extract_weights` or equivalent
3. Comparing with `testing_diff_matrix`

#### Longer term — `testing_diff_trx`
A dedicated `testing/tools/testing_diff_trx.cpp` would provide:
- Streamline geometry comparison (delegates to Hausdorff distance logic from `testing_diff_tck`)
- Per-streamline dps field comparison with tolerances
- Per-vertex dpv field comparison with tolerances
- Group membership comparison

This is worth writing once the test suite grows beyond 5-6 tests. Until then, the `tckconvert`-shim approach is sufficient.

### Building mrview on macOS with newer Xcode Command Line Tools

The default `MacOSX.sdk` symlink in newer Xcode CLI tools versions points to a beta/preview SDK (e.g. `MacOSX26.2.sdk`) that has dropped the `AGL.framework`, which mrview links against. `AGL` still exists in `MacOSX15.sdk`. Set `SDKROOT` at build time to work around this:

```bash
SDKROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX15.sdk ninja -C build mrview
```

This produces deprecation-version warnings (homebrew libs built for macOS 14/15, binary targeting 11.0) but links successfully.

**Permanent fix** — reconfigure cmake with the explicit sysroot so the flag is baked into build.ninja and `ninja -C build mrview` works without `SDKROOT` each time:

```bash
cmake -B build -DCMAKE_OSX_SYSROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX15.sdk .
ninja -C build mrview
```

This cmake invocation was used in the session that added the group panel, so the build directory is already configured with `MacOSX15.sdk`.

**Symptom of the wrong SDK**: compilation fails immediately with `implicit instantiation of undefined template 'std::basic_ifstream<char>'` in `cpp/core/file/matrix.h`, cascading into "too many errors emitted". This is not a code bug — it means `SDKROOT` is pointing at the wrong (beta) SDK.

### Adding new Qt widget files (Q_OBJECT / AUTOMOC)

`cpp/gui/CMakeLists.txt` uses `file(GLOB_RECURSE GUI_SOURCES *.h *.cpp)` so new `.cpp` files are picked up automatically — but only after cmake re-runs. Qt's AUTOMOC generates `moc_*.cpp` files by scanning for `Q_OBJECT` in headers; this scan also only runs during cmake configuration.

**Problem**: after adding a new `.h` with `Q_OBJECT`, the MOC file is silently skipped if cmake's autogen cache is stale. The linker then fails with:

```
vtable for MR::GUI::MRView::Tool::YourClass referenced from ...
NOTE: a missing vtable usually means the first non-inline virtual member function has no definition.
```

This looks like a code error but is actually a cmake/MOC cache problem.

**Fix**: delete the autogen directory and reconfigure:

```bash
rm -rf build/cpp/gui/mrtrix-gui_autogen
cmake -B build .
ninja -C build mrview
```

The `rm` forces cmake to re-run the full AUTOMOC scan on the next configure. A plain `cmake -B build .` without deleting the autogen directory is not sufficient — cmake considers the autogen outputs up-to-date even when new `Q_OBJECT` headers have been added.

### Current test data location

Test data lives at `https://github.com/mattcieslak/test_data`, branch `add-trx-test-data` (commit `552958e`). The `testing/CMakeLists.txt` `GIT_REPOSITORY` and `GIT_TAG` are set to point there. Build with tests enabled:

```bash
cmake -B build -DMRTRIX_BUILD_TESTS=ON .
cmake --build build --target testing_diff_tck testing_diff_tsf testing_diff_matrix testing_diff_image
```

### Workflow for adding more test data

1. Generate the reference file in a local build (working directory = test data root):
   ```bash
   tckconvert SIFT_phantom/tracks.tck SIFT_phantom/tracks.trx -force
   ```
2. Commit to `add-trx-test-data` branch of the fork and push
3. Update `GIT_TAG` in `testing/CMakeLists.txt` to the new commit hash
4. Add the bash test script under `testing/binaries/tests/{command}/trx_*`
5. The test is registered automatically via CMake's directory scan — no CMakeLists changes needed

### Implemented tests (all passing)

| Test | What it verifies |
|------|-----------------|
| `tckinfo/trx_metadata` | `tckinfo` prints streamline count, dps, dpv fields for TRX input |
| `tckgen/trx_output` | `tckgen` writes valid TRX with correct streamline count |
| `tckgen/trx_float16` | `-trx_float16` produces a smaller file than float32 default |
| `tckedit/trx_length` | `-minlength`/`-maxlength` filtering of TRX input matches TCK reference |
| `tckedit/trx_preserves_metadata` | dps and dpv fields survive tckedit subsetting |
| `tcksift/trx_subset` | SIFT on TRX input produces balanced density maps (same as TCK path) |
| `tcksift2/trx_dps` | SIFT2 weights appended as dps to TRX; field visible in `tckinfo` |
| `fixel2tsf/trx_dpv` | fixel2tsf dpv values match TSF output; field visible in `tckinfo` |
| `tcksample/trx_dps_mean` | Per-streamline mean stats appended as dps match CSV reference |
| `tcksample/trx_dpv` | Per-vertex sampled values appended as dpv match TSF reference |
| `trxlabel/default` | 4 groups created after labeling SIFT phantom; group names match node indices |
| `trxlabel/multiple_atlases` | Same atlas twice with prefixes a/b → 8 groups in output TRX |
| `trx2connectome/default` | Full trxlabel + trx2connectome pipeline matches `tck2connectome/out.csv` |
| `trx2connectome/forward_search` | With `-assignment_forward_search 5`, output matches tck2connectome reference |
| `trx2connectome/group_prefix` | `-group_prefix` filters correctly; stripped names appear in `-out_node_names` |
| `tsfinfo/trx_dpv` | `tsfinfo` lists dpv fields and vertex counts for TRX input |
| `tsfvalidate/trx_dpv` | `tsfvalidate -field` validates dpv vertex count matches TRX offsets |
| `tsfthreshold/trx_dpv` | `tsfthreshold` adds binary mask dpv field; TSF extraction has correct count |
| `tsfsmooth/trx_dpv` | `tsfsmooth` appends smoothed dpv field; both fields visible in tsfinfo |
| `tsfdivide/trx_dpv` | `tsfdivide` appends ratio dpv field; TSF path regression check |
| `tsfmult/trx_dpv` | `tsfmult` appends product dpv field; TSF path regression check |
| `connectome2tck/trx_default` | `trxlabel` + `connectome2tck labeled.trx - edge-`; per-edge TCK matches TCK + text-file path |
| `connectome2tck/trx_forward_search` | Same with `-assignment_forward_search 5` on both paths |
| `connectome2tck/trx_group_prefix` | Multi-atlas TRX; `-group_prefix dk` extracts same streamlines as single-atlas labeled TRX |
| `tckmap/trx_tdi` | TRX and TCK inputs produce identical TDI (within ±1.5 streamlines/voxel) |
| `tckmap/trx_dps_weights` | Weights from TRX dps field produce same weighted TDI as external text weights file |
| `tckmap/trx_length_contrast` | Length-weighted TDI matches between TRX and TCK inputs |
| `tckstats/trx_stats` | TRX and TCK inputs report identical count/mean/std/min/max |
| `tckstats/trx_dps` | Per-streamline lengths appended as dps field; field visible in `tckinfo` |
| `tcktransform/trx_geometry` | TRX input produces identical warped geometry as TCK input (vs. reference) |
| `tcktransform/trx_preserves_metadata` | TRX→TRX in-place path preserves dps fields after warping |

## Pending Package-Level Infrastructure

These changes are not in any specific command — they are improvements to shared library code or `trx_utils.h` that would unblock or clean up multiple commands.

### P1. `copy_trx_sidecar_data(src_path, dst_path, bool include_dpv)` in `trx_utils.h`

**Needed by**: `tckresample` (TRX→TRX, dps/groups/dpg preservation after streaming).
**Also useful for**: any future geometry-modifying command that streams through `TrxStream` and needs to carry metadata forward.

Implementation sketch:
```cpp
inline void copy_trx_sidecar_data(std::string_view src_path,
                                   std::string_view dst_path,
                                   bool include_dpv = false) {
  auto src = load_trx(src_path);
  // dps
  for (const auto &[name, arr] : src->data_per_streamline) {
    std::vector<float> vals(arr->_data.data(), arr->_data.data() + arr->_data.size());
    append_dps<float>(dst_path, name, vals);
  }
  // groups (requires append_group helper — see P2)
  for (const auto &[name, group] : src->groups) {
    std::vector<uint32_t> indices = /* extract from group->_data */;
    append_group(dst_path, name, indices);
    // TODO: also copy dpg fields for this group
  }
  // dpv (only if vertex count is unchanged)
  if (include_dpv) {
    for (const auto &[name, arr] : src->data_per_vertex) {
      std::vector<float> vals(arr->_data.data(), arr->_data.data() + arr->_data.size());
      append_dpv<float>(dst_path, name, vals);
    }
  }
}
```

### P2. `append_group(trx_path, name, indices)` in `trx_utils.h`

**Needed by**: `copy_trx_sidecar_data` (P1); also useful standalone for commands that need to add a single named group.

`trxlabel` already calls `trx::append_groups_to_zip/directory` with a full groups map. A simpler per-group wrapper would allow incremental addition and simplify `copy_trx_sidecar_data`.

### P3. `ModelBase::map_streamlines` TRX support

**Needed by**: `afdconnectivity` (uses `ModelBase`, not `Model`).

`SIFT::Model::map_streamlines` (in `model.h`) already has TRX support via `if (TRX::is_trx(path))`. `ModelBase::map_streamlines` (in `model_base.h`) does not. The minimal fix is to replace `Reader<>` with `open_tractogram` in `ModelBase::map_streamlines` — removing the duplication between base and derived class implementations.

### P4. `FIXEL_MATRIX_GENERATE_SHARED` macro update in `fixel/matrix.cpp`

**Needed by**: `fixelconnectivity`.

The macro expands `Reader<float> track_file(...)` + `TrackLoader loader(track_file, ...)`. Replace with `open_tractogram(track_filename, properties, weight_src)` + `TrackLoader loader(*track_reader, ...)`. Adds weight-source lookup from `get_options("tck_weights_in")` inside the macro so both `generate_unweighted` and `generate_weighted` benefit. Since the macro is in a `.cpp` file and is private to `fixel/matrix.cpp`, no header changes are needed.

### P5. `for_each_vertex(trx, fn)` helper in `trx_utils.h`

**Needed by**: `tckresample` (would benefit), future vertex-modifying commands.

Already noted in the tcktransform section. Abstracts `_data(v,0/1/2)` + `_offsets(s,0)` access pattern. Would be used by tckresample if it ever needs in-place vertex resampling (unlikely — vertex count changes so streaming is required), but would help if any command needs to read all vertex positions for inspection.

## Key Design Decisions

1. **TRX coordinates are RAS+, same as TCK**: MRtrix treats TRX streamline coordinates as RAS+ world-space coordinates, identical to the TCK convention. The `VOXEL_TO_RASMM` field in the TRX header is never used for coordinate transformation in any MRtrix command — it is printed by `tckinfo` for informational purposes only. No affine is ever applied when reading or writing TRX geometry.

3. **Append vs. rewrite**: trx-cpp supports `append_dps_to_zip()` for adding data to existing TRX files without rewriting. This is efficient but means commands need to decide: create new TRX, or modify existing? If the same file is listed as input and output, then the new data (dps, dpv, groups) should just be appended.

4. **No region-pair groups for tck2connectome**: Creating groups for each (node_A, node_B) pair would produce O(n²) groups and is impractical for large parcellations. Instead, each node becomes a trx group.

5. **Thread safety for TrxStream**: For `tckgen`, confirm that `TrxStream::push_streamline()` is called only from the writer thread. If multi-threaded push is needed, a mutex wrapper or per-thread buffering would be required.

6. **When input is TCK but user wants TRX output**: Some commands (like `tcksift2`) don't normally write tracks at all — they write sidecar data. Adding TRX output means they'd also need to copy/reference the geometry. The cleanest approach: require the user to first convert to TRX, then use the `-trx_dps`/`-trx_dpv` options to add metadata.

8. **Sidecar resolution — embedded-first, external-fallback**: For commands that accept external sidecar files (weights, TSF scalars), the option argument doubles as either a file path or a TRX field name. Helper functions in `trx_utils.h` (`resolve_dps_weights`, `resolve_dpv_scalars`) handle the dispatch: check if the string is a path to an existing file first; if not, treat it as a dps/dpv field name in the TRX input. This eliminates the need for new `-trx_dps_*` CLI options and keeps command code unchanged.

9. **Positions dtype on read**: `load_trx()` always returns `TrxFile<float>` with correct float32 coordinates regardless of the on-disk dtype. Float16/float64 files trigger a double-load: once as float32 (for structure/dps/dpv/groups) and once with the native dtype (for positions). A `WARN` is emitted. All MRtrix commands transparently handle float16 TRX inputs.

10. **Positions dtype on write**: `tckgen` defaults to float32 TRX output (`-trx_float16` opts in to float16). The dtype is plumbed via `properties["trx_positions_dtype"]` → `WriteKernel::create_writer()` → `TRXWriterAdapter` → `TrxStream(dtype)`. Other write paths (`TRXWriter` in `trx_utils.h`, `TrxStream` in `tckedit`) currently always write float32; float16 support can be added per-command using the same property mechanism.

7. **DPV and geometry-modifying commands**: Commands that change vertex count (tckresample) invalidate dpv data. Policy: discard dpv with a warning unless interpolation is explicitly requested. Commands that only transform coordinates (tcktransform) preserve dpv unchanged.
