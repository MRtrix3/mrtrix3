/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 27/06/08.

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

#include "command.h"
#include "progressbar.h"
#include "image.h"
#include "interp/linear.h"
#include "thread_queue.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"

using namespace MR;
using namespace MR::DWI;
using namespace App;


void usage ()
{
  DESCRIPTION
  + "apply a normalisation map to a tracks file.";

  ARGUMENTS
  + Argument ("tracks", "the input track file.").type_file_in()
  + Argument ("transform", "the image containing the transform.").type_image_in()
  + Argument ("output", "the output track file").type_file_out();
}



typedef float value_type;
typedef Tractography::Streamline<value_type> TrackType;



class Loader 
{
  public:
    Loader (const std::string& file) : reader (file, properties) {}

    bool operator() (TrackType& item) {
      return reader (item);
    }

    Tractography::Properties properties;
  protected:
    Tractography::Reader reader;
};



class Warper
{
  public:
    Warper (const Image<value_type>& warp) :
      interp (warp) { }

    bool operator () (const TrackType& in, TrackType& out) {
      out.resize (in.size());
      for (size_t n = 0; n < in.size(); ++n) 
        out[n] = pos(in[n]);
      return true;
    }

    Eigen::Matrix<value_type,3,1> pos (const Eigen::Matrix<value_type,3,1>& x) {
      Eigen::Matrix<value_type,3,1> p;
      if (!interp.scanner (x)) {
        interp.index(3) = 0; p[0] = interp.value();
        interp.index(3) = 1; p[1] = interp.value();
        interp.index(3) = 2; p[2] = interp.value();
      }
      return p;
    }


  protected:
    Interp::Linear< Image<value_type> > interp;
};



class Writer 
{
  public:
    Writer (const std::string& file, const Tractography::Properties& properties) :
      progress ("normalising tracks..."),
      writer (file, properties) { }

    bool operator() (const TrackType& item) {
      writer (item);
      ++progress;
      return true;
    }

  protected:
    ProgressBar progress;
    Tractography::Properties properties;
    Tractography::Writer<value_type> writer;
};





void run ()
{
  Loader loader (argument[0]);

  auto data = Image<value_type>::open (argument[1]).with_direct_io (Stride::contiguous_along_axis(3));
  Warper warper (data);

  Writer writer (argument[2], loader.properties);

  Thread::run_queue (
      loader, 
      Thread::batch (TrackType(), 1024), 
      Thread::multi (warper), 
      Thread::batch (TrackType(), 1024), 
      writer);
}

