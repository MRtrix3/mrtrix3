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
#include "dwi/tractography/streamline.h"
#include "dwi/tractography/connectomics/connectomics.h"
#include "dwi/tractography/connectomics/edge_metrics.h"
#include "dwi/tractography/connectomics/tck2nodes.h"

#include <set>



namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectomics {




class Mapped_track_nodepair
{

  public:
    Mapped_track_nodepair() :
      nodes (std::make_pair (0, 0)),
      factor (0.0),
      weight (1.0) { }

    void set_first_node  (const node_t i) { nodes.first = i;  }
    void set_second_node (const node_t i) { nodes.second = i; }
    void set_nodes       (const NodePair& i) { nodes = i; }
    void set_factor      (const float i)    { factor = i; }
    void set_weight      (const float i)    { weight = i; }

    node_t get_first_node()  const { return nodes.first;  }
    node_t get_second_node() const { return nodes.second; }
    const NodePair& get_nodes() const { return nodes; }
    float get_factor() const { return factor; }
    float get_weight() const { return weight; }

  private:
    NodePair nodes;
    float factor, weight;

};




class Mapped_track_nodelist
{

  public:
    Mapped_track_nodelist() :
      nodes (),
      factor (0.0),
      weight (1.0) { }

    void add_node   (const node_t i) { nodes.push_back (i);  }
    void set_nodes  (const std::vector<node_t>& i) { nodes = i; }
    void set_nodes  (std::vector<node_t>&& i) { std::swap (nodes, i); }
    void set_factor (const float i)    { factor = i; }
    void set_weight (const float i)    { weight = i; }

    const std::vector<node_t>& get_nodes() const { return nodes; }
    float get_factor() const { return factor; }
    float get_weight() const { return weight; }

  private:
    std::vector<node_t> nodes;
    float factor, weight;

};





class Mapper
{

  public:
    Mapper (Tck2nodes_base& a, const Metric_base& b) :
      tck2nodes (a),
      metric (b) { }

    Mapper (const Mapper& that) :
      tck2nodes (that.tck2nodes),
      metric (that.metric) { }


    bool operator() (const Tractography::Streamline<float>& in, Mapped_track_nodepair& out)
    {
      NodePair nodes;
      tck2nodes      (in, nodes);
      out.set_nodes  (nodes);
      out.set_factor (metric (in, out.get_nodes()));
      out.set_weight (in.weight);
      return true;
    }

    bool operator() (const Tractography::Streamline<float>& in, Mapped_track_nodelist& out)
    {
      std::vector<node_t> nodes;
      tck2nodes      (in, nodes);
      out.set_nodes  (nodes);
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
    Connectome (const node_t max_node_index, const bool vector_output = false) :
      data   (vector_output ? 1 : max_node_index + 1, max_node_index + 1),
      counts (vector_output ? 1 : max_node_index + 1, max_node_index + 1)
    {
      data = 0.0;
      counts = 0.0;
    }





    bool operator() (const Mapped_track_nodepair& in) {
      assert (in.get_first_node()  < data.columns());
      assert (in.get_second_node() < data.columns());
      if (is_vector()) {
        data   (0, in.get_first_node())  += in.get_factor() * in.get_weight();
        counts (0, in.get_first_node())  += in.get_weight();
        data   (0, in.get_second_node()) += in.get_factor() * in.get_weight();
        counts (0, in.get_second_node()) += in.get_weight();
      } else {
        assert (in.get_first_node() <= in.get_second_node());
        data   (in.get_first_node(), in.get_second_node()) += in.get_factor() * in.get_weight();
        counts (in.get_first_node(), in.get_second_node()) += in.get_weight();
      }
      return true;
    }



    bool operator() (const Mapped_track_nodelist& in) {
      const std::vector<node_t>& list (in.get_nodes());
      if (is_vector()) {
        if (list.empty()) {
          data   (0, 0) += in.get_factor() * in.get_weight();
          counts (0, 0) += in.get_weight();
        } else {
          for (std::vector<node_t>::const_iterator n = list.begin(); n != list.end(); ++n) {
            data   (0, *n) += in.get_factor() * in.get_weight();
            counts (0, *n) += in.get_weight();
          }
        }
      } else {
        if (list.empty()) {
          data   (0, 0) += in.get_factor() * in.get_weight();
          counts (0, 0) += in.get_weight();
        } else if (list.size() == 1) {
          data   (0, list.front()) += in.get_factor() * in.get_weight();
          counts (0, list.front()) += in.get_weight();
        } else {
          for (size_t i = 0; i != list.size(); ++i) {
            for (size_t j = i; j != list.size(); ++j) {
              data   (list[i], list[j]) += in.get_factor() * in.get_weight();
              counts (list[i], list[j]) += in.get_weight();
            }
          }
        }
      }
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
      std::vector<uint32_t> node_counts (data.columns(), 0);
      for (node_t i = 0; i != counts.rows(); ++i) {
        for (node_t j = i; j != counts.columns(); ++j) {
          node_counts[i] += counts (i, j);
          node_counts[j] += counts (i, j);
        }
      }
      std::vector<node_t> empty_nodes;
      for (size_t i = 1; i != node_counts.size(); ++i) {
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
      if (is_vector()) {
        for (node_t i = 0; i != data.columns() - 1; ++i) {
          data   (0, i) = data   (0, i+1);
          counts (0, i) = counts (0, i+1);
        }
        data  .resize (1, data  .columns() - 1);
        counts.resize (1, counts.columns() - 1);
      } else {
        for (node_t i = 0; i != data.rows() - 1; ++i) {
          for (node_t j = i; j != data.columns() - 1; ++j) {
            data   (i, j) = data   (i+1, j+1);
            counts (i, j) = counts (i+1, j+1);
          }
        }
        data  .resize (data  .rows() - 1, data  .columns() - 1);
        counts.resize (counts.rows() - 1, counts.columns() - 1);
      }
    }


    void zero_diagonal() {
      if (is_vector()) return;
      for (node_t i = 0; i != data.rows(); ++i)
        data (i, i) = counts (i, i) = 0.0;
    }


    void write (const std::string& path) { data.save (path); }


    //node_t num_nodes() const { return (data.columns() - 1); }
    bool is_vector() const { return (data.rows() == 1); }


  private:
    Math::Matrix<double> data, counts;

};









class MappedTrackWithData_nodepair : public Mapped_track_nodepair
{
  public:
    MappedTrackWithData_nodepair () :
      Mapped_track_nodepair () { }
    std::vector< Point<float> > tck;
};

class MappedTrackWithData_nodelist : public Mapped_track_nodelist
{
  public:
    MappedTrackWithData_nodelist () :
      Mapped_track_nodelist () { }
    std::vector< Point<float> > tck;
};



class NodeExtractMapper
{

  public:
    NodeExtractMapper (Tck2nodes_base& a) :
      tck2nodes (a) { }

    NodeExtractMapper (const NodeExtractMapper& that) :
      tck2nodes (that.tck2nodes) { }

    bool operator() (const Tractography::Streamline<float>& in, MappedTrackWithData_nodepair& out) const
    {
      NodePair nodes;
      tck2nodes (in, nodes);
      out.set_nodes (nodes);
      out.set_factor (0.0);
      out.set_weight (in.weight);
      out.tck = in;
      return true;
    }

    bool operator() (const Tractography::Streamline<float>& in, MappedTrackWithData_nodelist& out) const
    {
      std::vector<node_t> nodes;
      tck2nodes (in, nodes);
      out.set_nodes (nodes);
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

        bool operator() (const Mapped_track_nodepair& nodes) const
        {
          if (is_pair)
            return ((one == nodes.get_first_node()) && (two == nodes.get_second_node()));
          else
            return ((one == nodes.get_first_node()) || (one == nodes.get_second_node()));
        }

        bool operator() (const Mapped_track_nodelist& nodes) const
        {
          bool one_present = false, two_present = false;
          for (std::vector<node_t>::const_iterator n = nodes.get_nodes().begin(); n != nodes.get_nodes().end(); ++n) {
            if (*n == one) one_present = true;
            if (*n == two) two_present = true;
          }
          if (is_pair)
            return (one_present && two_present);
          else
            return (one_present);
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


    bool operator() (const MappedTrackWithData_nodepair& in) const
    {
      for (size_t i = 0; i != file_count(); ++i) {
        if (nodes[i] (in)) {
          Tractography::Streamline<float> temp (in.tck);
          temp.weight = in.get_weight();
          (*writers[i]) (temp);
        } else {
          (*writers[i]) (empty_tck);
        }
      }
      return true;
    }

    bool operator() (const MappedTrackWithData_nodelist& in) const
    {
      for (size_t i = 0; i != file_count(); ++i) {
        if (nodes[i] (in)) {
          Tractography::Streamline<float> temp (in.tck);
          temp.weight = in.get_weight();
          (*writers[i]) (temp);
        } else {
          (*writers[i]) (empty_tck);
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

