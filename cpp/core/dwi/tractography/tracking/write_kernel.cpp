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

#include "dwi/tractography/tracking/write_kernel.h"
#include "dwi/tractography/trx_utils.h"

namespace MR::DWI::Tractography::Tracking {

namespace {

// Wraps Writer<float> (TCK) behind AbstractTrackWriter.
// Writer<float> manages its own internal count/total_count for the file header;
// we maintain the base-class fields independently for WriteKernel's use.
class TCKWriterAdapter : public AbstractTrackWriter {
public:
  TCKWriterAdapter(std::string_view path, const Tractography::Properties &props) : impl(path, props) {}
  bool operator()(const Streamline<float> &tck) override {
    impl(tck);
    ++count;
    ++total_count;
    return true;
  }
  void skip() override {
    impl.skip();
    ++total_count;
  }

private:
  Tractography::Writer<float> impl;
};

// Wraps TrxStream behind AbstractTrackWriter for TRX output without pre-counting.
class TRXWriterAdapter : public AbstractTrackWriter {
public:
  explicit TRXWriterAdapter(std::string_view path,
                            std::string_view positions_dtype = "float32",
                            const Tractography::Properties *props = nullptr)
      : output_path(std::string(path)), stream(std::string(positions_dtype)) {
    if (props) {
      json::object meta;
      for (const auto &[key, val] : *props) {
        if (key == "trx_positions_dtype")
          continue; // internal implementation detail, not useful metadata
        meta[key] = json(val);
      }
      if (!meta.empty())
        stream.header = trx::_json_set(stream.header, "metadata", json(meta));
    }
  }
  bool operator()(const Streamline<float> &tck) override {
    std::vector<std::array<float, 3>> pts(tck.size());
    for (size_t i = 0; i < tck.size(); ++i)
      pts[i] = {tck[i][0], tck[i][1], tck[i][2]};
    stream.push_streamline(pts);
    ++count;
    ++total_count;
    return true;
  }
  void skip() override { ++total_count; }
  ~TRXWriterAdapter() override {
    try {
      stream.finalize(output_path, trx::TrxSaveOptions{});
    } catch (const std::exception &e) {
      Exception(e.what()).display();
      App::exit_error_code = 1;
    }
  }

private:
  std::string output_path;
  trx::TrxStream stream;
};

} // anonymous namespace

std::unique_ptr<AbstractTrackWriter> WriteKernel::create_writer(std::string_view path,
                                                                const Tractography::Properties &properties) {
  if (TRX::is_trx(path)) {
    const auto it = properties.find("trx_positions_dtype");
    const std::string dtype = (it != properties.end() && !it->second.empty()) ? it->second : "float32";
    return std::make_unique<TRXWriterAdapter>(path, dtype, &properties);
  }
  return std::make_unique<TCKWriterAdapter>(path, properties);
}

bool WriteKernel::operator()(const GeneratedTrack &tck) {
  if (complete())
    return false;
  if (!tck.empty() && output_seeds) {
    const auto &p = tck[tck.get_seed_index()];
    (*output_seeds) << str(writer->count) << "," << str(tck.get_seed_index()) << "," << str(p[0]) << "," << str(p[1])
                    << "," << str(p[2]) << ",\n";
  }
  switch (tck.get_status()) {
  case GeneratedTrack::status_t::INVALID:
    assert(0);
    break;
  case GeneratedTrack::status_t::ACCEPTED:
    ++selected;
    ++streamlines;
    ++seeds;
    (*writer)(tck);
    break;
  case GeneratedTrack::status_t::TRACK_REJECTED:
    ++streamlines;
    ++seeds;
    writer->skip();
    break;
  case GeneratedTrack::status_t::SEED_REJECTED:
    ++seeds;
    break;
  }
  progress.update(
      [&]() {
        return printf(
            "%8" PRIu64 " seeds, %8" PRIu64 " streamlines, %8" PRIu64 " selected", seeds, streamlines, selected);
      },
      always_increment ? true : tck.size());
  if (early_exit(seeds, selected)) {
    WARN(std::string("Track generation terminating prematurely:"                   //
                     " Highly unlikely to reach target number of streamlines (p<") //
         + str(EarlyExit::probability_threshold, 1) + ")");                        //
    return false;
  }
  return true;
}

} // namespace MR::DWI::Tractography::Tracking
