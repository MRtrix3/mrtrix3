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



#ifndef __dwi_tractography_connectome_tck2nodes_h__
#define __dwi_tractography_connectome_tck2nodes_h__


#include <vector>

#include "image.h"
#include "interp/linear.h"
#include "interp/nearest.h"

#include "dwi/tractography/connectome/connectome.h"

#include "dwi/tractography/streamline.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectome {




// Provides a common interface for assigning a streamline to the relevant parcellation node pair
// Note that this class is NOT copy-constructed, so derivative classes must be thread-safe
class Tck2nodes_base { MEMALIGN(Tck2nodes_base)

  public:
    Tck2nodes_base (const Image<node_t>& nodes_data, const bool pair) :
        nodes     (nodes_data),
        transform (new Transform (nodes)),
        pair      (pair) { }

    Tck2nodes_base (const Tck2nodes_base& that) = default;

    virtual ~Tck2nodes_base() { }

    NodePair operator() (const Tractography::Streamline<>& tck) const
    {
      assert (pair);
      Image<node_t> v (nodes);
      const node_t node_one = select_node (tck, v, false);
      const node_t node_two = select_node (tck, v, true);
      return std::make_pair (node_one, node_two);
    }

    bool operator() (const Streamline<>& tck, vector<node_t>& out) const
    {
      assert (!pair);
      Image<node_t> v (nodes);
      select_nodes (tck, v, out);
      return true;
    }

    bool provides_pair() const { return pair; }


  protected:
    const Image<node_t> nodes;
    std::shared_ptr<Transform> transform;
    const bool pair;

    virtual node_t select_node (const Tractography::Streamline<>& tck, Image<node_t>& v, const bool end) const {
      throw Exception ("Calling empty virtual function Tck2nodes_base::select_node()");
    }

    virtual void select_nodes (const Streamline<>& tck, Image<node_t>& v, vector<node_t>& out) const {
      throw Exception ("Calling empty virtual function Tck2nodes_base::select_nodes()");
    }

    class voxel_type : public Eigen::Array<int,3,1>
    { MEMALIGN(voxel_type)
      public:
        using Eigen::Array<int,3,1>::Array;
        bool operator< (const Eigen::Array<int,3,1>& that) const {
          return ((*this)[2] == that[2]) ? (((*this)[1] == that[1]) ? ((*this)[0] < that[0]) : ((*this)[1] < that[1])) : ((*this)[2] < that[2]);
        }
    };

};




// Specific implementations of assignment methodologies

// Most basic: look up the voxel value at the voxel containing the streamline endpoint
class Tck2nodes_end_voxels : public Tck2nodes_base { MEMALIGN(Tck2nodes_end_voxels)

  public:
    Tck2nodes_end_voxels (const Image<node_t>& nodes_data) :
        Tck2nodes_base (nodes_data, true) { }

    Tck2nodes_end_voxels (const Tck2nodes_end_voxels& that) :
        Tck2nodes_base (that) { }

    ~Tck2nodes_end_voxels() { }

  private:
    node_t select_node (const Tractography::Streamline<float>&, Image<node_t>&, const bool) const override;

};





// Radial search
class Tck2nodes_radial : public Tck2nodes_base { MEMALIGN(Tck2nodes_radial)

  public:
    Tck2nodes_radial (const Image<node_t>& nodes_data, const default_type radius) :
        Tck2nodes_base (nodes_data, true),
        max_dist       (radius),
        max_add_dist   (std::sqrt (Math::pow2 (0.5 * nodes.spacing(2)) + Math::pow2 (0.5 * nodes.spacing(1)) + Math::pow2 (0.5 * nodes.spacing(0))))
    {
      initialise_search ();
    }

    Tck2nodes_radial (const Tck2nodes_radial& that) :
        Tck2nodes_base (that),
        radial_search  (that.radial_search),
        max_dist       (that.max_dist),
        max_add_dist   (that.max_add_dist) { }

    ~Tck2nodes_radial() { }

  private:
    node_t select_node (const Tractography::Streamline<>&, Image<node_t>&, const bool) const override;

    void initialise_search ();
    vector<voxel_type> radial_search;
    const default_type max_dist;
    // Distances are sub-voxel from the precise streamline termination point, so the search order is imperfect.
    //   This parameter controls when to stop the radial search because no voxel within the search space can be closer
    //   than the closest voxel with non-zero node index processed thus far.
    const default_type max_add_dist;

    friend class Tck2nodes_visitation;

};



// Do a reverse-search from the track endpoints inwards
class Tck2nodes_revsearch : public Tck2nodes_base 
{ MEMALIGN (Tck2nodes_revsearch)

  public:
    Tck2nodes_revsearch (const Image<node_t>& nodes_data, const default_type length) :
        Tck2nodes_base (nodes_data, true),
        max_dist       (length) { }

    Tck2nodes_revsearch (const Tck2nodes_revsearch& that) :
        Tck2nodes_base (that),
        max_dist       (that.max_dist) { }

    ~Tck2nodes_revsearch() { }

  private:
    node_t select_node (const Tractography::Streamline<>&, Image<node_t>&, const bool) const override;

    const default_type max_dist;

};



// Forward search - form a diamond-like shape emanating from the streamline endpoint in the direction of the tangent
class Tck2nodes_forwardsearch : public Tck2nodes_base
{ MEMALIGN(Tck2nodes_forwardsearch)

  public:
    Tck2nodes_forwardsearch (const Image<node_t>& nodes_data, const default_type length) :
        Tck2nodes_base (nodes_data, true),
        max_dist       (length),
        angle_limit    (Math::pi_4) { } // 45 degree limit

    Tck2nodes_forwardsearch (const Tck2nodes_forwardsearch& that) :
        Tck2nodes_base (that),
        max_dist       (that.max_dist),
        angle_limit    (that.angle_limit) { }

    ~Tck2nodes_forwardsearch() { }

  private:
    node_t select_node (const Tractography::Streamline<>&, Image<node_t>&, const bool) const override;

    const default_type max_dist;
    const default_type angle_limit;

    default_type get_cf (const Eigen::Vector3&, const Eigen::Vector3&, const voxel_type&) const;

};





// Class that obtains a list of all nodes overlapped by the streamline
class Tck2nodes_all_voxels : public Tck2nodes_base
{ MEMALIGN(Tck2nodes_all_voxels)
  public:
    Tck2nodes_all_voxels (const Image<node_t>& nodes_data) :
        Tck2nodes_base (nodes_data, false) { }

    Tck2nodes_all_voxels (const Tck2nodes_all_voxels& that) :
        Tck2nodes_base (that) { }

    ~Tck2nodes_all_voxels() { }

  private:
    void select_nodes (const Streamline<>&, Image<node_t>&, vector<node_t>&) const override;

};






}
}
}
}


#endif

