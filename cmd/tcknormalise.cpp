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

#include <fstream>

#include "command.h"
#include "progressbar.h"
#include "get_set.h"
#include "image/buffer_preload.h"
#include "image/voxel.h"
#include "thread/queue.h"
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
  + Argument ("tracks", "the input track file.").type_file ()
  + Argument ("transform", "the image containing the transform.").type_image_in()
  + Argument ("output", "the output fraction image").type_image_out();
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
    Tractography::Reader<value_type> reader;
};



class Warper
{
  public:
    Warper (const Image::BufferPreload<value_type>::voxel_type& warp_voxel) :
      interp (warp_voxel) { }

    bool operator () (const TrackType& in, TrackType& out) {
      out.resize (in.size());
      for (size_t n = 0; n < in.size(); ++n) 
        out[n] = pos(in[n]);
      return true;
    }

    Point<value_type> pos (const Point<value_type>& x) {
      Point<value_type> p;
      if (!interp.scanner (x)) {
        interp[3] = 0; p[0] = interp.value();
        interp[3] = 1; p[1] = interp.value();
        interp[3] = 2; p[2] = interp.value();
      }
      return p;
    }


  protected:
    Image::Interp::Linear<Image::BufferPreload<value_type>::voxel_type> interp;
};



class Writer 
{
  public:
    Writer (const std::string& file, const Tractography::Properties& properties) :
      progress ("normalising tracks..."),
      writer (file, properties) { }

    bool operator() (const TrackType& item) {
      writer (item);
      writer.total_count++;
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

  Image::BufferPreload<value_type> data (argument[1]);
  Image::BufferPreload<value_type>::voxel_type vox (data);
  Warper warper (vox);

  Writer writer (argument[2], loader.properties);

  Thread::run_batched_queue_threaded_pipe (loader, TrackType(), 1024, warper, TrackType(), 1024, writer);
}

