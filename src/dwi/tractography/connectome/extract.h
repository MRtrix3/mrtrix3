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



#ifndef __dwi_tractography_connectome_extract_h__
#define __dwi_tractography_connectome_extract_h__


#include "file/ofstream.h"

#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"
#include "dwi/tractography/connectome/connectome.h"
#include "dwi/tractography/connectome/exemplar.h"


namespace MR {
namespace DWI {
namespace Tractography {
namespace Connectome {






class Selector
{
  public:
    Selector (const node_t node) :
      list (1, node),
      exact_match (false) { }
    Selector (const node_t node_one, const node_t node_two) :
      exact_match (true) { list.push_back (node_one); list.push_back (node_two); }
    Selector (const std::vector<node_t>& node_list, const bool both) :
      list (node_list),
      exact_match (both) { }
    Selector (const Selector& that) :
      list (that.list), exact_match (that.exact_match) { }
    Selector (Selector&& that) :
      list (std::move (that.list)), exact_match (that.exact_match) { }

    bool operator() (const node_t) const;
    bool operator() (const NodePair&) const;
    bool operator() (const node_t one, const node_t two) const { return (*this) (NodePair (one, two)); }
    bool operator() (const std::vector<node_t>&) const;

  private:
    std::vector<node_t> list;
    bool exact_match;
};






class WriterExemplars
{
  public:
    WriterExemplars (const Tractography::Properties&, const std::vector<node_t>&, const bool, const node_t, const std::vector<Eigen::Vector3f>&);

    bool operator() (const Tractography::Connectome::Streamline_nodepair&);
    bool operator() (const Tractography::Connectome::Streamline_nodelist&);

    void finalize();

    void write (const node_t, const node_t, const std::string&, const std::string&);
    void write (const node_t, const std::string&, const std::string&);
    void write (const std::string&, const std::string&);


  private:
    float step_size;
    std::vector<Selector> selectors;
    std::vector<Exemplar> exemplars;
};









class WriterExtraction
{

  public:
    WriterExtraction (const Tractography::Properties&, const std::vector<node_t>&, const bool);
    ~WriterExtraction();

    void add (const node_t, const std::string&, const std::string);
    void add (const node_t, const node_t, const std::string&, const std::string);
    void add (const std::vector<node_t>&, const std::string&, const std::string);

    void clear();

    bool operator() (const Connectome::Streamline_nodepair&) const;
    bool operator() (const Connectome::Streamline_nodelist&) const;

    size_t file_count() const { return writers.size(); }


  private:
    const Tractography::Properties& properties;
    const std::vector<node_t>& node_list;
    const bool exclusive;
    std::vector< Selector > selectors;
    std::vector< Tractography::WriterUnbuffered<float>* > writers;
    Tractography::Streamline<> empty_tck;

};






}
}
}
}


#endif

