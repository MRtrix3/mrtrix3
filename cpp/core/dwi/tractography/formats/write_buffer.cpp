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

#include "dwi/tractography/formats/write_buffer.h"

#include <algorithm>
#include <cassert>
#include <cstring>

#include "exception.h"

namespace MR::DWI::Tractography::Formats {

WriteBuffer::WriteBuffer(size_t capacity_bytes, size_t element_size)
    : element_size(element_size), buffer_size(0), counts_ptr(nullptr) {
  if (element_size == 0)
    throw Exception("write buffer element size must be non-zero");
  // Round the requested capacity down to a whole number of elements so that an
  //   individual element is never split across a commit boundary; guarantee
  //   room for at least one element.
  buffer_capacity = std::max<size_t>(1, capacity_bytes / element_size) * element_size;
  buffer.reset(new std::byte[buffer_capacity]);
}

WriteBuffer::~WriteBuffer() {
  try {
    commit();
  } catch (Exception &e) {
    Exception(e, "Tractography write buffer not properly finalised").display();
  }
}

void WriteBuffer::add(const std::byte *data, size_t size) {
  if (size == 0)
    return;

  // Commit the buffered data before it would overflow the capacity.
  if (buffer_size + size > buffer_capacity)
    commit();

  // A single append larger than the capacity grows the buffer to fit it; this
  //   preserves the invariant that one logical write is committed atomically.
  if (size > buffer_capacity) {
    buffer_capacity = ((size + element_size - 1) / element_size) * element_size;
    buffer.reset(new std::byte[buffer_capacity]);
    assert(buffer_size == 0);
  }

  std::memcpy(buffer.get() + buffer_size, data, size);
  buffer_size += size;
}

void WriteBuffer::commit() {
  if (buffer_size == 0)
    return;
  if (flush) {
    const Counts counts = (counts_ptr != nullptr) ? *counts_ptr : Counts{0, 0};
    flush(buffer.get(), buffer_size, counts);
  }
  buffer_size = 0;
}

} // namespace MR::DWI::Tractography::Formats
