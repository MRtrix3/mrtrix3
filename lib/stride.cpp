/*
 * Copyright (c) 2008-2016 the MRtrix3 contributors
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/
 * 
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * 
 * For more details, see www.mrtrix.org
 * 
 */
#include "stride.h"

namespace MR
{

  namespace Stride
  {

    using namespace App;

    const OptionGroup Options = OptionGroup ("Stride options")
      + Option ("stride",
          "specify the strides of the output data in memory, as a comma-separated list. "
          "The actual strides produced will depend on whether the output image "
          "format can support it.")
      + Argument ("spec").type_sequence_int();






    List& sanitise (List& current, const List& desired, const vector<ssize_t>& dims)
    {
      // remove duplicates
      for (size_t i = 0; i < current.size()-1; ++i) {
        if (dims[i] == 1) current[i] = 0;
        if (!current[i]) continue;
        for (size_t j = i+1; j < current.size(); ++j) {
          if (!current[j]) continue;
          if (std::abs (current[i]) == std::abs (current[j]))
            current[j] = 0;
        }
      }

      ssize_t desired_max = 0;
      for (size_t i = 0; i < desired.size(); ++i)
        if (std::abs (desired[i]) > desired_max)
          desired_max = std::abs (desired[i]);

      ssize_t in_max = 0;
      for (size_t i = 0; i < current.size(); ++i)
        if (std::abs (current[i]) > in_max)
          in_max = std::abs (current[i]);
      in_max += desired_max + 1;

      for (size_t i = 0; i < current.size(); ++i) 
        if (dims[i] > 1 && desired[i]) 
          current[i] = desired[i];
        else if (current[i])
          current[i] += current[i] < 0 ? -desired_max : desired_max;
        else 
          current[i] = in_max++;

      symbolise (current);
      return current;
    }


    List __from_command_line (const List& current)
    {
      List strides;
      auto opt = App::get_options ("stride");
      if (!opt.size()) 
        return strides;

      vector<int> tmp = opt[0][0];
      for (auto x : tmp)
        strides.push_back (x); 


      if (strides.size() > current.size())
        WARN ("too many axes supplied to -stride option - ignoring remaining strides");
      strides.resize (current.size(), 0);

      for (const auto x : strides)
        if (std::abs(x) > int (current.size()))
          throw Exception ("strides specified exceed image dimensions: got " + str(opt[0][0]) + ", but image has " + str(current.size()) + " axes");

      for (size_t i = 0; i < strides.size()-1; ++i) {
        if (!strides[1]) continue;
        for (size_t j = i+1; j < strides.size(); ++j) 
          if (std::abs (strides[i]) == std::abs (strides[j])) 
            throw Exception ("duplicate entries provided to \"-stride\" option: " + str(opt[0][0]));
      }

      List prev = get_symbolic (current);
      
      for (size_t i = 0; i < strides.size(); ++i) 
        if (strides[i] != 0) 
          prev[i] = 0;

      prev = get_symbolic (prev);
      ssize_t max_remaining = 0;
      for (const auto x : prev)
        if (std::abs(x) > max_remaining)
          max_remaining = std::abs(x);

      struct FindStride { NOMEMALIGN
        FindStride (List::value_type value) : x (std::abs(value)) { }
        bool operator() (List::value_type a) { return std::abs (a) == x; }
        const List::value_type x;
      };

      ssize_t next_avail = 0;
      for (ssize_t next = 1; next <= max_remaining; ++next) {
        auto p = std::find_if (prev.begin(), prev.end(), FindStride (next));
        assert (p != prev.end());
        List::value_type s;
        while (1) {
          s = *p + ( *p > 0 ? next_avail : -next_avail );
          if (std::find_if (strides.begin(), strides.end(), FindStride (s)) == strides.end())
            break;
          ++next_avail;
        }
        strides[std::distance (prev.begin(), p)] = s;
      }
      
      return strides;
    }



  }
}


