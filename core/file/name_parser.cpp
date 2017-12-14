/* Copyright (c) 2008-2017 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#include <algorithm>

#include "file/name_parser.h"

namespace MR
{
  namespace File
  {

    namespace
    {

      inline bool in_seq (const vector<int>& seq, int val)
      {
        if (seq.size() == 0) 
          return true;
        for (size_t i = 0; i < seq.size(); i++)
          if (seq[i] == val)
            return true;
        return false;
      }


    }






    void NameParser::parse (const std::string& imagename, size_t max_num_sequences)
    {
      specification = imagename;
      if (Path::is_dir (imagename)) {
        array.resize (1);
        array[0].set_str (imagename);
        return;
      }

      folder_name = Path::dirname (specification);


      try {
        std::string::size_type pos;
        std::string basename = Path::basename (specification);
        size_t num = 0;

        while ( (pos = basename.find_last_of (']')) < std::string::npos && num < max_num_sequences) {
          insert_str (basename.substr (pos+1));
          basename = basename.substr (0, pos);
          if ( (pos = basename.find_last_of ('[')) == std::string::npos)
            throw Exception ("malformed image sequence specifier for image \"" + specification + "\"");

          insert_seq (basename.substr (pos+1));
          num++;
          basename = basename.substr (0, pos);
        }

        insert_str (basename);


        for (size_t i = 0; i < array.size(); i++)
          if (array[i].is_sequence())
            if (array[i].sequence().size())
              for (size_t n = 0; n < array[i].sequence().size()-1; n++)
                for (size_t m = n+1; m < array[i].sequence().size(); m++)
                  if (array[i].sequence() [n] == array[i].sequence() [m])
                    throw Exception ("malformed image sequence specifier for image \""
                        + specification + "\" (duplicate indices)");
      }
      catch (...) {
        array.resize (1);
        array[0].set_str (imagename);
        throw;
      }
    }





    std::ostream& operator<< (std::ostream& stream, const NameParser::Item& item)
    {
      if (item.is_string()) 
        stream << "\"" << item.string() << "\"";
      else {
        if (item.sequence().size()) 
          stream << item.sequence();
        else 
          stream << "[ any ]";
      }
      return stream;
    }





    std::ostream& operator<< (std::ostream& stream, const NameParser& parser)
    {
      stream << "File::NameParser: " << parser.specification << "\n";
      for (size_t i = 0; i < parser.array.size(); i++)
        stream << "  " << i << ": " << parser.array[i] << "\n";
      return stream;
    }










    bool NameParser::match (const std::string& file_name, vector<int>& indices) const
    {
      int current = 0;
      size_t num = 0;
      indices.resize (seq_index.size());

      for (size_t i = 0; i < array.size(); i++) {
        if (array[i].is_string()) {
          if (file_name.substr (current, array[i].string().size()) != array[i].string()) 
            return false;
          current += array[i].string().size();
        }
        else {
          int x = current;
          while (isdigit (file_name[current]))
            current++;
          if (x == current) 
            return false;
          x = to<int> (file_name.substr (x, current-x));
          if (!in_seq (array[i].sequence(), x)) 
            return false;
          indices[num] = x;
          num++;
        }
      }
      return true;
    }




    void NameParser::calculate_padding (const vector<int>& maxvals)
    {
      assert (maxvals.size() == seq_index.size());
      for (size_t n = 0; n < seq_index.size(); n++)
        assert (maxvals[n] > 0);

      for (size_t n = 0; n < seq_index.size(); n++) {
        size_t m = seq_index.size() - 1 - n;
        Item& item (array[seq_index[n]]);
        if (item.sequence().size()) {
          if (maxvals[m])
            if (item.sequence().size() != (size_t) maxvals[m])
              throw Exception ("dimensions requested in image specifier \"" + specification
                               + "\" do not match supplied header information");
        }
        else {
          item.sequence().resize (maxvals[m]);
          for (size_t i = 0; i < item.sequence().size(); i++)
            item.sequence() [i] = i;
        }

        item.calc_padding (maxvals[m]);
      }
    }





    void NameParser::Item::calc_padding (size_t maxval)
    {
      for (size_t i = 0; i < sequence().size(); i++) {
        assert (sequence() [i] >= 0);
        if (maxval < (size_t) sequence() [i]) maxval = sequence() [i];
      }

      seq_length = 1;
      for (size_t num = 10; maxval >= num; num *= 10)
        seq_length += 1;
    }





    std::string NameParser::name (const vector<int>& indices)
    {
      if (!seq_index.size())
        return Path::join (folder_name, array[0].string());

      assert (indices.size() == seq_index.size());

      std::string str;
      size_t n = seq_index.size()-1;
      for (size_t i = 0; i < array.size(); i++) {
        if (array[i].is_string()) str += array[i].string();
        else {
          str += printf ("%*.*d", array[i].size(), array[i].size(), array[i].sequence() [indices[n]]);
          n--;
        }
      }

      return Path::join (folder_name, str);
    }







    std::string NameParser::get_next_match (vector<int>& indices, bool return_seq_index)
    {
      if (!folder)
        folder.reset (new Path::Dir (folder_name));

      std::string fname;
      while ( (fname = folder->read_name()).size()) {
        if (match (fname, indices)) {
          if (return_seq_index) {
            for (size_t i = 0; i < ndim(); i++) {
              if (sequence (i).size()) {
                size_t n = 0;
                while (indices[i] != sequence (i) [n]) n++;
                indices[i] = n;
              }
            }
          }
          return Path::join (folder_name, fname);
        }
      }

      return "";
    }


    bool ParsedName::operator< (const ParsedName& pn) const
    {
      for (size_t i = 0; i < ndim(); i++)
        if (index (i) != pn.index (i))
          return (index (i) < pn.index (i));
      return false;
    }




    vector<int> ParsedName::List::parse_scan_check (const std::string& specifier, size_t max_num_sequences)
    {
      NameParser parser;
      parser.parse (specifier);

      scan (parser);
      std::sort (list.begin(), list.end(), compare_ptr_contents());
      vector<int> dim = count();

      for (size_t n = 0; n < dim.size(); n++)
        if (parser.sequence (n).size())
          if (dim[n] != (int) parser.sequence (n).size())
            throw Exception ("number of files found does not match specification \"" + specifier + "\"");

      return dim;
    }






    void ParsedName::List::scan (NameParser& parser)
    {
      vector<int> index;
      if (parser.ndim() == 0) {
        list.push_back (std::shared_ptr<ParsedName> (new ParsedName (parser.name (index), index)));
        return;
      }

      std::string entry;

      while ( (entry = parser.get_next_match (index, true)).size())
        list.push_back (std::shared_ptr<ParsedName> (new ParsedName (entry, index)));

      if (!size())
        throw Exception ("no matching files found for image specifier \"" + parser.spec() + "\"");
    }






    vector<int> ParsedName::List::count () const
    {
      if (! list[0]->ndim()) {
        if (size() == 1) return (vector<int>());
        else throw Exception ("image number mismatch");
      }

      vector<int> dim ( list[0]->ndim(), 0);
      size_t current_entry = 0;

      count_dim (dim, current_entry, 0);

      return dim;
    }




    void ParsedName::List::count_dim (vector<int>& dim, size_t& current_entry, size_t current_dim) const
    {
      int n;
      bool stop = false;
      std::shared_ptr<const ParsedName> first_entry ( list[current_entry]);

      for (n = 0; current_entry < size(); n++) {
        for (size_t d = 0; d < current_dim; d++)
          if ( list[current_entry]->index (d) != first_entry->index (d)) 
            stop = true;
        if (stop) 
          break;

        if (current_dim < list[0]->ndim()-1)
          count_dim (dim, current_entry, current_dim+1);
        else current_entry++;
      }

      if (dim[current_dim] && dim[current_dim] != n)
        throw Exception ("number mismatch between number of images along different dimensions");

      dim[current_dim] = n;
    }






    std::ostream& operator<< (std::ostream& stream, const ParsedName::List& list) 
    {
      stream << "parsed name list, size " << list.size() << ", counts " << list.count() << "\n";
      for (const auto& entry : list.list)
        stream << *entry << "\n";
      return stream;
    }







    std::ostream& operator<< (std::ostream& stream, const ParsedName& pin)
    {
      stream << "[ ";
      for (size_t n = 0; n < pin.ndim(); n++)
        stream << pin.index (n) << " ";
      stream << "] " << pin.name();
      return (stream);
    }




  }
}
