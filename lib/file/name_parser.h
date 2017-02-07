/* Copyright (c) 2008-2017 the MRtrix3 contributors
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


#ifndef __file_name_parser_h__
#define __file_name_parser_h__

#include <memory>

#include "mrtrix.h"
#include "memory.h"
#include "file/path.h"

namespace MR
{
  namespace File
  {

    //! a class to interpret numbered filenames
    class NameParser { NOMEMALIGN
      public:
        class Item { NOMEMALIGN
          public:
            Item () : seq_length (0) { }

            void set_str (const std::string& s) {
              clear ();
              str = s;
            }

            void set_seq (const std::string& s) {
              clear ();
              if (s.size()) seq = parse_ints (s);
              seq_length = 1;
            }

            void clear () {
              str.clear();
              seq.clear();
              seq_length = 0;
            }

            std::string string () const {
              return (str);
            }

            const vector<int>& sequence () const {
              return (seq);
            }

            vector<int>& sequence () {
              return (seq);
            }

            bool is_string () const {
              return (seq_length == 0);
            }

            bool is_sequence () const {
              return (seq_length != 0);
            }

            size_t size () const {
              return (seq_length ? seq_length : str.size());
            }

            void calc_padding (size_t maxval = 0);

            friend std::ostream& operator<< (std::ostream& stream, const Item& item);

          protected:
            size_t seq_length;
            std::string str;
            vector<int> seq;
        };


        void parse (const std::string& imagename, size_t max_num_sequences = std::numeric_limits<size_t>::max());

        size_t num () const {
          return (array.size());
        }

        std::string spec () const {
          return (specification);
        }

        const Item& operator[] (size_t i) const {
          return (array[i]);
        }

        const vector<int>& sequence (size_t index) const {
          return (array[seq_index[index]].sequence());
        }

        size_t ndim () const {
          return (seq_index.size());
        }

        size_t index_of_sequence (size_t number = 0) const {
          return (seq_index[number]);
        }

        bool match (const std::string& file_name, vector<int>& indices) const;
        void calculate_padding (const vector<int>& maxvals);
        std::string name (const vector<int>& indices);
        std::string get_next_match (vector<int>& indices, bool return_seq_index = false);

        friend std::ostream& operator<< (std::ostream& stream, const NameParser& parser);

      private:
        vector<Item> array;
        vector<size_t> seq_index;
        std::string folder_name, specification, current_name;
        std::unique_ptr<Path::Dir> folder;

        void insert_str (const std::string& str) {
          Item item;
          item.set_str (str);
          array.insert (array.begin(), item);
        }

        void insert_seq (const std::string& str) {
          Item item;
          item.set_seq (str);
          array.insert (array.begin(), item);
          seq_index.push_back (array.size()-1);
        }
    };








    //! a class to hold a parsed image filename
    class ParsedName { NOMEMALIGN
      public:
        ParsedName (const std::string& name, const vector<int>& index) : indices (index), filename (name) { }

        //! a class to hold a set of parsed image filenames
        class List { NOMEMALIGN
          public:
            vector<int> parse_scan_check (const std::string& specifier, 
                size_t max_num_sequences = std::numeric_limits<size_t>::max());

            void scan (NameParser& parser);

            vector<int> count () const;

            size_t biggest_filename_size () const {
              return max_name_size;
            }

            size_t size () const { return list.size(); }

            const ParsedName& operator[] (size_t index) const { return *list[index]; }

          protected:
            vector<std::shared_ptr<ParsedName>> list;
            void count_dim (vector<int>& dim, size_t& current_entry, size_t current_dim) const;
            size_t max_name_size;
        };



        std::string name () const {
          return filename;
        }
        size_t ndim () const {
          return indices.size();
        }
        int index (size_t num) const {
          return indices[num];
        }

        bool operator< (const ParsedName& pn) const;
        friend std::ostream& operator<< (std::ostream& stream, const ParsedName& pin);

      protected:
        vector<int>    indices;
        std::string         filename;

    };

  }
}

#endif

