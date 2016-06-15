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



#ifndef __dwi_tractography_connectome_metric_h__
#define __dwi_tractography_connectome_metric_h__


#include <vector>

#include "image.h"
#include "algo/loop.h"
#include "interp/linear.h"

#include "connectome/connectome.h"

#include "dwi/tractography/streamline.h"
#include "dwi/tractography/connectome/connectome.h"




namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectome {



// Provide a common interface for calculating the contribution from a
//   particular streamline to a particular edge of the connectome
class Metric {

  public:
    Metric() :
        scale_by_length (false),
        scale_by_invlength (false),
        scale_by_invnodevol (false),
        scale_by_file (false) { }

    double operator() (const Streamline<>& tck, const NodePair& nodes) const
    {
      if (scale_by_invnodevol) {
        assert (nodes.first < node_volumes.size());
        assert (nodes.second < node_volumes.size());
        const double sum_volumes = (node_volumes[nodes.first] + node_volumes[nodes.second]);
        if (!sum_volumes) return 0.0;
        return (*this)(tck) * 2.0 / sum_volumes;
      }
      return (*this)(tck);
    }

    double operator() (const Streamline<>& tck, const std::vector<node_t>& nodes) const
    {
      if (scale_by_invnodevol) {
        double sum_volumes = 0.0;
        for (std::vector<node_t>::const_iterator n = nodes.begin(); n != nodes.end(); ++n) {
          assert (*n < node_volumes.size());
          sum_volumes += node_volumes[*n];
        }
        if (!sum_volumes) return 0.0;
        return (*this)(tck) * nodes.size() / sum_volumes;
      }
      return (*this)(tck);
    }

    double operator() (const Streamline<>& tck) const
    {
      double result = 1.0;
      if (scale_by_length)
        result *= tck.calc_length();
      else if (scale_by_invlength)
        result = (tck.size() > 1 ? (result / tck.calc_length()) : 0.0);
      if (scale_by_file) {
        if (tck.index >= size_t(file_values.size()))
          throw Exception ("File " + file_path + " does not contain enough entries for this tractogram");
        result *= file_values[tck.index];
      }
      return result;
    }


    void set_scale_length (const bool i = true) {
      if (i) assert (!scale_by_invlength);
      scale_by_length = i;
    }
    void set_scale_invlength (const bool i = true) {
      if (i) assert (!scale_by_length);
      scale_by_invlength = i;
    }
    void set_scale_invnodevol (Image<node_t>& nodes, const bool i = true) {
      scale_by_invnodevol = i;
      if (!i) {
        node_volumes.resize (0);
        return;
      }
      for (auto l = Loop() (nodes); l; ++l) {
        const node_t index = nodes.value();
        if (index >= node_volumes.size())
          node_volumes.conservativeResizeLike (Eigen::VectorXd::Zero (index + 1));
        node_volumes[index]++;
      }
    }
    void set_scale_file (const std::string& path, const bool i = true) {
      scale_by_file = i;
      if (!i) {
        file_path.clear();
        file_values.resize (0);
        return;
      }
      file_path = Path::basename (path);
      file_values = MR::load_vector (path);
    }


  private:
    bool scale_by_length, scale_by_invlength, scale_by_invnodevol, scale_by_file;
    Eigen::VectorXd node_volumes;
    std::string file_path;
    Eigen::VectorXd file_values;

};






}
}
}
}


#endif

