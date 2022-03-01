/* Copyright (c) 2008-2021 the MRtrix3 contributors.
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


#include "file/npy.h"



namespace MR
{
  namespace File
  {
    namespace NPY
    {



      DataType descr2datatype (const std::string& s)
      {
        size_t type_offset = 0;
        bool is_little_endian = true;
        bool expect_one_byte_width = false;
        bool issue_endianness_warning = false;
        if (s[0] == '<') {
          type_offset = 1;
        } else if (s[0] == '>') {
          is_little_endian = false;
          type_offset = 1;
        } else if (s[0] == '|') {
          expect_one_byte_width = true;
        } else {
          issue_endianness_warning = true;
        }
        size_t bytes = 0;
        if (s.size() > type_offset + 1) {
          try {
            bytes = to<size_t> (s.substr (type_offset+1));
          } catch (Exception& e) {
            throw Exception ("Invalid byte width specifier \"" + s.substr (type_offset+1) + "\"");
          }
        }
        DataType data_type;
        switch (s[type_offset]) {
          case '?':
            data_type = DataType::Bit;
            if (bytes && bytes > 1)
              throw Exception ("Unexpected byte width (" + str(bytes) + ") for bitwise data");
            break;
          case 'b':
            data_type = DataType::Int8;
            if (bytes && bytes > 1)
              throw Exception ("Unexpected byte width (" + str(bytes) + ") for signed byte data");
            break;
          case 'B':
            data_type = DataType::UInt8;
            if (bytes && bytes > 1)
              throw Exception ("Unexpected byte width (" + str(bytes) + ") for unsigned byte data");
            break;
          case 'h':
            data_type = DataType::Int16;
            if (bytes && bytes != 2)
              throw Exception ("Unexpected byte width (" + str(bytes) + ") for signed short integer data");
            break;
          case 'H':
            data_type = DataType::UInt16;
            if (bytes && bytes != 2)
              throw Exception ("Unexpected byte width (" + str(bytes) + ") for unsigned short integer data");
            break;
          case 'i':
            switch (bytes) {
              case 1: data_type = DataType::Int8;  break;
              case 2: data_type = DataType::Int16; break;
              case 4: data_type = DataType::Int32; break;
              case 8: data_type = DataType::Int64; break;
              default: throw Exception ("Unexpected bit width (" + str(bytes) + ") for signed integer data");
            }
            break;
          case 'u':
            switch (bytes) {
              case 1: data_type = DataType::UInt8;  break;
              case 2: data_type = DataType::UInt16; break;
              case 4: data_type = DataType::UInt32; break;
              case 8: data_type = DataType::UInt64; break;
              default: throw Exception ("Unexpected bit width (" + str(bytes) + ") for unsigned integer data");
            }
            break;
          case 'e':
            data_type = DataType::Float16;
            break;
          case 'f':
            switch (bytes) {
              case 2: data_type = DataType::Float16; break;
              case 4: data_type = DataType::Float32; break;
              case 8: data_type = DataType::Float64; break;
              default: throw Exception ("Unexpected bit width (" + str(bytes) + ") for floating-point data");
            }
            break;
          default:
            throw Exception (std::string ("Unsupported data type indicator \'") + s[type_offset] + "\'");
        }
        if (data_type.bytes() != 1 && expect_one_byte_width)
          throw Exception ("Inconsistency in byte width specification (expected one byte; got " + str(data_type.bytes()) + ')');
        if (bytes > 1)
          data_type = data_type() | (is_little_endian ? DataType::LittleEndian : DataType::BigEndian);
        if (issue_endianness_warning) {
           WARN (std::string ("NumPy file does not indicate data endianness; assuming ") + (MRTRIX_IS_BIG_ENDIAN ? "big" : "little") + "-endian (same as system)");
        }
        return data_type;
      }



      std::string datatype2descr (const DataType data_type)
      {
        std::string descr;
        if (data_type.bytes() > 1) {
          if (data_type.is_big_endian())
            descr.push_back ('>');
          else if (data_type.is_little_endian())
            descr.push_back ('<');
          // Otherwise, use machine endianness, and don't explicitly flag in the file
        }
        if (data_type.is_integer()) {
          descr.push_back (data_type.is_signed() ? 'i' : 'u');
        } else if (data_type.is_floating_point()) {
          descr.push_back (data_type.is_complex() ? 'c' : 'f');
        } else if (data_type == DataType::Bit) {
          descr.push_back ('?');
          return descr;
        }
        descr.append (str(data_type.bytes()));
        return descr;
      }



      //CONF option: NPYFloatMaxSavePrecision
      //CONF default: 64
      //CONF When exporting floating-point data to NumPy .npy format, do not
      //CONF use a precision any greater than this value in bits (used to
      //CONF minimise file size). Must be equal to either 16, 32 or 64.
      size_t float_max_save_precision()
      {
        static size_t result = to<size_t> (File::Config::get ("NPYFloatMaxSavePrecision", "64"));
        if (!(result == 16 || result == 32 || result == 64))
          throw Exception ("Invalid value for config file entry \"NPYFloatMaxSavePrecision\" (must be 16, 32 or 64)");
        return result;
      }




      KeyValues parse_header (std::string s)
      {
        if (s[s.size()-1] == '\n')
          s.resize (s.size()-1);
        // Remove trailing space-padding
        s = strip (s, " ", false, true);
        // Expect a dictionary literal; remove curly braces
        s = strip (strip (s, "{", true, false), "}", false, true);
        const std::map<char, char> pairs { {'{', '}'}, {'[', ']'}, {'(', ')'}, {'\'', '\''}, {'"', '"'}};
        vector<char> openers;
        bool prev_was_escape = false;
        std::string current, key;
        KeyValues keyval;
        for (const auto c : s) {
          //std::cerr << "Openers: [" << join(openers, ",") << "]; prev_was_escape = " << str(prev_was_escape) << "; current: " << current << "; key: " << key << "\n";
          if (prev_was_escape) {
            current.push_back (c);
            prev_was_escape = false;
            continue;
          }
          if (c == ' ' && current.empty())
            continue;
          if (c == '\\') {
            if (prev_was_escape) {
              current.push_back ('\\');
              prev_was_escape = false;
            } else {
              prev_was_escape = true;
            }
            continue;
          }
          if (openers.size()) {
            if (c == pairs.find(openers.back())->second)
              openers.pop_back();
            // If final opener is a string quote, and it's now being closed, 
            //   want to capture anything until the corresponding close,
            //   regardless of whether or not it is itself an opener
            // For other openers, a new opener can stack, and so we
            //   don't want to immediately call continue
            if (c == '\'' || c == '\"') {
              current.push_back (c);
              continue;
            }
          } else if (c == ':') {
            if (key.size())
              throw Exception ("Error parsing NumPy header: non-isolated colon separator");
            if ((current.front() == '\"' && current.back() == '\"') || (current.front() == '\'' && current.back() == '\''))
              key = current.substr(1, current.size() - 2);
            else
              key = current;
            if (keyval.find (key) != keyval.end())
              throw Exception ("Error parsing NumPy header: duplicate key");
            current.clear();
            continue;
          } else if (c == ',') {
            if (key.empty())
              throw Exception ("Error parsing NumPy header: colon separator with no colon-separated key beforehand");
            if ((current.front() == '\"' && current.back() == '\"') || (current.front() == '\'' && current.back() == '\''))
              current = current.substr(1, current.size() - 2);
            keyval[key] = current;
            key.clear();
            current.clear();
            continue;
          }
          if (pairs.find (c) != pairs.end())
            openers.push_back (c);
          current.push_back (c);
        }
        if (openers.size())
          throw Exception ("Error parsing NumPy header: unpaired bracket or quotation symbol(s) at EOF");
        if (key.size())
          throw Exception ("Error parsing NumPy header: key without associated value at EOF");
        current = strip (current, " ,");
        if (current.size())
          throw Exception ("Error parsing NumPy header: non-empty content at EOF");

        std::cerr << "Final keyvalues: {";
        for (const auto& kv : keyval)
          std::cerr << " '" + kv.first + "': '" + kv.second + "', ";
        std::cerr << "}\n";
        return keyval;
      }




    }
  }
}
