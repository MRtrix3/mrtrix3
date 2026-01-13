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

#include <stdint.h>
#include <string>

namespace MR::File::Dicom {

constexpr uint16_t VR_OB = 0x4F42U;
constexpr uint16_t VR_OW = 0x4F57U;
constexpr uint16_t VR_OF = 0x4F46U;
constexpr uint16_t VR_SQ = 0x5351U;
constexpr uint16_t VR_UN = 0x554EU;
constexpr uint16_t VR_AE = 0x4145U;
constexpr uint16_t VR_AS = 0x4153U;
constexpr uint16_t VR_AT = 0x4154U;
constexpr uint16_t VR_CS = 0x4353U;
constexpr uint16_t VR_DA = 0x4441U;
constexpr uint16_t VR_DS = 0x4453U;
constexpr uint16_t VR_DT = 0x4454U;
constexpr uint16_t VR_FD = 0x4644U;
constexpr uint16_t VR_FL = 0x464CU;
constexpr uint16_t VR_IS = 0x4953U;
constexpr uint16_t VR_LO = 0x4C4FU;
constexpr uint16_t VR_LT = 0x4C54U;
constexpr uint16_t VR_PN = 0x504EU;
constexpr uint16_t VR_SH = 0x5348U;
constexpr uint16_t VR_SL = 0x534CU;
constexpr uint16_t VR_SS = 0x5353U;
constexpr uint16_t VR_ST = 0x5354U;
constexpr uint16_t VR_TM = 0x544DU;
constexpr uint16_t VR_UI = 0x5549U;
constexpr uint16_t VR_UL = 0x554CU;
constexpr uint16_t VR_US = 0x5553U;
constexpr uint16_t VR_UT = 0x5554U;

constexpr uint16_t group_byte_order = 0x0002U;
constexpr uint16_t group_byte_order_swapped = 0x0200U;
constexpr uint16_t group_sequence = 0xFFFEU;
constexpr uint16_t group_data = 0x7FE0U;

constexpr uint16_t element_transfer_syntax_uid = 0x0010U;
constexpr uint16_t element_sequence_item = 0xE000U;
constexpr uint16_t element_sequence_delimitation_item = 0xE0DDU;
constexpr uint16_t element_data = 0x0010U;

constexpr uint64_t undefined_length = 0xFFFFFFFFUL;

inline std::string format_date(std::string_view date) {
  if (date.empty() || date.size() < 8)
    return std::string(date);
  return std::string(date.substr(6, 2)) + "/" + std::string(date.substr(4, 2)) + "/" + std::string(date.substr(0, 4));
}

inline std::string format_time(std::string_view time) {
  if (time.empty())
    return std::string(time);
  return std::string(time.substr(0, 2)) + ":" + std::string(time.substr(2, 2)) + ":" + std::string(time.substr(4, 2));
}

inline std::string format_ID(std::string_view ID) {
  if (ID.empty())
    return std::string(ID);
  return "(" + std::string(ID) + ")";
}

} // namespace MR::File::Dicom
