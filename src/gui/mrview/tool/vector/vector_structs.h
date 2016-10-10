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

#ifndef __gui_mrview_tool_vector_structs_h__
#define __gui_mrview_tool_vector_structs_h__

namespace MR
{
  namespace GUI
  {
    namespace MRView
    {
      namespace Tool
      {
        enum FixelColourType { Direction, CValue };
        enum FixelScaleType { Unity, Value };

        struct FixelValue {
          bool loaded = false;
          float value_min = std::numeric_limits<float>::max ();
          float value_max = std::numeric_limits<float>::min ();
          float lessthan = value_min, greaterthan = value_max;
          float current_min = value_min, current_max = value_max;
          std::vector<float> buffer_store;


          void add_value (float value) {
            buffer_store.push_back (value);
            value_min = std::min (value_min, value);
            value_max = std::max (value_max, value);
          }


          void initialise_windowing () {
            lessthan = value_min;
            greaterthan = value_max;
            set_windowing (value_min, value_max);
          }


          void set_windowing (float min, float max) {
            current_min = min;
            current_max = max;
          }


          float get_relative_threshold_lower (FixelValue& fixel_value) const {

            float relative_min = std::numeric_limits<float>::max();

            // Find min value based relative to lower-thresholded fixels
            for(size_t i = 0, N = buffer_store.size(); i < N; ++i) {
              if (buffer_store[i] > lessthan)
                relative_min = std::min(relative_min, fixel_value.buffer_store[i]);
            }

            // Clamp our value to windowing
            return std::max(relative_min, fixel_value.current_min);
          }


          float get_relative_threshold_upper (FixelValue& fixel_value) const {

            float relative_max = std::numeric_limits<float>::min();

            // Find max value based relative to upper-thresholded fixels
            for(size_t i = 0, N = buffer_store.size(); i < N; ++i) {
              if (buffer_store[i] < greaterthan)
                relative_max = std::max(relative_max, fixel_value.buffer_store[i]);
            }

            // Clamp our value to windowing
            return std::min(relative_max, fixel_value.current_max);
          }

        };

      }
    }
  }
}

#endif
