/*
    Copyright 2012 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2012.

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



#ifndef __dwi_tractography_connectomics_edge_metrics_h__
#define __dwi_tractography_connectomics_edge_metrics_h__


#include <vector>

#include "point.h"

#include "image/buffer.h"
#include "image/loop.h"
#include "image/voxel.h"

#include "dwi/tractography/connectomics/connectomics.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectomics {



// Provide a common interface for performing these calculations
class Metric_base {

  public:
    Metric_base (const bool scale) : scale_by_count (scale) { }
    virtual ~Metric_base() { }

    virtual double operator() (const std::vector< Point<float> >& tck, const std::pair<node_t, node_t>& nodes) const
    {
      throw Exception ("Calling empty virtual function Metric_base::operator()");
    }

    const bool scale_edges_by_streamline_count() const { return scale_by_count; }

  private:
    const bool scale_by_count;

};



class Metric_count : public Metric_base {

  public:
    Metric_count () : Metric_base (false) { }

    double operator() (const std::vector< Point<float> >& tck, const std::pair<node_t, node_t>& nodes) const
    {
      return 1.0;
    }

};


class Metric_meanlength : public Metric_base {

  public:
    Metric_meanlength () : Metric_base (true) { }

    double operator() (const std::vector< Point<float> >& tck, const std::pair<node_t, node_t>& nodes) const
    {
      return (tck.size() - 1);
    }

};


class Metric_invlength : public Metric_base {

  public:
    Metric_invlength () : Metric_base (false) { }

    double operator() (const std::vector< Point<float> >& tck, const std::pair<node_t, node_t>& nodes) const
    {
      return (tck.size() > 1 ? (1.0 / (tck.size() - 1)) : 0);
    }

};



class Metric_invnodevolume : public Metric_base {

  public:
    Metric_invnodevolume (Image::Buffer<node_t>& in_data) :
      Metric_base (false)
    {
      Image::Buffer<node_t>::voxel_type in (in_data);
      Image::Loop loop;
      for (loop.start (in); loop.ok(); loop.next (in)) {
        const node_t node_index = in.value();
        if (node_index >= node_volumes.size())
          node_volumes.resize (node_index + 1, 0);
        node_volumes[node_index]++;
      }
    }

    virtual double operator() (const std::vector< Point<float> >& tck, const std::pair<node_t, node_t>& nodes) const
    {
      return (2.0 / (node_volumes[nodes.first] + node_volumes[nodes.second]));
    }

  private:
    std::vector<size_t> node_volumes;

};



class Metric_invlength_invnodevolume : public Metric_invnodevolume {

  public:
    Metric_invlength_invnodevolume (Image::Buffer<node_t>& in_data) :
      Metric_invnodevolume (in_data) { }

    double operator() (const std::vector< Point<float> >& tck, const std::pair<node_t, node_t>& nodes) const
    {
      return (tck.size() > 1 ? (Metric_invnodevolume::operator() (tck, nodes) / (tck.size() - 1)) : 0);
    }

};



}
}
}
}


#endif

