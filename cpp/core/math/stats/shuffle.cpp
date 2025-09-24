/* Copyright (c) 2008-2025 the MRtrix3 contributors.
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

#include "math/stats/shuffle.h"

#include <algorithm>
#include <random>

#include "file/matrix.h"
#include "math/factorial.h"
#include "math/rng.h"

namespace MR::Math::Stats {

std::vector<std::string> error_types = {"ee", "ise", "both"};

App::OptionGroup shuffle_options(const bool include_nonstationarity, const default_type default_skew) {
  using namespace App;

  // clang-format off
  OptionGroup result =
      OptionGroup("Options relating to shuffling of data for nonparametric statistical inference")
      + Option("notest",
               "don't perform statistical inference;"
               " only output population statistics"
               " (effect size, stdev etc)")
      + Option("errors",
               "specify nature of errors for shuffling;"
               " options are: " + join(error_types, ",") +
               " (default: ee)")
        + Argument("spec").type_choice(error_types)
      + Option("exchange_within",
               "specify blocks of observations within each of which data may undergo restricted exchange")
        + Argument("file").type_file_in()
      + Option("exchange_whole",
               "specify blocks of observations that may be exchanged with one another"
               " (for independent and symmetric errors, sign-flipping will occur block-wise)")
        + Argument("file").type_file_in()
      + Option("strong",
               "use strong familywise error control across multiple hypotheses")
      + Option("nshuffles",
               "the number of shuffles"
               " (default: " + str(DEFAULT_NUMBER_SHUFFLES) + ")")
        + Argument("number").type_integer(1)
      + Option("permutations",
               "manually define the permutations (relabelling)."
               " The input should be a text file defining a m x n matrix,"
               " where each relabelling is defined as a column vector of size m,"
               " and the number of columns n defines the number of permutations."
               " Can be generated with the palm_quickperms function in PALM"
               " (http://fsl.fmrib.ox.ac.uk/fsl/fslwiki/PALM)."
               " Overrides the -nshuffles option.")
        + Argument("file").type_file_in();

  if (include_nonstationarity) {
    result + Option("nonstationarity",
                    "perform non-stationarity correction")
           + Option("skew_nonstationarity",
                    "specify the skew parameter for empirical statistic calculation"
                    " (default for this command is " + str(default_skew) + ")")
             + Argument("value").type_float(0.0)
           + Option("nshuffles_nonstationarity",
                    "the number of shuffles to use when precomputing the empirical statistic image"
                    " for non-stationarity correction"
                    " (default: " + str(DEFAULT_NUMBER_SHUFFLES_NONSTATIONARITY) + ")")
             + Argument("number").type_integer(1)
           + Option("permutations_nonstationarity",
                    "manually define the permutations (relabelling)"
                    " for computing the emprical statistics for non-stationarity correction."
                    " The input should be a text file defining a m x n matrix,"
                    " where each relabelling is defined as a column vector of size m,"
                    " and the number of columns n defines the number of permutations."
                    " Can be generated with the palm_quickperms function in PALM"
                    " (http://fsl.fmrib.ox.ac.uk/fsl/fslwiki/PALM)."
                    " Overrides the -nshuffles_nonstationarity option.")
             + Argument("file").type_file_in();
  }
  // clang-format on
  return result;
}

Shuffler::Shuffler(const index_type num_rows, const bool is_nonstationarity, const std::string msg)
    : rows(num_rows),
      nshuffles(is_nonstationarity ? DEFAULT_NUMBER_SHUFFLES_NONSTATIONARITY : DEFAULT_NUMBER_SHUFFLES),
      counter(0) {
  using namespace App;
  auto opt = get_options("errors");
  error_t error_types = error_t::EE;
  if (!opt.empty()) {
    switch (int(opt[0][0])) {
    case 0:
      error_types = error_t::EE;
      break;
    case 1:
      error_types = error_t::ISE;
      break;
    case 2:
      error_types = error_t::BOTH;
      break;
    }
  }

  bool nshuffles_explicit = false;
  opt = get_options(is_nonstationarity ? "nshuffles_nonstationarity" : "nshuffles");
  if (!opt.empty()) {
    nshuffles = opt[0][0];
    nshuffles_explicit = true;
  }

  opt = get_options(is_nonstationarity ? "permutations_nonstationarity" : "permutations");
  if (!opt.empty()) {
    if (error_types == error_t::EE || error_types == error_t::BOTH) {
      load_permutations(opt[0][0]);
      if (permutations[0].size() != rows)
        throw Exception("Number of entries per shuffle in file \"" + std::string(opt[0][0]) +
                        "\" does not match number of rows in design matrix (" + str(rows) + ")");
      if (nshuffles_explicit && nshuffles != permutations.size())
        throw Exception("Number of shuffles explicitly requested (" + str(nshuffles) +
                        ") does not match number of shuffles in file \"" + std::string(opt[0][0]) + "\" (" +
                        str(permutations.size()) + ")");
      nshuffles = permutations.size();
    } else {
      throw Exception("Cannot manually provide permutations if errors are not exchangeable");
    }
  }

  opt = get_options("exchange_within");
  index_array_type eb_within;
  if (!opt.empty()) {
    try {
      eb_within = load_blocks(std::string(opt[0][0]), false);
    } catch (Exception &e) {
      throw Exception(e, "Unable to read file \"" + std::string(opt[0][0]) + "\" as within-block exchangeability");
    }
  }

  opt = get_options("exchange_whole");
  index_array_type eb_whole;
  if (!opt.empty()) {
    if (eb_within.size())
      throw Exception("Cannot specify both \"within\" and \"whole\" exchangeability block data");
    try {
      eb_whole = load_blocks(std::string(opt[0][0]), true);
    } catch (Exception &e) {
      throw Exception(e, "Unable to read file \"" + std::string(opt[0][0]) + "\" as whole-block exchangeability");
    }
  }

  initialise(error_types, nshuffles_explicit, is_nonstationarity, eb_within, eb_whole);

  if (!msg.empty())
    progress.reset(new ProgressBar(msg, nshuffles));
}

Shuffler::Shuffler(const index_type num_rows,
                   const index_type num_shuffles,
                   const error_t error_types,
                   const bool is_nonstationarity,
                   const std::string msg)
    : Shuffler(num_rows, num_shuffles, error_types, is_nonstationarity, index_array_type(), index_array_type(), msg) {}

Shuffler::Shuffler(const index_type num_rows,
                   const index_type num_shuffles,
                   const error_t error_types,
                   const bool is_nonstationarity,
                   const index_array_type &eb_within,
                   const index_array_type &eb_whole,
                   const std::string msg)
    : rows(num_rows), nshuffles(num_shuffles) {
  initialise(error_types, true, is_nonstationarity, eb_within, eb_whole);
  if (!msg.empty())
    progress.reset(new ProgressBar(msg, nshuffles));
}

bool Shuffler::operator()(Shuffle &output) {
  output.index = counter;
  if (counter >= nshuffles) {
    if (progress)
      progress.reset(nullptr);
    output.data.resize(0, 0);
    return false;
  }
  // TESTME Think I need to adjust the signflips application based on the permutations
  if (!permutations.empty()) {
    output.data = matrix_type::Zero(rows, rows);
    for (index_type i = 0; i != rows; ++i)
      output.data(i, permutations[counter][i]) = 1.0;
  } else {
    output.data = matrix_type::Identity(rows, rows);
  }
  if (!signflips.empty()) {
    for (index_type r = 0; r != rows; ++r) {
      if (signflips[counter][r]) {
        for (index_type c = 0; c != rows; ++c) {
          if (output.data(r, c))
            output.data(r, c) *= -1.0;
        }
      }
    }
  }
  ++counter;
  if (progress)
    ++(*progress);
  return true;
}

void Shuffler::reset() {
  counter = 0;
  progress.reset();
}

void Shuffler::initialise(const error_t error_types,
                          const bool nshuffles_explicit,
                          const bool is_nonstationarity,
                          const index_array_type &eb_within,
                          const index_array_type &eb_whole) {
  assert(!(eb_within.size() && eb_whole.size()));
  if (eb_within.size()) {
    assert(index_type(eb_within.size()) == rows);
    assert(!eb_within.minCoeff());
  }
  if (eb_whole.size()) {
    assert(index_type(eb_whole.size()) == rows);
    assert(!eb_whole.minCoeff());
  }

  const bool ee = (error_types == error_t::EE || error_types == error_t::BOTH);
  const bool ise = (error_types == error_t::ISE || error_types == error_t::BOTH);

  uint64_t max_num_permutations;
  if (eb_within.size()) {
    std::vector<index_type> counts(eb_within.maxCoeff() + 1, 0);
    for (index_type i = 0; i != eb_within.size(); ++i)
      counts[eb_within[i]]++;
    max_num_permutations = 1;
    for (const auto &b : counts) {
      const uint64_t old_value = max_num_permutations;
      const uint64_t max_permutations_within_block = factorial<uint64_t>(b);
      max_num_permutations *= factorial<uint64_t>(b);
      if (max_num_permutations / max_permutations_within_block != old_value) {
        max_num_permutations = std::numeric_limits<uint64_t>::max();
        break;
      }
    }
  } else if (eb_whole.size()) {
    max_num_permutations = factorial<uint64_t>(eb_whole.maxCoeff() + 1);
  } else {
    max_num_permutations = factorial<uint64_t>(rows);
  }

  auto safe2pow = [](const uint64_t i) {
    return (i >= 8 * sizeof(uint64_t)) ? (std::numeric_limits<uint64_t>::max()) : ((uint64_t(1) << i));
  };
  const uint64_t max_num_signflips = eb_whole.size() ? safe2pow(eb_whole.maxCoeff() + 1) : safe2pow(rows);

  uint64_t max_shuffles;
  if (ee) {
    if (ise) {
      max_shuffles = max_num_permutations * max_num_signflips;
      if (max_shuffles / max_num_signflips != max_num_permutations)
        max_shuffles = std::numeric_limits<uint64_t>::max();
    } else {
      max_shuffles = max_num_permutations;
    }
  } else {
    max_shuffles = max_num_signflips;
  }

  if (max_shuffles < nshuffles) {
    if (nshuffles_explicit) {
      WARN("User requested " + str(nshuffles) + " shuffles for " +
           (is_nonstationarity ? "non-stationarity correction" : "null distribution generation") + ", but only " +
           str(max_shuffles) +
           " unique shuffles can be generated; "
           "this will restrict the minimum achievable p-value to " +
           str(1.0 / max_shuffles));
    } else {
      WARN("Only " + str(max_shuffles) +
           " unique shuffles can be generated, which is less than the default number of " + str(nshuffles) + " for " +
           (is_nonstationarity ? "non-stationarity correction" : "null distribution generation") +
           "; "
           "this will restrict the minimum achievable p-value to " +
           str(1.0 / max_shuffles));
    }
    nshuffles = max_shuffles;
  } else if (max_shuffles == std::numeric_limits<uint64_t>::max()) {
    DEBUG("Maximum possible number of shuffles was not computable using 64-bit integers");
  } else {
    DEBUG("Maximum possible number of shuffles was computed as " + str(max_shuffles) + "; " +
          (nshuffles_explicit ? "user-requested" : "default") + " number of " + str(nshuffles) + " will be used");
  }

  // Need special handling of cases where both ee and ise are used
  // - If forced to use all shuffles, need to:
  //   - Generate all permutations, but duplicate each according to the number of signflips
  //   - Generate all signflips, but duplicate each according to the number of permutations
  //   - Interleave one of the two of them, so that every combination of paired permutation-signflip is unique
  // - If using a fixed number of shuffles:
  //   - If fixed number is less than the maximum number of permutations, generate that number randomly
  //   - If fixed number is equal to the maximum number of permutations, generate the full set
  //   - If fixed number is greater than the maximum number of permutations, generate that number randomly,
  //     while disabling detection of duplicates
  //   - Repeat the three steps above for signflips

  if (ee && permutations.empty()) {
    if (ise) {
      if (nshuffles == max_shuffles) {
        generate_all_permutations(rows, eb_within, eb_whole);
        assert(permutations.size() == max_num_permutations);
        std::vector<PermuteLabels> duplicated_permutations;
        duplicated_permutations.reserve(max_shuffles);
        for (const auto &p : permutations) {
          for (index_type i = 0; i != max_num_signflips; ++i)
            duplicated_permutations.push_back(p);
        }
        std::swap(permutations, duplicated_permutations);
        assert(permutations.size() == max_shuffles);
      } else if (nshuffles == max_num_permutations) {
        generate_all_permutations(rows, eb_within, eb_whole);
        assert(permutations.size() == max_num_permutations);
      } else {
        // - Only include the default shuffling if this is the actual permutation testing;
        //   if we're doing nonstationarity correction, don't include the default
        // - Permit duplicates (specifically of permutations only) if an adequate number cannot be generated
        generate_random_permutations(
            nshuffles, rows, eb_within, eb_whole, !is_nonstationarity, nshuffles > max_num_permutations);
      }
    } else if (nshuffles < max_shuffles) {
      generate_random_permutations(nshuffles, rows, eb_within, eb_whole, !is_nonstationarity, false);
    } else {
      generate_all_permutations(rows, eb_within, eb_whole);
      assert(permutations.size() == max_shuffles);
    }
  }

  if (ise) {
    if (ee) {
      if (nshuffles == max_shuffles) {
        generate_all_signflips(rows, eb_whole);
        assert(signflips.size() == max_num_signflips);
        std::vector<BitSet> duplicated_signflips;
        duplicated_signflips.reserve(max_shuffles);
        for (index_type i = 0; i != max_num_permutations; ++i)
          duplicated_signflips.insert(duplicated_signflips.end(), signflips.begin(), signflips.end());
        std::swap(signflips, duplicated_signflips);
        assert(signflips.size() == max_shuffles);
      } else if (nshuffles == max_num_signflips) {
        generate_all_signflips(rows, eb_whole);
        assert(signflips.size() == max_num_signflips);
      } else {
        generate_random_signflips(nshuffles, rows, eb_whole, !is_nonstationarity, nshuffles > max_num_signflips);
      }
    } else if (nshuffles < max_shuffles) {
      generate_random_signflips(nshuffles, rows, eb_whole, !is_nonstationarity, false);
    } else {
      generate_all_signflips(rows, eb_whole);
      assert(signflips.size() == max_shuffles);
    }
  }

  if (max_shuffles < nshuffles)
    nshuffles = max_shuffles;
}

index_array_type Shuffler::load_blocks(const std::string &filename, const bool equal_sizes) {
  index_array_type data = File::Matrix::load_vector<index_type>(filename).array();
  if (index_type(data.size()) != rows)
    throw Exception("Number of entries in file \"" + filename + "\" (" + str(data.size()) +
                    ") does not match number of inputs (" + str(rows) + ")");
  const index_type min_coeff = data.minCoeff();
  index_type max_coeff = data.maxCoeff();
  if (min_coeff > 1)
    throw Exception("Minimum index in file \"" + filename + "\" must be either 0 or 1");
  if (min_coeff) {
    data.array() -= 1;
    max_coeff--;
  }
  std::vector<index_type> counts(max_coeff + 1, 0);
  for (index_type i = 0; i != index_type(data.size()); ++i)
    counts[data[i]]++;
  for (index_type i = 0; i <= max_coeff; ++i) {
    if (counts[i] < 2)
      throw Exception("Sequential indices in file \"" + filename + "\" must contain at least two entries each");
  }
  if (equal_sizes) {
    for (index_type i = 1; i <= max_coeff; ++i) {
      if (counts[i] != counts[0])
        throw Exception("Indices in file \"" + filename + "\" do not contain the same number of elements each");
    }
  }
  return data;
}

bool Shuffler::is_duplicate(const PermuteLabels &v1, const PermuteLabels &v2) const {
  assert(v1.size() == v2.size());
  for (index_type i = 0; i < v1.size(); i++) {
    if (v1[i] != v2[i])
      return false;
  }
  return true;
}

bool Shuffler::is_duplicate(const PermuteLabels &perm) const {
  for (const auto &p : permutations) {
    if (is_duplicate(perm, p))
      return true;
  }
  return false;
}

void Shuffler::generate_random_permutations(const index_type num_perms,
                                            const index_type num_rows,
                                            const index_array_type &eb_within,
                                            const index_array_type &eb_whole,
                                            const bool include_default,
                                            const bool permit_duplicates) {
  Math::RNG rng;

  permutations.clear();
  permutations.reserve(num_perms);

  PermuteLabels default_labelling(num_rows);
  for (index_type i = 0; i < num_rows; ++i)
    default_labelling[i] = i;

  index_type p = 0;
  if (include_default) {
    permutations.push_back(default_labelling);
    ++p;
  }

  // Unrestricted exchangeability
  if (!eb_within.size() && !eb_whole.size()) {
    for (; p != num_perms; ++p) {
      PermuteLabels permuted_labelling(default_labelling);
      do {
        std::shuffle(permuted_labelling.begin(), permuted_labelling.end(), rng);
      } while (!permit_duplicates && is_duplicate(permuted_labelling));
      permutations.push_back(permuted_labelling);
    }
    return;
  }

  std::vector<std::vector<index_type>> blocks;

  // Within-block exchangeability
  if (eb_within.size()) {
    blocks = indices2blocks(eb_within);
    PermuteLabels permuted_labelling(default_labelling);
    for (; p != num_perms; ++p) {
      do {
        permuted_labelling = default_labelling;
        // Random permutation within each block independently
        for (index_type ib = 0; ib != blocks.size(); ++ib) {
          std::vector<index_type> permuted_block(blocks[ib]);
          std::shuffle(permuted_block.begin(), permuted_block.end(), rng);
          for (index_type i = 0; i != permuted_block.size(); ++i)
            permuted_labelling[blocks[ib][i]] = permuted_block[i];
        }
      } while (!permit_duplicates && is_duplicate(permuted_labelling));
      permutations.push_back(permuted_labelling);
    }
    return;
  }

  // Whole-block exchangeability
  blocks = indices2blocks(eb_whole);
  const index_type num_blocks = blocks.size();
  assert(!(num_rows % num_blocks));
  const index_type block_size = num_rows / num_blocks;
  PermuteLabels default_blocks(num_blocks);
  for (index_type i = 0; i != num_blocks; ++i)
    default_blocks[i] = i;
  PermuteLabels permuted_labelling(default_labelling);
  for (; p != num_perms; ++p) {
    do {
      // Randomly order a list corresponding to the block indices, and then
      //   generate the full permutation label listing accordingly
      PermuteLabels permuted_blocks(default_blocks);
      std::shuffle(permuted_blocks.begin(), permuted_blocks.end(), rng);
      for (index_type ib = 0; ib != num_blocks; ++ib) {
        for (index_type i = 0; i != block_size; ++i)
          permuted_labelling[blocks[ib][i]] = blocks[permuted_blocks[ib]][i];
      }
    } while (!permit_duplicates && is_duplicate(permuted_labelling));
    permutations.push_back(permuted_labelling);
  }
}

void Shuffler::generate_all_permutations(const index_type num_rows,
                                         const index_array_type &eb_within,
                                         const index_array_type &eb_whole) {
  permutations.clear();

  // Unrestricted exchangeability
  if (!eb_within.size() && !eb_whole.size()) {
    permutations.reserve(factorial(num_rows));
    PermuteLabels temp(num_rows);
    for (index_type i = 0; i < num_rows; ++i)
      temp[i] = i;
    permutations.push_back(temp);
    while (std::next_permutation(temp.begin(), temp.end()))
      permutations.push_back(temp);
    return;
  }

  std::vector<std::vector<index_type>> original;

  // Within-block exchangeability
  if (eb_within.size()) {

    original = indices2blocks(eb_within);

    auto write = [&](const std::vector<std::vector<index_type>> &data) {
      PermuteLabels temp(num_rows);
      for (index_type block = 0; block != data.size(); ++block) {
        for (index_type i = 0; i != data[block].size(); ++i)
          temp[original[block][i]] = data[block][i];
      }
      permutations.push_back(std::move(temp));
    };

    std::vector<std::vector<index_type>> blocks(original);
    write(blocks);
    do {
      index_type ib = 0;
      // Go to the next valid permutation within the first block;
      //   if there are no more permutations left, the data within that block becomes
      //   re-sorted (i.e. the first within-block permutation), and we get the
      //   next permutation of the next block; if there's also no more permutations
      //   within that block, go to the next block, and so on.
      while (!std::next_permutation(blocks[ib].begin(), blocks[ib].end())) {
        if (++ib == blocks.size())
          return;
      }
      write(blocks);
    } while (true);
  }

  // Whole-block exchangeability
  original = indices2blocks(eb_whole);
  const index_type num_blocks = original.size();
  PermuteLabels indices(num_blocks);
  for (index_type i = 0; i != num_blocks; ++i)
    indices[i] = i;

  auto write = [&](const PermuteLabels &data) {
    PermuteLabels temp(num_rows);
    for (index_type ib = 0; ib != original.size(); ++ib) {
      for (index_type i = 0; i != original[ib].size(); ++i)
        temp[original[ib][i]] = original[data[ib]][i];
    }
    permutations.push_back(std::move(temp));
  };

  // All possible permutations of blocks;
  //   preserving data within each block is handled within write()
  write(indices);
  while (std::next_permutation(indices.begin(), indices.end()))
    write(indices);
}

void Shuffler::load_permutations(const std::string &filename) {
  std::vector<std::vector<index_type>> temp = File::Matrix::load_matrix_2D_vector<index_type>(filename);
  if (temp.empty())
    throw Exception("no data found in permutations file: " + str(filename));

  const index_type min_value = *std::min_element(std::begin(temp[0]), std::end(temp[0]));
  if (min_value > 1)
    throw Exception("indices for relabelling in permutations file must start from either 0 or 1");

  // TODO Support transposed permutations
  permutations.assign(temp[0].size(), PermuteLabels(temp.size()));
  for (index_type i = 0; i != temp[0].size(); i++) {
    for (index_type j = 0; j != temp.size(); j++)
      permutations[i][j] = temp[j][i] - min_value;
  }
}

bool Shuffler::is_duplicate(const BitSet &sign) const {
  for (const auto &s : signflips) {
    if (sign == s)
      return true;
  }
  return false;
}

void Shuffler::generate_random_signflips(const index_type num_signflips,
                                         const index_type num_rows,
                                         const index_array_type &block_indices,
                                         const bool include_default,
                                         const bool permit_duplicates) {
  signflips.clear();
  signflips.reserve(num_signflips);
  index_type s = 0;
  if (include_default) {
    BitSet default_labelling(num_rows, false);
    signflips.push_back(default_labelling);
    ++s;
  }
  std::random_device rd;
  std::mt19937 generator(rd());
  std::uniform_int_distribution<> distribution(0, 1);

  BitSet rows_to_flip(num_rows);

  // Whole-block sign-flipping
  if (block_indices.size()) {
    const auto blocks = indices2blocks(block_indices);
    for (; s != num_signflips; ++s) {
      do {
        for (index_type ib = 0; ib != blocks.size(); ++ib) {
          const bool value = distribution(generator);
          for (const auto i : blocks[ib])
            rows_to_flip[i] = value;
        }
      } while (!permit_duplicates && is_duplicate(rows_to_flip));
      signflips.push_back(rows_to_flip);
    }
    return;
  }

  // Unrestricted sign-flipping
  for (; s != num_signflips; ++s) {
    do {
      // TODO Should be a faster mechanism for generating / storing random bits
      for (index_type ir = 0; ir != num_rows; ++ir)
        rows_to_flip[ir] = distribution(generator);
    } while (!permit_duplicates && is_duplicate(rows_to_flip));
    signflips.push_back(rows_to_flip);
  }
}

void Shuffler::generate_all_signflips(const index_type num_rows, const index_array_type &block_indices) {
  signflips.clear();

  // Whole-block sign-flipping
  if (block_indices.size()) {
    const auto blocks = indices2blocks(block_indices);

    auto write = [&](const BitSet &data) {
      BitSet temp(num_rows);
      for (index_type ib = 0; ib != blocks.size(); ++ib) {
        if (data[ib]) {
          for (const auto i : blocks[ib])
            temp[i] = true;
        }
      }
      signflips.push_back(std::move(temp));
    };

    BitSet temp(blocks.size());
    write(temp);
    do {
      index_type ib = 0;
      while (temp[ib]) {
        if (++ib == blocks.size())
          return;
      }
      temp[ib] = true;
      for (index_type ib2 = 0; ib2 != ib; ++ib2)
        temp[ib2] = false;
      write(temp);
    } while (true);
  }

  // Unrestricted sign-flipping
  signflips.reserve(size_t(1) << num_rows);
  BitSet temp(num_rows, false);
  signflips.push_back(temp);
  while (!temp.full()) {
    index_type last_zero_index;
    for (last_zero_index = num_rows - 1; temp[last_zero_index]; --last_zero_index)
      ;
    temp[last_zero_index] = true;
    for (index_type indices_zeroed = last_zero_index + 1; indices_zeroed != num_rows; ++indices_zeroed)
      temp[indices_zeroed] = false;
    signflips.push_back(temp);
  }
}

std::vector<std::vector<index_type>> Shuffler::indices2blocks(const index_array_type &indices) const {
  const index_type num_blocks = indices.maxCoeff() + 1;
  std::vector<std::vector<index_type>> result;
  result.resize(num_blocks);
  for (index_type i = 0; i != indices.size(); ++i)
    result[indices[i]].push_back(i);
  return result;
}

} // namespace MR::Math::Stats
