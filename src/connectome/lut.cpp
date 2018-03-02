/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
 */


#include "connectome/lut.h"

#include <fstream>

#include "mrtrix.h" // For strip()



namespace MR {
namespace Connectome {




LUT::LUT (const std::string& path) :
    exclusive (true)
{
  load (path);
}

void LUT::load (const std::string& path)
{
  file_format format = LUT_NONE;
  try {
    format = guess_file_format (path);
  } catch (Exception& e) {
    throw e;
  }
  std::ifstream in_lut (path, std::ios_base::in);
  if (!in_lut)
    throw Exception ("Unable to open lookup table file");
  std::string line;
  while (std::getline (in_lut, line)) {
    if (line.size() > 1 && line[0] != '#') {
      switch (format) {
        case LUT_BASIC:      parse_line_basic      (line); break;
        case LUT_FREESURFER: parse_line_freesurfer (line); break;
        case LUT_AAL:        parse_line_aal        (line); break;
        case LUT_ITKSNAP:    parse_line_itksnap    (line); break;
        case LUT_MRTRIX:     parse_line_mrtrix     (line); break;
        default: assert (0);
      }
    }
    line.clear();
  }
}




LUT::file_format LUT::guess_file_format (const std::string& path)
{

  class Column
  { NOMEMALIGN
    public:
      Column () :
          numeric (true),
          integer (true),
          min (std::numeric_limits<default_type>::infinity()),
          max (-std::numeric_limits<default_type>::infinity()),
          sum_lengths (0),
          count (0) { }

      void operator() (const std::string& entry)
      {
        try {
          const default_type value = to<default_type> (entry);
          min = std::min (min, value);
          max = std::max (max, value);
        } catch (...) {
          numeric = integer = false;
        }
        if (entry.find ('.') != std::string::npos)
          integer = false;
        sum_lengths += entry.size();
        ++count;
      }

      default_type mean_length() const { return sum_lengths / default_type(count); }
      bool is_numeric() const { return numeric; }
      bool is_integer() const { return integer; }
      bool is_unary_range_float() const { return is_numeric() && min >= 0.0 && max <= 1.0; }
      bool is_8bit() const { return is_integer() && min >= 0 && max <= 255; }

    private:
      bool numeric, integer;
      default_type min, max;
      size_t sum_lengths, count;
  };

  std::ifstream in_lut (path, std::ios_base::in);
  if (!in_lut)
    throw Exception ("Unable to open lookup table file");
  vector<Column> columns;
  std::string line;
  while (std::getline (in_lut, line)) {
    if (line.size() > 1 && line[0] != '#') {
      // Before splitting by whitespace, need to capture any strings that are
      //   encased within quotation marks
      auto split_by_quotes = split (line, "\"\'", false);
      if (!(split_by_quotes.size()%2))
        throw Exception ("Odd number of quotation marks in a line in LUT file \"" + Path::basename (path) + "\"");
      decltype(split_by_quotes) entries;
      for (size_t i = 0; i != split_by_quotes.size(); ++i) {
        // Every second line must be encased in quotation marks, and is
        //   therefore preserved without splitting
        if (i % 2) {
          entries.push_back (split_by_quotes[i]);
        } else {
          const auto block_split = split(split_by_quotes[i], "\t ", true);
          entries.insert (entries.end(), block_split.begin(), block_split.end());
        }
      }
      for (decltype(entries)::iterator i = entries.begin(); i != entries.end();) {
        if (!i->size() || (i->size() == 1 && std::isspace ((*i)[0])))
          i = entries.erase (i);
        else
          ++i;
      }
      if (entries.size()) {
        if (columns.size() && entries.size() != columns.size()) {
          Exception E ("Inconsistent number of columns in LUT file \"" + Path::basename (path) + "\"");
          E.push_back ("Initial contents contain " + str(columns.size()) + " columns, but following line contains " + str(entries.size()) + ":");
          E.push_back ("\"" + line + "\"");
          throw E;
        }
        if (columns.empty())
          columns.resize (entries.size());
        for (size_t c = 0; c != columns.size(); ++c)
          columns[c] (entries[c]);
      }
    }
  }

  // Make an assessment of the LUT format
  if (columns.size() == 2 &&
      columns[0].is_integer() &&
      !columns[1].is_numeric()) {
    DEBUG ("LUT file \"" + Path::basename (path) + "\" contains 1 integer, 1 string per line: Basic format");
    return LUT_BASIC;
  }
  if (columns.size() == 6 &&
      columns[0].is_integer() &&
      !columns[1].is_numeric() &&
      columns[2].is_8bit() &&
      columns[3].is_8bit() &&
      columns[4].is_8bit() &&
      columns[5].is_8bit()) {
    DEBUG ("LUT file \"" + Path::basename (path) + "\" contains 1 integer, 1 string, then 4 8-bit integers per line: Freesurfer format");
    return LUT_FREESURFER;
  }
  if (columns.size() == 3 &&
      !columns[0].is_numeric() &&
      !columns[1].is_numeric() &&
      columns[0].mean_length() < columns[1].mean_length() &&
      columns[2].is_integer()) {
    DEBUG ("LUT file \"" + Path::basename (path) + "\" contains 2 strings (shorter first), then an integer per line: AAL format");
    return LUT_AAL;
  }
  if (columns.size() == 8 &&
      columns[0].is_integer() &&
      columns[1].is_8bit() &&
      columns[2].is_8bit() &&
      columns[3].is_8bit() &&
      columns[4].is_unary_range_float() &&
      columns[5].is_integer() &&
      columns[6].is_integer() &&
      !columns[7].is_numeric()) {
    DEBUG ("LUT file \"" + Path::basename (path) + "\" contains an integer, 3 8-bit integers, a float, two integers, and a string per line: ITKSNAP format");
    return LUT_ITKSNAP;
  }
  if (columns.size() == 7 &&
      columns[0].is_integer() &&
      !columns[1].is_numeric() &&
      !columns[2].is_numeric() &&
      columns[1].mean_length() < columns[2].mean_length() &&
      columns[3].is_8bit() &&
      columns[4].is_8bit() &&
      columns[5].is_8bit() &&
      columns[6].is_8bit()) {
    DEBUG ("LUT file \"" + Path::basename (path) + "\" contains 1 integer, 2 strings (shortest first), then 4 8-bit integers per line: MRtrix format");
    return LUT_MRTRIX;
  }
  throw Exception ("LUT file \"" + Path::basename (path) + "\" in unrecognized format");
  return LUT_NONE;
}






void LUT::parse_line_basic (const std::string& line)
{
  node_t index = std::numeric_limits<node_t>::max();
  char name [80];
  sscanf (line.c_str(), "%u %s", &index, name);
  if (index != std::numeric_limits<node_t>::max()) {
    const std::string strname (strip(name, " \t\n\""));
    check_and_insert (index, LUT_node (strname));
  }
}
void LUT::parse_line_freesurfer (const std::string& line)
{
  node_t index = std::numeric_limits<node_t>::max();
  node_t r = 256, g = 256, b = 256, a = 255;
  char name [80];
  sscanf (line.c_str(), "%u %s %u %u %u %u", &index, name, &r, &g, &b, &a);
  if (index != std::numeric_limits<node_t>::max()) {
    if (std::max ({r, g, b}) > 255)
      throw Exception ("Lookup table is malformed");
    const std::string strname (strip(name, " \t\n\""));
    check_and_insert (index, LUT_node (strname, r, g, b, a));
  }
}
void LUT::parse_line_aal (const std::string& line)
{
  node_t index = std::numeric_limits<node_t>::max();
  char short_name[20], name [80];
  sscanf (line.c_str(), "%s %s %u", short_name, name, &index);
  if (index != std::numeric_limits<node_t>::max()) {
    const std::string strshortname (strip(short_name, " \t\n\""));
    const std::string strname (strip(name, " \t\n\""));
    check_and_insert (index, LUT_node (strname, strshortname));
  }
}
void LUT::parse_line_itksnap (const std::string& line)
{
  node_t index = std::numeric_limits<node_t>::max();
  node_t r = 256, g = 256, b = 256;
  float a = 1.0;
  unsigned int label_vis = 0, mesh_vis = 0;
  char name [80];
  sscanf (line.c_str(), "%u %u %u %u %f %u %u %s", &index, &r, &g, &b, &a, &label_vis, &mesh_vis, name);
  if (index != std::numeric_limits<node_t>::max()) {
    std::string strname (strip(name, " \t\n\""));
    check_and_insert (index, LUT_node (strname, r, g, b, uint8_t(a*255.0)));
  }
}
void LUT::parse_line_mrtrix (const std::string& line)
{
  node_t index = std::numeric_limits<node_t>::max();
  node_t r = 256, g = 256, b = 256, a = 255;
  char short_name[20], name[80];
  sscanf (line.c_str(), "%u %s %s %u %u %u %u", &index, short_name, name, &r, &g, &b, &a);
  if (index != std::numeric_limits<node_t>::max()) {
    if (std::max ({r, g, b}) > 255)
      throw Exception ("Lookup table is malformed");
    const std::string strshortname (strip(short_name, " \t\n\""));
    const std::string strname (strip(name, " \t\n\""));
    check_and_insert (index, LUT_node (strname, strshortname, r, g, b, a));
  }
}





void LUT::check_and_insert (const node_t index, const LUT_node& data)
{
  if (find (index) != end())
    exclusive = false;
  insert (std::make_pair (index, data));
}





vector<node_t> get_lut_mapping (const LUT& in, const LUT& out)
{
  if (in.empty())
    return vector<node_t>();
  vector<node_t> map (in.rbegin()->first + 1, 0);
  for (const auto& node_in : in) {
    node_t target = 0;
    for (const auto& node_out : out) {
      if (node_out.second.get_name() == node_in.second.get_name()) {
        if (target) {
          throw Exception ("Cannot perform LUT conversion: Node " + str(node_in.first) + " (" + node_in.second.get_name() + ") has multiple possible targets");
          return vector<node_t> ();
        }
        target = node_out.first;
        break;
      }
    }
    if (target)
      map[node_in.first] = target;
  }
  return map;
}





}
}



