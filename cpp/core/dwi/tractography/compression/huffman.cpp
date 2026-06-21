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

#include "dwi/tractography/compression/huffman.h"

#include <algorithm>
#include <queue>
#include <utility>

#include "exception.h"
#include "mrtrix.h"

namespace MR::DWI::Tractography::Compression {

bool Huffman::node_less(const int a, const int b) const {
  // Replicates the reference Huffman.h NodeOrder comparator exactly. Used as the
  //   std::priority_queue comparator, whose top() is therefore the node of
  //   smallest frequency (a min-heap on frequency).
  const Node &na = nodes[a];
  const Node &nb = nodes[b];
  if (nb.frequency < na.frequency)
    return true;
  if (na.frequency < nb.frequency)
    return false;
  const bool a_leaf = na.left < 0;
  const bool b_leaf = nb.left < 0;
  if (a_leaf && !b_leaf)
    return true;
  if (!a_leaf && b_leaf)
    return false;
  if (!a_leaf && !b_leaf) {
    if (node_less(na.left, nb.left))
      return true;
    if (node_less(nb.left, na.left))
      return false;
    return node_less(na.right, nb.right);
  }
  // Two leaves of equal frequency: the reference compares the dictionary index
  //   (*(a->data) < *(b->data)), i.e. the storage order, not the symbol value.
  return na.order < nb.order;
}

void Huffman::build() {
  nodes.clear();
  codes.clear();
  root = -1;
  if (dict.empty())
    return;

  // One leaf per dictionary entry; its arena index doubles as the storage-order
  //   tie-break key, so it must mirror the on-disk dictionary order.
  nodes.reserve(2 * dict.size());
  for (int i = 0; i != static_cast<int>(dict.size()); ++i)
    nodes.push_back(Node{dict[i].frequency, -1, -1, dict[i].symbol, i});

  // The comparator dereferences live arena entries by index, so it remains valid
  //   across reallocation of the arena as internal nodes are appended.
  const auto compare = [this](const int x, const int y) { return node_less(x, y); };
  std::priority_queue<int, std::vector<int>, decltype(compare)> pqueue(compare);
  for (int i = 0; i != static_cast<int>(nodes.size()); ++i)
    pqueue.push(i);

  while (!pqueue.empty()) {
    const int top = pqueue.top();
    pqueue.pop();
    if (pqueue.empty()) {
      root = top;
      break;
    }
    const int top2 = pqueue.top();
    pqueue.pop();
    // left = first-popped (smaller), right = second-popped; left→0, right→1.
    //   An internal node's order key is unused (the tie-break recurses on children).
    nodes.push_back(Node{nodes[top].frequency + nodes[top2].frequency, top, top2, 0.0F, -1});
    pqueue.push(static_cast<int>(nodes.size()) - 1);
  }

  // Assign code words by an explicit-stack traversal (left→0, right→1), avoiding
  //   recursion that could be as deep as the symbol count for a degenerate tree.
  std::vector<std::pair<int, std::vector<bool>>> stack;
  stack.emplace_back(root, std::vector<bool>());
  while (!stack.empty()) {
    const std::pair<int, std::vector<bool>> entry = std::move(stack.back());
    stack.pop_back();
    const Node &node = nodes[entry.first];
    if (node.left < 0) {
      // A single-symbol dictionary yields an empty code word; encode() then emits
      //   no bits and decode() reproduces the symbol from the symbol count alone.
      codes[node.symbol] = entry.second;
      continue;
    }
    std::vector<bool> left_prefix = entry.second;
    left_prefix.push_back(false);
    std::vector<bool> right_prefix = entry.second;
    right_prefix.push_back(true);
    stack.emplace_back(node.left, std::move(left_prefix));
    stack.emplace_back(node.right, std::move(right_prefix));
  }
}

Huffman Huffman::from_histogram(const std::map<float, uint64_t> &histogram) {
  Huffman out;
  uint64_t total = 0;
  for (const auto &pair : histogram)
    total += pair.second;
  out.dict.reserve(histogram.size());
  // std::map iterates in ascending symbol order; the tree is order-independent,
  //   but a deterministic storage order keeps the on-disk dictionary stable.
  for (const auto &pair : histogram) {
    const float frequency = (total != 0) ? static_cast<float>(static_cast<double>(pair.second) / //
                                                              static_cast<double>(total))
                                         : 0.0F;
    out.dict.push_back(DictEntry{pair.first, frequency});
  }
  out.build();
  return out;
}

Huffman Huffman::from_dictionary(std::vector<DictEntry> dictionary) {
  Huffman out;
  // Preserve the on-disk dictionary order: it is the reference encoder's leaf
  //   storage index, which the equal-frequency tie-break depends upon. Re-sorting
  //   here would desynchronise the rebuilt tree from the writer's.
  out.dict = std::move(dictionary);
  out.build();
  return out;
}

Huffman::Encoded Huffman::encode(const std::vector<float> &symbols) const {
  Encoded out;
  out.bit_count = 0;
  uint8_t current = 0;
  int bit_index = 0;
  for (const float symbol : symbols) {
    auto it = codes.find(symbol);
    if (it == codes.end())
      throw Exception("Huffman encode: symbol absent from dictionary");
    for (const bool bit : it->second) {
      if (bit)
        current |= static_cast<uint8_t>(1U << bit_index);
      ++out.bit_count;
      bit_index = (bit_index + 1) % 8;
      if (bit_index == 0) {
        out.bytes.push_back(static_cast<std::byte>(current));
        current = 0;
      }
    }
  }
  // Flush a trailing partial byte; its unused high bits are zero and are ignored
  //   on decode (decoding stops at the expected symbol count).
  if (bit_index != 0)
    out.bytes.push_back(static_cast<std::byte>(current));
  return out;
}

std::vector<float> Huffman::decode(const std::byte *data, const size_t num_bytes, const size_t symbol_count) const {
  std::vector<float> out;
  if (symbol_count == 0)
    return out;
  if (root < 0)
    throw Exception("Huffman decode: empty dictionary but " + str(symbol_count) + " symbols expected");
  out.reserve(symbol_count);

  // Degenerate single-symbol tree: the root is a leaf and codes are empty.
  if (nodes[root].left < 0) {
    out.assign(symbol_count, nodes[root].symbol);
    return out;
  }

  int node = root;
  for (size_t byte_index = 0; byte_index != num_bytes && out.size() != symbol_count; ++byte_index) {
    const uint8_t value = static_cast<uint8_t>(data[byte_index]);
    for (int bit = 0; bit != 8 && out.size() != symbol_count; ++bit) {
      const bool set = ((value >> bit) & 1U) != 0U;
      node = set ? nodes[node].right : nodes[node].left;
      if (nodes[node].left < 0) {
        out.push_back(nodes[node].symbol);
        node = root;
      }
    }
  }
  if (out.size() != symbol_count)
    throw Exception("Huffman decode: stream exhausted after " + str(out.size()) + " of " + //
                    str(symbol_count) + " expected symbols");
  return out;
}

} // namespace MR::DWI::Tractography::Compression
