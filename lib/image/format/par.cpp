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
#include "file/par.h" // not used yet

#include "image/format/mrtrix_utils.h"
#include <tuple>

#include <iterator>
#include <set>
#include <algorithm>
#include <memory>
// #include <unordered_map>
// #include <string>



namespace MR
{
  namespace Image
  {
    namespace Format
    {
      // template<typename T, typename... Args>
      // T intersect(T first, Args... args) {
      //   return first + intersect(args...);
      // }
      // std::set<size_t>  a = get_matching_indices(images[vUID[2]],std::string(images[vUID[2]][0]));
      // std::set<size_t>  b = get_matching_indices(images[vUID[1]],std::string(images[vUID[1]][0]));

      // std::set<size_t>  uni;
      // std::set_intersection (a.begin(), a.end(),
      //            b.begin(), b.end(),
      //            std::inserter(uni, uni.begin()));
      // std::cerr << vUID[2] + "="  + images[vUID[2]][0] + ", " + vUID[1] + "="  + images[vUID[1]][0] + " has indices ";
      // std::copy(uni.begin(), uni.end(), std::ostream_iterator<size_t>(std::cerr, " "));
      // std::cerr << std::endl;

      // if (map.count(key) == 1)
      //   return map.at(key);
      // else 
      //   return nullptr;
      template<typename T>
      std::set<size_t> get_matching_indices(const std::vector<T>& v, const T& criterion){
        std::set<size_t> indices;
        auto it = std::find_if(std::begin(v), std::end(v), [&](T i){return i == criterion;});
        while (it != std::end(v)) {
          // indices.emplace_back(std::distance(std::begin(v), it));
          indices.insert(std::distance(std::begin(v), it));
          it = std::find_if(std::next(it), std::end(v), [&](T i){return i == criterion;});
        }
        return indices;
      }

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

      // todo check file existence
      RefPtr<Handler::Base> PAR::read (Header& H) const
      {
        if (!Path::has_suffix (H.name(), ".PAR") && !Path::has_suffix (H.name(), ".par")){
          return RefPtr<Handler::Base>();
        }
        std::string rec_file = H.name().substr(0,H.name().size()-4)+".REC";

        MR::File::PAR::KeyValue kv (H.name());
        
        ParHeader PH;             // General information
        ParImageInfo image_info;  // image column info
        ParImages images;         // columns
        std::vector<std::pair<size_t,size_t>> slice_data_block_positions; 
        
        while (kv.next_general()){
          std::pair<ParHeader::iterator, bool> res = PH.insert(std::make_pair(kv.key(), kv.value()));
          if ( ! res.second )
            WARN("ParHeader key " +  kv.key() + " defined multiple times. Using: " + (res.first)->second);
        }

        for (auto& item: PH) {
          DEBUG(item.first + ":" + item.second);
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
          cnt+=extent;
        }

        // show some info about the data
        {
          std::vector<std::string> v = {"Patient position", 
                                        "Preparation direction", 
                                        "FOV (ap,fh,rl) [mm]",
                                        "Technique",
                                        "Protocol name",
                                        "Dynamic scan      <0=no 1=yes> ?",
                                        "Diffusion         <0=no 1=yes> ?"};
          auto it = std::max_element(v.begin(), v.end(), [](const std::string& x, const std::string& y) {  return x.size() < y.size(); });
          size_t padding = (*it).size();
          for (auto& k : v){
            if (PH.find(k) != PH.end()){
              INFO(k.insert(k.size(), padding - k.size(), ' ') + ": " + PH[k]);            
            }
            else{
              WARN("PAR header lacks '" + k +"' field." );
            }
          }
        }

        // check version
        { 
        if (!kv.valid_version())
          WARN("par/rec file " + H.name() + " claims to be of version '" + 
            kv.version() + "' which is not supported. You've got to ask yourself one question: Do I feel lucky?");
          size_t number_of_columns=0; 
          for (auto& item:image_info)
            number_of_columns = std::max(number_of_columns,std::get<1>(item.second));
          std::string version;
          if (number_of_columns <= 41)
            version = "V4";
          else if (number_of_columns <= 48)
            version = "V4.1";
          else
            version = "V4.2";
          if (kv.version() != version)
            WARN("number of columns in " + H.name() + " does not match version number: " + kv.version() + " (" + version + ")");
        }

        PH.insert(std::make_pair("version", kv.version()));

        // define what information we need for unique identifier
        std::vector<std::string> vUID;
        if (std::stoi(PH["Max. number of echoes"]) > 1)
          vUID.push_back("echo number");
        if (std::stoi(PH["Max. number of slices/locations"]) > 1)
          vUID.push_back("slice number");
        if (std::stoi(PH["Max. number of cardiac phases"]) > 1)
          vUID.push_back("cardiac phase number");
        if (std::stoi(PH["Max. number of dynamics"]) > 1)
          vUID.push_back("dynamic scan number");
        // 4.1
        if ((kv.version()=="V4.1" || kv.version()=="V4.2") && (std::stoi(PH["Max. number of diffusion values"]) > 1))
          vUID.push_back("gradient orientation number (imagekey!)");
        if ((kv.version()=="V4.1" || kv.version()=="V4.2") && (std::stoi(PH["Max. number of diffusion values"]) > 1))
          vUID.push_back("diffusion b value number    (imagekey!)");
        // 4.2
        if (kv.version()=="V4.2" && (std::stoi(PH["Number of label types   <0=no ASL>"]) > 1))
          vUID.push_back("label type (ASL)            (imagekey!)");
        vUID.push_back("image_type_mr");

        if (vUID.size()>1)
          WARN("Multiple volumes in file " + H.name() + ". Uid category indices required.");

        // parse image information and save it in images
        std::map<std::string, size_t> uid_tester;
        std::vector<std::string> vec;
        std::string uid; // minimum unique identifier for each slice
        std::string uid_cat;
        std::for_each(vUID.begin(), vUID.end(), [&](const std::string &piece){ uid_cat += piece; uid_cat += ";"; });
        uid_cat.pop_back();
        // INFO("uid categories: " + uid_cat);
        std::vector<std::vector<int> > uid_indices;
        size_t slice_data_block_start=0;
        while (kv.next_image()){
          vec = split_image_line<std::string>(kv.value());

          for (auto& item: image_info) {
            // stop_col - start_col >1 --> item spans multiple columns
            size_t start_col = std::get<0>(item.second);
            size_t stop_col =  std::get<1>(item.second);
            std::string s;
            std::for_each(vec.begin()+start_col, vec.begin()+stop_col, [&](const std::string &piece){ s += piece; s += " "; });
            images[item.first].push_back(s.substr(0, s.size()-1));
          }
          // slice_data_block_positions
          {
            // image = reshape(data(1+idx.*(x.*y):(idx+1).*x.*y),x,y);
            // size_t idx = std::stoi(images["index in REC file (in images)"].back());
            std::vector<size_t> xy = split_image_line<size_t>(images["recon resolution (x y)"].back());
            // size_t start = idx*(xy[0] *xy[1]);
            // size_t stop = (idx+1)*(xy[0] *xy[1]);
            size_t stop = slice_data_block_start + (xy[0] *xy[1]);
            slice_data_block_positions.push_back(std::pair<size_t,size_t>(slice_data_block_start,stop));
            slice_data_block_start = stop;
          }


          uid.clear();
          for (auto& k : vUID)    
            uid += images[k].back() + " ";
          uid.pop_back();
          // INFO("uid: " + uid);
          ++uid_tester[uid];
          if (uid_tester[uid] > 1){
            WARN("uid not unique: " + uid_cat + ": " + uid);
          }
          uid_indices.push_back(split_image_line<int>(uid));
        }
        kv.close();
        INFO("uid categories: " + uid_cat);

        if (!std::all_of(images["recon resolution (x y)"].begin()+1,images["recon resolution (x y)"].end(),
          [&](const std::string & r) {return r==images["recon resolution (x y)"].front();}))
          throw Exception ("recon resolution (x y) not the same for all slices");
        

        // TODO separate uid code from image parsing

        // TODO: combine type values

        // TODO: user defined volume slicing via \a categories and \a values. here we choose an arbitrary volume
        std::vector<size_t>  chosen_slices;
        if (vUID.size() == 1) {
          chosen_slices.reserve(images[vUID[0]].size());
          std::iota(chosen_slices.begin(), chosen_slices.end(), 0);
        }
        else {         
          std::vector<std::string>  categories; // which categories in have to be unique (the remaining one can have any value)
          std::vector<std::string>  values;     // the value corresponding to categories

          // TODO: change this to user defined values
          categories.insert(categories.end(), vUID.begin()+1, vUID.end());
          for (auto cat : categories)
            values.push_back(images[cat][0]);
          VAR(categories);
          VAR(values);
          H.comments().push_back("categories:" + str(categories));
          H.comments().push_back("values:" + str(values));


          std::vector<std::set<size_t>> matching_lines_per_categ;
          for (size_t i=0; i< categories.size(); i++) {
            matching_lines_per_categ.push_back(std::set<size_t>(get_matching_indices(images[categories[i]],values[i]))); // TODO use pointers
          }
          std::map<size_t,size_t> intersect_counter;
          for (auto& lines: matching_lines_per_categ){
            for (auto& line :lines)
              intersect_counter[line] += 1;
          }

          for (auto& item: intersect_counter) {
            if (item.second == matching_lines_per_categ.size())
              chosen_slices.push_back(item.first);
          }

          H.set_ndim (3);
          H.dim(0) = split_image_line<size_t>(images["recon resolution (x y)"].back())[1];
          H.dim(1) = split_image_line<size_t>(images["recon resolution (x y)"].back())[0];
          H.dim(2) = chosen_slices.size();
          H.vox(0) = 1;
          H.vox(1) = 1;
          H.vox(2) = 1;
          H.datatype() = DataType::UInt16;
          H.datatype().set_byte_order_native();
          for (auto& item: PH)
            H[item.first] = item.second;

          // H.intensity_offset();
          // H.intensity_scale();

          // H.transform().allocate (4,4);
          // H.transform()(3,0) = H.transform()(3,1) = H.transform()(3,2) = 0.0;
          // H.transform()(3,3) = 1.0;
          // int count = 0;
          // for (int row = 0; row < 3; ++row)
          //   for (int col = 0; col < 4; ++col)
          //     H.transform() (row,col) = transform[count++];

          // std::set<size_t>  uni;
          // std::set<size_t>  a = get_matching_indices(images[vUID[0]],std::string(images[vUID[0]][0]));
          // std::set<size_t>  b = get_matching_indices(images[vUID[1]],std::string(images[vUID[1]][0]));
          // std::set_intersection (a.begin(), a.end(),
          //            b.begin(), b.end(),
          //            std::inserter(uni, uni.begin()));
          // std::cerr << vUID[2] + "="  + images[vUID[2]][0] + ", " + vUID[1] + "="  + images[vUID[1]][0] + " has indices ";
          // std::copy(uni.begin(), uni.end(), std::ostream_iterator<size_t>(std::cerr, " "));
          // std::cerr << std::endl;
        }

        RefPtr<Handler::Base> handler (new Handler::Default (H));

        { INFO("selected slices:");
          std::vector<std::string> image_info = {"image offcentre (ap,fh,rl in mm )"};
          image_info.insert(image_info.end(),vUID.begin(),vUID.end());
          // ,"slice number","echo number","dynamic scan number","image_type_mr"};
          for (auto& slice : chosen_slices){
            std::string s;
            for (auto& cat: image_info)
              s += cat +": " + images[cat][slice] + "\t";
            INFO(s + " (" + str(slice_data_block_positions[slice].first) + "," +str(slice_data_block_positions[slice].second) +")");
            handler->files.push_back (File::Entry (rec_file, 2*slice_data_block_positions[slice].first));
          }
        }

        // TODO truncation_checks: check slice_numbers against "Max. number of slices/locations" 
        // TODO dynamics with arbitrary start?
        

        // convert strVec to float:
        // std::vector<float> flVect(strVect.size());
        // std::transform(strVect.begin(), strVect.end(), flVect.begin(), 
                       // [](const std::string &arg) { return std::stof(arg); });


        // NIBABEL:        
        // "It seems that everyone agrees that Philips stores REC data in little-endian
        // format - see https://github.com/nipy/nibabel/issues/274

        // Philips XML header files, and some previous experience, suggest that the REC
        // data is always stored as 8 or 16 bit unsigned integers - see
        // https://github.com/nipy/nibabel/issues/275"


        // PV = pixel value in REC file, FP = floating point value, DV = displayed value on console
        // RS = rescale slope,           RI = rescale intercept,    SS = scale slope
        // DV = PV * RS + RI             FP = DV / (RS * SS)

        // File::MMap fmap (rec_file);
        // std::cerr << fmap << std::endl;

        // File::PAR::read (H, * ( (const par_header*) fmap.address()));

        // How to load the image data for non-contiguous data blocks?
        // e.g. (0,4096), (36864,40960), (73728,77824), ...
        // lib/image/handler/mosaic.cpp
        // Mosaic::load

        // RefPtr<Handler::Base> handler (new Handler::Default (H));

        // for (auto& data_block_position : slice_data_block_positions){
        //   size_t data_offset = data_block_position.first;
        //   handler->files.push_back (File::Entry (rec_file, data_offset));
        // }

        // throw Exception ("par/rec not yet implemented... \"" + H.name() + "\"");
        // for (size_t n = 0; n < chosen_slices.size(); ++n)
          // handler->files.push_back (File::Entry (frames[n]->filename, frames[n]->data))



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



