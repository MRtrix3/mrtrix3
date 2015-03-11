/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by Maximilian Pietsch, 03/03/15.

    This file is part of MRtrix.

    MRtrix is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MRtrix is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with MRtrix.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef __file_par_utils_h__
#define __file_par_utils_h__

// #include "file/par.h"
#include "math/matrix.h"

#include <fstream>
#include "mrtrix.h"

#include <sstream>
#include <vector>
#include <iterator>

// #include <regex>

namespace MR
{
  namespace Image
  {
    class Header;
  }
  namespace File
  {
    namespace PAR
    {       
      class KeyValue
      {
        // KeyValue: use KeyValue.next_general() to extract general information followed by KeyValue.next_image() for image lines
        public:
          KeyValue () { }
          KeyValue (const std::string& file, const char* first_line = nullptr) {
            open (file, first_line);
          }

          void  open (const std::string& file, const char* first_line = nullptr);
          bool  next_general ();
          bool  next_image ();
          bool  next_image_information ();
          void  close () {
            in.close();
          }
          // std::vector<T> split_image_line(const std::string&);
          
          const std::string& version() const throw () {
            return (ver);
          }
          const std::string& key () const throw ()   {
            return (K);
          }
          const std::string& value () const throw () {
            return (V);
          }
          const std::string& name () const throw ()  {
            return (filename);
          }
          const bool is_general () const throw ()   {
            return (general_information);
          }
          const bool valid_version() const throw () {
            return (std::find(understood_versions.begin(), understood_versions.end(), ver) != understood_versions.end());
          }

        private:
          std::string trim(std::string const& str, char leading_char = '.'); 

          bool general_information = true;

        protected:
          std::string K, V, filename, ver;
          std::ifstream in;
          const std::vector<std::string> understood_versions {
            "V4",
            "V4.1",
            "V4.2"};
      };

      // Math::Matrix<float> adjust_transform (const Image::Header& H, std::vector<size_t>& order);

      // void check (Image::Header& H, bool single_file);
      // size_t read (Image::Header& H, const par_header& PH);
      // void check (Image::Header& H, bool single_file);
      // void write (par_header& PH, const Image::Header& H, bool single_file);



    }
  }
}

#endif

