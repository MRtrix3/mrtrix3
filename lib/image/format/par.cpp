/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

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


#include "file/config.h"
#include "file/ofstream.h"
#include "file/path.h"
#include "file/utils.h"
#include "file/mmap.h"
#include "image/utils.h"
#include "image/format/list.h"
#include "image/header.h"
#include "image/handler/default.h"
#include "get_set.h"
    
#include "file/par_utils.h"

#include "image/format/mrtrix_utils.h"
#include <tuple>

#include <iterator>
#include <algorithm>
// #include <string>



namespace MR
{
  namespace Image
  {
    namespace Format
    {
      template<typename T>
      std::vector<T> split_image_line(const std::string& line) {
          std::istringstream is(line);
          return std::vector<T>(std::istream_iterator<T>(is), std::istream_iterator<T>());
      }

      typedef double ComputeType;

      // typedef std::vector<int>::const_iterator Input_iterator;
      typedef std::map<std::string, std::string>              ParHeader;

      typedef std::tuple<size_t, size_t, std::string>         ParCol;
      typedef std::map<std::string, ParCol>                   ParImageInfo;
      typedef std::map<std::string, std::vector<std::string>> ParImages; 
      // TOOD ParImages: use different types?      
      // TOOD ParImages: use array?      

      RefPtr<Handler::Base> PAR::read (Header& H) const
      {
        if (!Path::has_suffix (H.name(), ".PAR") && !Path::has_suffix (H.name(), ".par")){
          return RefPtr<Handler::Base>();
        }
        MR::File::PAR::KeyValue kv (H.name());
        
        ParHeader PH;             // General information
        ParImageInfo image_info;  // image column info
        ParImages images;         // columns
        
        while (kv.next_general()){
          std::pair<ParHeader::iterator, bool> res = PH.insert(std::make_pair(kv.key(), kv.value()));
          if ( ! res.second )
            WARN("ParHeader key " +  kv.key() + " defined multiple times. Using: " + (res.first)->second);
        }
        PH.insert(std::make_pair("version", kv.version()));

        for (auto& item: PH) {
          DEBUG(item.first + ":" + item.second);
        }

        if (!kv.valid_version()){
          WARN("par/rec file " + H.name() + " claims to be of version '" + 
            kv.version() + "' which is not supported. You've got to ask yourself one question: Do I feel lucky?");
        }

        size_t cnt=0;
        while (kv.next_image_information ()){
          int extent = 1;
          std::string type = kv.value();
          { 
            size_t star = kv.value().find_first_of ('*');
            if (star != std::string::npos){
              extent = std::stoi(kv.value().substr(0, star)) ;
              type = kv.value().substr(star+1);
            }
          }
          image_info.insert(std::make_pair(kv.key(), std::make_tuple(cnt, cnt+extent, type)));
          cnt++;
        }
        // image_info.insert(std::make_pair("volume number", std::make_tuple(-1, -1, "integer")));
        DEBUG("image column info:");
        for (auto& item: image_info) {
          DEBUG(item.first + ": columns "  + 
            str(std::get<0>(item.second)) + "-"  +
            str(std::get<1>(item.second)-1) + " " +
            std::get<2>(item.second)); 
        }

        std::vector<std::string> vec;
        std::map<std::string, size_t> image_number_counter;
        while (kv.next_image()){
          vec = split_image_line<std::string>(kv.value());
          image_number_counter[vec[0]]++;

          for (auto& item: image_info) {
            size_t start_col = std::get<0>(item.second);
            size_t stop_col =  std::get<1>(item.second);
            // TODO how to handle multiple rows in a nicer way?
            std::string s;
            std::for_each(vec.begin()+start_col, vec.begin()+stop_col, [&](const std::string &piece){ s += piece; s += " "; });
            images[item.first].push_back(s);
          }
          // calculate volume numbers inferred from slice number
          // std::string volume_number = std::count (images["slice number"].begin(), images["slice number"].end(), vec[0]);
          images["volume number"].push_back(str(image_number_counter[vec[0]]));
        }
        // TODO check slice_numbers against "Max. number of slices/locations" ... see _truncation_checks
        
        // for (auto& item: images)
          // DEBUG(item.first + ": " +str(item.second));

        // convert strVec to float:
        // std::vector<float> flVect(strVect.size());
        // std::transform(strVect.begin(), strVect.end(), flVect.begin(), 
                       // [](const std::string &arg) { return std::stof(arg); });


        File::MMap fmap (H.name());
        std::cerr << fmap << std::endl;
        size_t data_offset = 0; // File::PAR::read (H, * ( (const par_header*) fmap.address()));
        // size_t data_offset = 0;
        throw Exception ("par/rec not yet implemented... \"" + H.name() + "\"");

        RefPtr<Handler::Base> handler (new Handler::Default (H));
        handler->files.push_back (File::Entry (H.name(), data_offset));

        return handler;
      }


      bool PAR::check (Header& H, size_t num_axes) const
      {
        return false;
      }

      RefPtr<Handler::Base> PAR::create (Header& H) const
      {
        assert (0);
        return RefPtr<Handler::Base>();
      }

    }
  }
}



