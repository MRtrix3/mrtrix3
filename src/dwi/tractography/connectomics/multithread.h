/*
    Copyright 2013 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2013.

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



#ifndef __dwi_tractography_connectomics_multithread_h__
#define __dwi_tractography_connectomics_multithread_h__



#include "math/matrix.h"

#include "dwi/tractography/mapping/mapping.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/connectomics/connectomics.h"
#include "dwi/tractography/connectomics/edge_metrics.h"
#include "dwi/tractography/connectomics/tck2nodes.h"

#include <set>



namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectomics {




class Mapped_track
{

  public:
    Mapped_track() :
      nodes (std::make_pair (0, 0)),
      factor (0.0),
      weight (1.0) { }

    void set_first_node  (const node_t i) { nodes.first = i;  }
    void set_second_node (const node_t i) { nodes.second = i; }
    void set_nodes       (const std::pair<node_t, node_t>& i) { nodes = i; }
    void set_factor      (const float i)    { factor = i; }
    void set_weight      (const float i)    { weight = i; }

    node_t get_first_node()  const { return nodes.first;  }
    node_t get_second_node() const { return nodes.second; }
    const std::pair<node_t, node_t>& get_nodes() const { return nodes; }
    float get_factor() const { return factor; }
    float get_weight() const { return weight; }

  private:
    std::pair<node_t, node_t> nodes;
    float factor, weight;

};





class Mapper
{

  public:
    Mapper (Tck2nodes_base& a, const Metric_base& b) :
      tck2nodes (a),
      metric (b) { }

    Mapper (Mapper& that) :
      tck2nodes (that.tck2nodes),
      metric (that.metric) { }


    bool operator() (const Tractography::TrackData<float>& in, Mapped_track& out)
    {
      out.set_nodes (tck2nodes (in));
      out.set_factor (metric (in, out.get_nodes()));
      out.set_weight (in.weight);
      return true;
    }


  private:
    Tck2nodes_base& tck2nodes;
    const Metric_base& metric;

};








class Connectome
{

  public:
    Connectome (const node_t max_node_index) :
      data   (max_node_index + 1, max_node_index + 1),
      counts (max_node_index + 1, max_node_index + 1)
    {
      data = 0.0;
      counts = 0.0;
    }


    bool operator() (const Mapped_track& in) {
      assert (in.get_first_node()  < data.rows());
      assert (in.get_second_node() < data.rows());
      assert (in.get_first_node() <= in.get_second_node());
      data   (in.get_first_node(), in.get_second_node()) += in.get_factor() * in.get_weight();
      counts (in.get_first_node(), in.get_second_node()) += in.get_weight();
      return true;
    }


    void scale_by_streamline_count() {
      for (node_t i = 0; i != counts.rows(); ++i) {
        for (node_t j = i; j != counts.columns(); ++j) {
          if (counts (i, j)) {
            data (i, j) /= counts (i, j);
            counts (i, j) = 1;
          }
        }
      }
    }


    void error_check (const std::set<node_t>& missing_nodes) {
      std::vector<uint32_t> node_counts (num_nodes(), 0);
      for (node_t i = 0; i != counts.rows() - 1; ++i) {
        for (node_t j = i; j != counts.columns() - 1; ++j) {
          node_counts[i] += counts (i, j);
          node_counts[j] += counts (i, j);
        }
      }
      std::vector<node_t> empty_nodes;
      for (size_t i = 0; i != node_counts.size(); ++i) {
        if (!node_counts[i] && missing_nodes.find (i) == missing_nodes.end())
          empty_nodes.push_back (i);
      }
      if (empty_nodes.size()) {
        WARN ("The following nodes do not have any streamlines assigned:");
        std::string list = str(empty_nodes.front());
        for (size_t i = 1; i != empty_nodes.size(); ++i)
          list += ", " + str(empty_nodes[i]);
        WARN (list);
        WARN ("(This may indicate a poor registration)");
      }
    }


    void remove_unassigned() {
      for (node_t i = 0; i != data.rows() - 1; ++i) {
        for (node_t j = i; j != data.columns() - 1; ++j) {
          data   (i, j) = data   (i+1, j+1);
          counts (i, j) = counts (i+1, j+1);
        }
      }
      data  .resize (data  .rows() - 1, data  .columns() - 1);
      counts.resize (counts.rows() - 1, counts.columns() - 1);
    }


    void zero_diagonal() {
      for (node_t i = 0; i != data.rows(); ++i)
        data (i, i) = counts (i, i) = 0.0;
    }


    void write (const std::string& path) { data.save (path); }


    node_t num_nodes() const { return (data.rows() - 1); }


  private:
    Math::Matrix<double> data, counts;

};









class MappedTrackWithData : public Mapped_track
{
  public:
    MappedTrackWithData () :
      Mapped_track () { }
    std::vector< Point<float> > tck;
};



class NodeExtractMapper
{

  public:
    NodeExtractMapper (Tck2nodes_base& a) :
      tck2nodes (a) { }

    NodeExtractMapper (const NodeExtractMapper& that) :
      tck2nodes (that.tck2nodes) { }

    bool operator() (const Tractography::TrackData<float>& in, MappedTrackWithData& out) const
    {
      out.set_nodes (tck2nodes (in));
      out.set_factor (0.0);
      out.set_weight (in.weight);
      out.tck = in;
      return true;
    }

  private:
    Tck2nodes_base& tck2nodes;

};



class NodeExtractWriter
{

  public:

    class NodeSelector
    {
      public:
        NodeSelector (const node_t node) :
          one (node),
          two (0),
          is_pair (false) { }
        NodeSelector (const node_t node_one, const node_t node_two) :
          one (std::min (node_one, node_two)),
          two (std::max (node_one, node_two)),
          is_pair (true) { }
        NodeSelector (const NodeSelector& that) :
          one (that.one), two (that.two), is_pair (that.is_pair) { }

        bool operator() (const Mapped_track& nodes) const
        {
          if (is_pair)
            return ((one == nodes.get_first_node()) && (two == nodes.get_second_node()));
          else
            return ((one == nodes.get_first_node()) || (one == nodes.get_second_node()));
        }

      private:
        node_t one, two;
        bool is_pair;
    };

    NodeExtractWriter (const Tractography::Properties& p) :
      properties (p) { }

    ~NodeExtractWriter()
    {
      for (size_t i = 0; i != writers.size(); ++i) {
        delete writers[i];
        writers[i] = NULL;
      }
    }


    void add (const node_t node, const std::string& path, const std::string weights_path = "")
    {
      nodes.push_back (NodeSelector (node));
      writers.push_back (new Tractography::WriterUnbuffered<float> (path, properties));
      if (weights_path.size())
        writers.back()->set_weights_path (weights_path);
    }

    void add (const node_t node_one, const node_t node_two, const std::string& path, const std::string weights_path = "")
    {
      nodes.push_back (NodeSelector (node_one, node_two));
      writers.push_back (new Tractography::WriterUnbuffered<float> (path, properties));
      if (weights_path.size())
        writers.back()->set_weights_path (weights_path);
    }

    void clear()
    {
      nodes.clear();
      for (size_t i = 0; i != writers.size(); ++i) {
        delete writers[i];
        writers[i] = NULL;
      }
      writers.clear();
    }


    bool operator() (const MappedTrackWithData& in) const
    {
      for (size_t i = 0; i != file_count(); ++i) {
        if (nodes[i] (in)) {
          Tractography::TrackData<float> temp (in.tck);
          temp.weight = in.get_weight();
          writers[i]->append (temp);
        } else {
          writers[i]->append (empty_tck);
        }
      }
      return true;
    }


    size_t file_count() const { return writers.size(); }


  private:
    Tractography::Properties properties;
    std::vector< NodeSelector > nodes;
    std::vector< Tractography::WriterUnbuffered<float>* > writers;
    std::vector< Point<float> > empty_tck;

};






}
}
}
}


#endif

