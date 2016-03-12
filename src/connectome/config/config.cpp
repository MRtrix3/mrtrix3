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



#include "connectome/config/config.h"

#include <fstream>



namespace MR {
namespace Connectome {





void load_config (const std::string& path, ConfigInvLookup& config)
{

  assert (config.empty());

  // Import the configuration file
  if (!Path::exists (path))
    throw Exception ("Cannot find input configuration file " + Path::basename (path));
  std::ifstream in (path.c_str(), std::ios_base::in);
  if (!in)
    throw Exception ("Unable to open configuration file " + Path::basename (path));

  // Configuration file should contain node index, followed by structure name
  // Name must be identical to that in relevant lookup table file
  // However, when importing in this function, these are inverted; map structure name to desired node index
  std::string line;
  char name [80];
  const node_t max_node_index = std::numeric_limits<node_t>::max();
  while (std::getline (in, line)) {
    if (line.size() > 1 && line[0] != '#') {
      node_t index = max_node_index;
      sscanf (line.c_str(), "%u %s", &index, name);
      if (index != max_node_index)
        config.insert (std::make_pair (std::string (name), index));
    }
  }

}




void load_config (const std::string& path, std::vector<std::string>& config)
{

  assert (config.empty());

  // Import the configuration file
  if (!Path::exists (path))
    throw Exception ("Cannot find input configuration file " + Path::basename (path));
  std::ifstream in (path.c_str(), std::ios_base::in);
  if (!in)
    throw Exception ("Unable to open configuration file " + Path::basename (path));

  // Configuration file should contain node index, followed by structure name
  // Can only construct a mapping from index to name if there are no duplicate indices
  //   (this may sometimes be the case, e.g. mapping FreeSurfer output to lobes)
  std::string line;
  char name [80];
  const node_t max_node_index = std::numeric_limits<node_t>::max();
  while (std::getline (in, line)) {
    if (line[0] != '#' && line.size() > 1) {
      node_t index = max_node_index;
      sscanf (line.c_str(), "%u %s", &index, name);
      if (index != max_node_index) {
        if (index >= config.size())
          config.resize (index + 1);
        if (!(config[index].empty()))
          throw Exception ("Duplicate indices found in connectome config file " + Path::basename(path) + "; cannot create index->name lookup");
        config[index] = std::string (name);
      }
    }
  }

}





}
}



