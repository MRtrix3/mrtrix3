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
#include <set>

#include "app.h"
#include "progressbar.h"
#include "get_set.h"
#include "image/voxel.h"
#include "dataset/misc.h"
#include "dataset/buffer.h"
#include "dataset/interp/linear.h"
#include "thread/exec.h"
#include "thread/queue.h"
#include "math/hermite.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"

using namespace MR; 
using namespace MR::DWI; 
using namespace std; 

SET_VERSION_DEFAULT;

DESCRIPTION = {
  "convert a tracks file into a map of the fraction of tracks to enter each voxel.",
  NULL
};

ARGUMENTS = {
  Argument ("tracks", "track file", "the input track file.").type_file (),
  Argument ("template", "template image", "an image file to be used as a template for the output (the output image wil have the same voxel size and dimensions).").type_image_in(),
  Argument ("output", "output image", "the output fraction image").type_image_out(),
  Argument::End
};


OPTIONS = {
  Option ("fraction", "output fibre fraction", "produce an image of the fraction of fibres through each voxel (as a proportion of the total number in the file), rather than the count."),

  Option ("datatype", "data type", "specify output image data type.")
    .append (Argument ("spec", "specifier", "the data type specifier.").type_choice (DataType::identifiers)),

  Option ("resample", "resample tracks", "resample the tracks at regular intervals using Hermite interpolation.")
    .append (Argument ("factor", "factor", "the factor by which to resample.").type_integer (1, INT_MAX, 1)),

  Option::End
};





class Voxel 
{
  public:
    Voxel () { }
    Voxel (const Point& p) : 
      x (Math::round<size_t> (p[0])), 
      y (Math::round<size_t> (p[1])), 
      z (Math::round<size_t> (p[2])) { }

    bool operator< (const Voxel& v) const 
    {
      if (x < v.x) return (true);
      if (x > v.x) return (false);
      if (y < v.y) return (true);
      if (y > v.y) return (false);
      if (z < v.z) return (true);
      return (false);
    }

    size_t x, y, z;
};



typedef Thread::Queue<std::vector<Point> > Queue1;
typedef Thread::Queue<std::set<Voxel> > Queue2;






class TrackLoader 
{
  public:
    TrackLoader (Queue1& queue, DWI::Tractography::Reader& file, size_t count) :
      writer (queue),
      reader (file),
      total_count (count) { }

    void execute () 
    {
      Queue1::Writer::Item item (writer);

      ProgressBar progress ("mapping tracks to image...", total_count);
      while (reader.next (*item)) {
        if (!item.write())
          throw Exception ("error writing to track-mapping queue!");
        ++progress;
      }
    }

  private:
    Queue1::Writer writer;
    DWI::Tractography::Reader& reader;
    size_t total_count;
};






class TrackMapper 
{
  public:
    TrackMapper (Queue1& input, Queue2& output, const Image::Header& header, const Math::Matrix<float>* resample_matrix = NULL) :
      reader (input),
      writer (output),
      H (header),
      resampler (resample_matrix) {
      }

    void execute () 
    {
      Queue1::Reader::Item in (reader);
      Queue2::Writer::Item out (writer);
      DataSet::Interp::Linear<const Image::Header> interp (H);
      Math::Matrix<float> resampled, orig (4,3);
      if (resampler) 
        resampled.allocate (resampler->rows(), 3);

      while (in.read()) {
        out->clear();
        for (std::vector<Point>::const_iterator i = in->begin(); i != in->end(); ++i) {
          out->insert (Voxel (interp.scanner2voxel (*i)));
          if (resampler) { // TODO: this is not complete!!!
            Math::mult (resampled, *resampler, orig);
          }
        }

        if (!out.write()) 
          throw Exception ("error writing to write-back queue!");
      }
    }

  private:
    Queue1::Reader reader;
    Queue2::Writer writer;
    const Image::Header& H;
    const Math::Matrix<float>* resampler;
};







class MapWriter
{
  public:
    MapWriter (Queue2& queue, const Image::Header& header, bool fraction_scaling_factor) :
      reader (queue),
      H (header),
      buffer (H, "buffer"),
      interp (H),
      scale (fraction_scaling_factor)
    {
    }

    ~MapWriter () 
    {
      if (scale) {
        Image::Voxel<float> vox (H);
        DataSet::Loop loop ("writing data back to image...", 0, 3);
        for (loop.start (vox, buffer); loop.ok(); loop.next (vox, buffer))
          vox.value() = scale * buffer.value();
      }
      else {
        Image::Voxel<size_t> vox (H);
        DataSet::copy_with_progress_message ("writing data back to image...", vox, buffer, 0, 3);
      }
    }


    void execute () 
    {
      Queue2::Reader::Item item (reader);
      while (item.read()) {
        for (std::set<Voxel>::const_iterator i = item->begin(); i != item->end(); ++i) {
          buffer[0] = i->x;
          buffer[1] = i->y;
          buffer[2] = i->z;
          buffer.value() += 1;
        }
      }
    }


  private:
    Queue2::Reader reader;
    const Image::Header& H;
    DataSet::Buffer<size_t,3> buffer;
    DataSet::Interp::Linear<const Image::Header> interp;
    float scale;
};







EXECUTE {

  Image::Header header (argument[1].get_image());

  bool fibre_fraction = get_options("count").size();

  std::vector<OptBase> opt = get_options ("datatype");
  if (opt.size()) 
    header.datatype().parse (DataType::identifiers[opt[0][0].get_int()]);
  else 
    header.datatype() = fibre_fraction ? DataType::Float32 : DataType::UInt32;

  Tractography::Properties properties;
  Tractography::Reader file;
  file.open (argument[0].get_string(), properties);

  header.axes.ndim() = 3;
  header.comments.push_back (std::string ("track ") + (fibre_fraction ? "fraction" : "count") + " map");
  for (Tractography::Properties::iterator i = properties.begin(); i != properties.end(); ++i) 
    header.comments.push_back (i->first + ": " + i->second);
  for (std::multimap<std::string,std::string>::const_iterator i = properties.roi.begin(); i != properties.roi.end(); ++i)
    header.comments.push_back ("ROI: " + i->first + " " + i->second);
  for (std::vector<std::string>::iterator i = properties.comments.begin(); i != properties.comments.end(); ++i)
    header.comments.push_back ("comment: " + *i);

  const Image::Header header_out = argument[2].get_image (header);

  size_t total_count = properties["total_count"].empty() ? 0 : to<size_t> (properties["total_count"]);
  size_t num_tracks = properties["count"].empty() ? 0 : to<size_t> (properties["count"]);

  Queue1 queue1 ("loaded tracks");
  Queue2 queue2 ("processed tracks");

  TrackLoader loader (queue1, file, num_tracks);
  TrackMapper mapper (queue1, queue2, header_out);
  MapWriter writer (queue2, header_out, total_count ? 1.0 / total_count : 0.0);
  
  Thread::Exec loader_thread (loader, "loader");
  Thread::Array<TrackMapper> mapper_list (mapper);
  Thread::Exec mapper_threads (mapper_list, "mapper");

  writer.execute();
}

