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
#include <cstdint>
#include <map>
#include <vector>

namespace MR::DWI::Tractography::Compression {

//! \brief A canonical Huffman coder over float symbols for the .zfib format.
/*! Self-contained Huffman coder operating on a single dictionary of \c float
 * symbols (the absolute, quantized world-mm coordinate values of the .zfib
 * signal). The encoding is the "encoding" step of the .zfib pipeline (Presseau
 * et al.): a binary tree is built from the symbol frequencies, the most probable
 * symbols receive the shortest code words, and the message is packed bit-by-bit.
 *
 * \par Determinism contract
 * The on-disk .zfib block stores, per symbol, a \c (symbol, frequency) pair; the
 * reader rebuilds the tree from those pairs alone. For the rebuilt tree to be
 * byte-identical to the writer's, both sides must order nodes identically. The
 * tie-break replicates the reference encoder
 * (github.com/scilus/FiberCompression, Huffman.h \c NodeOrder): a min-heap on
 * frequency, then leaf-before-internal, then recursion on the children, then by
 * symbol value. Because that ordering is total over the leaf set, the tree
 * depends only on the \c (symbol, frequency) multiset and not on insertion
 * order, so from_histogram() and from_dictionary() produce the same tree.
 *
 * \par Frequencies
 * Frequencies are stored as a normalised probability in \c float (count divided
 * by the total symbol count). from_histogram() builds the tree from exactly the
 * \c float values it will store, so a tree rebuilt by from_dictionary() from
 * those stored values is identical.
 *
 * \par Bit packing
 * Bits are packed least-significant-bit first into each byte, matching the
 * reference's std::bitset<8> serialisation (bit \c i of the stream occupies bit
 * position \c i%8 of its byte). Any trailing pad bits in the final byte are
 * ignored on decode because decoding stops once the expected symbol count is
 * reached. */
class Huffman {
public:
  //! \brief One dictionary entry: a symbol and its normalised probability.
  struct DictEntry {
    float symbol;
    float frequency;
  };

  //! \brief Build a coder from a symbol→count histogram (writer side).
  static Huffman from_histogram(const std::map<float, uint64_t> &histogram);

  //! \brief Rebuild a coder from the stored \c (symbol, frequency) pairs (reader side).
  static Huffman from_dictionary(std::vector<DictEntry> dictionary);

  //! \brief the dictionary entries, in ascending symbol order.
  const std::vector<DictEntry> &dictionary() const { return dict; }

  //! \brief Encode \a symbols into a packed byte stream (LSB-first).
  std::vector<std::byte> encode(const std::vector<float> &symbols) const;

  //! \brief Decode \a symbol_count symbols from \a num_bytes packed bytes.
  std::vector<float> decode(const std::byte *data, size_t num_bytes, size_t symbol_count) const;

private:
  //! \brief A tree node held in an index-addressed arena (no naked pointers).
  /*! A node is a leaf when \c left is negative; an internal node stores its two
   * child arena indices in \c left (bit 0) and \c right (bit 1). */
  struct Node {
    float frequency;
    int left;
    int right;
    float symbol;
  };

  std::vector<Node> nodes; //!< arena of leaves and internal nodes
  int root = -1;           //!< arena index of the tree root, or -1 if empty
  std::vector<DictEntry> dict;
  std::map<float, std::vector<bool>> codes; //!< per-symbol code word

  //! \brief assemble \c nodes / \c root / \c codes from the populated \c dict.
  void build();

  //! \brief the reference NodeOrder strict-weak-ordering over two arena nodes.
  bool node_less(int a, int b) const;
};

} // namespace MR::DWI::Tractography::Compression
