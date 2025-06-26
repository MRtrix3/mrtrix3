/* Copyright (c) 2008-2024 the MRtrix3 contributors.
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

#include "denoise/exports.h"

#include "app.h"
#include "dwi/gradient.h"
#include "file/ofstream.h"

namespace MR::Denoise {

Exports::Exports(const Header &in, const Header &ss) //
    : H_in(std::make_shared<Header>(in)),            //
      H_ss(std::make_shared<Header>(ss)) {           //
  DWI::clear_DW_scheme(*H_in);
  H_in->ndim() = 3;
  H_in->reset_intensity_scaling();
  H_in->datatype() = DataType::Float32;
  H_in->datatype().set_byte_order_native();
}

void Exports::set_noise_out(const std::string &path) { noise_out = Image<float>::create(path, *H_ss); }
void Exports::set_noise_out() { noise_out = Image<float>::scratch(*H_ss, "Scratch image for noise level output"); }

void Exports::set_lamplus(const std::string &path) { lamplus = Image<float>::create(path, *H_ss); }

void Exports::set_rank_pcanonzero(const std::string &path) {
  Header H(*H_ss);
  H.datatype() = DataType::UInt16;
  H.datatype().set_byte_order_native();
  rank_pcanonzero = Image<uint16_t>::create(path, H);
}

void Exports::set_rank_input(const std::string &path) {
  Header H(*H_ss);
  H.datatype() = DataType::UInt16;
  H.datatype().set_byte_order_native();
  rank_input = Image<uint16_t>::create(path, H);
}

void Exports::set_rank_output(const std::string &path) { rank_output = Image<float>::create(path, *H_in); }

void Exports::set_sum_optshrink(const std::string &path) { sum_optshrink = Image<float>::create(path, *H_ss); }

void Exports::set_max_dist(const std::string &path) { max_dist = Image<float>::create(path, *H_ss); }

void Exports::set_voxelcount(const std::string &path) {
  Header H(*H_ss);
  H.datatype() = DataType::UInt16;
  H.datatype().set_byte_order_native();
  voxelcount = Image<uint16_t>::create(path, H);
}

void Exports::set_patchcount(const std::string &path) {
  Header H(*H_in);
  H.datatype() = DataType::UInt16;
  H.datatype().set_byte_order_native();
  patchcount = Image<uint16_t>::create(path, H);
}

void Exports::set_sum_aggregation(const std::string &path) { sum_aggregation = Image<float>::create(path, *H_in); }
void Exports::set_sum_aggregation() {
  sum_aggregation = Image<float>::scratch(*H_in, "Scratch image for patch aggregation sums");
}

void Exports::set_variance_removed(const std::string &path) { variance_removed = Image<float>::create(path, *H_ss); }

void Exports::set_eigenspectra_path(const std::string &path) {
  assert(!eigenspectra);
  eigenspectra.reset(new Eigenspectra(path));
}

void Exports::add_eigenspectrum(const eigenvalues_type &s) {
  assert(eigenspectra);
  eigenspectra->add(s);
}

Exports::Eigenspectra::Eigenspectra(const std::string &path) : path(path) {}

void Exports::Eigenspectra::add(const eigenvalues_type &s) { data.push_back(s); }

Exports::Eigenspectra::~Eigenspectra() {
  File::OFStream out(path);
  out << "# " << App::command_history_string << "\n";
  for (const auto &s : data) {
    out << s[0];
    for (ssize_t i = 1; i != s.size(); ++i)
      out << "," << s[i];
    out << "\n";
  }
}

} // namespace MR::Denoise
