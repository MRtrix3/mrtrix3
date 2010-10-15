/*
    Copyright 2010 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier and Robert E. Smith, 03/06/10.

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
#include <vector>
#include <set>

#include "app.h"
#include "progressbar.h"
#include "get_set.h"
#include "point.h"
#include "image/voxel.h"
#include "dataset/misc.h"
#include "dataset/buffer.h"
#include "dataset/interp/linear.h"
#include "thread/exec.h"
#include "thread/queue.h"
#include "math/hermite.h"
#include "math/matrix.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"

using namespace MR;

SET_VERSION_DEFAULT;
SET_AUTHOR ("Robert E. Smith (r.smith@brain.org.au) and J-Donald Tournier (d.tournier@brain.org.au)");
SET_COPYRIGHT (NULL);

DESCRIPTION = {
  "convert a tracks file into a map of the number of tracks to enter each voxel.",
  NULL
};

ARGUMENTS = {
  Argument ("tracks", "track file", "the input track file.").type_file (),
  Argument ("output", "output image", "the output track density image").type_image_out(),
  Argument::End
};


OPTIONS = {

  Option ("template", "template image", "an image file to be used as a template for the output (the output image wil have the same transform and field of view).")
    .append (Argument ("image", "template image", "the input template image").type_image_in()),

  Option ("vox", "voxel size", "provide either an isotropic voxel size, or comma-separated list of 3 voxel dimensions.")
    .append (Argument ("size", "voxel size", "the voxel size (in mm).").type_sequence_float()),

  Option ("colour", "coloured map", "add colour to the output image according to the direction of the tracks."),

  Option ("fraction", "output fibre fraction", "produce an image of the fraction of fibres through each voxel (as a proportion of the total number in the file), rather than the count."),

  Option ("alt", "alternative count mode", "alternative track counting mode which improves rotational invariance, but individual voxel values no longer correspond to an exact track count"),

  Option ("lengthnorm", "weight by track length", "weight the contribution of each track by the inverse of its length."),

  Option ("datatype", "data type", "specify output image data type.")
    .append (Argument ("spec", "specifier", "the data type specifier.").type_choice (DataType::identifiers)),

  Option ("resample", "resample tracks", "resample the tracks at regular intervals using Hermite interpolation.")
    .append (Argument ("factor", "factor", "the factor by which to resample.").type_integer (1, INT_MAX, 1)),

  Option::End
};



using namespace MR::DWI;


#define HERMITE_TENSION 0.1
#define MAX_TRACKS_READ_FOR_HEADER 1000000
#define MAX_VOXEL_STEP_RATIO_DEFAULT_MODE 0.5


class Voxel : public Point<size_t>
{
  public:
  Voxel (const size_t x, const size_t y, const size_t z) { p[0] = x; p[1] = y; p[2] = z; }
  Voxel () { memset (p, 0x00, 3 * sizeof(size_t)); }
  bool operator< (const Voxel V) const { return (p[2] < V.p[2] ? true : (p[1] < V.p[1] ? true : (p[0] < V.p[0] ? true : false))); }
};


Voxel round (const Point<float>& p) 
{ 
  assert (finite (p[0]) && finite (p[1]) && finite (p[2]));
  return (Voxel (Math::round<size_t> (p[0]), Math::round<size_t> (p[1]), Math::round<size_t> (p[2])));
}
bool check (const Voxel& V, const Image::Header& H) 
{
  return (V[0] < size_t(H.dim(0)) && V[1] < size_t(H.dim(1)) && V[2] < size_t(H.dim(2)));
}

class VoxelDir : public Voxel 
{
  public:
    VoxelDir () :
      dir (Point<float> (0.0, 0.0, 0.0))
    {
      memset(p, 0x00, 3 * sizeof(size_t));
    }
    VoxelDir (const Voxel& V) :
      dir (Point<float> (0.0, 0.0, 0.0))
    {
      memcpy (p, V.get(), 3 * sizeof(size_t));
    }
    Point<float> dir;
    VoxelDir& operator=  (const Voxel& V)         { Voxel::operator= (V); return (*this); }
    bool      operator<  (const VoxelDir V) const { return Voxel::operator< (V); }
};
Point<float> abs (const Point<float>& d) 
{
  return (Point<float> (Math::abs(d[0]), Math::abs(d[1]), Math::abs(d[2])));
}


class SetVoxel : public std::set<Voxel> { public: size_t num_points; };
class VectorVoxel : public std::vector<Voxel> { public: size_t num_points; };
class SetVoxelDir : public std::set<VoxelDir> { public: size_t num_points; };
class VectorVoxelDir : public std::vector<VoxelDir> { public: size_t num_points; };

typedef Thread::Queue<std::vector<Point<float> > >    TrackQueue;



template <typename T> class Resampler 
{
  public:
    Resampler (const Math::Matrix<T>& interp_matrix, const size_t c) :
      M (interp_matrix),
      columns (c),
      data (4, c) { }

    ~Resampler() { }

    size_t get_columns () const { return (columns); }
    bool valid () const { return (M.is_set()); }

    void init (const T* a, const T* b, const T* c) 
    {
      for (size_t i = 0; i != columns; ++i) {
        data(0,i) = 0.0;
        data(1,i) = a[i];
        data(2,i) = b[i];
        data(3,i) = c[i];
      }
    }

    void increment (const T* a) 
    {
      for (size_t i = 0; i != columns; ++i) {
        data(0,i) = data(1,i);
        data(1,i) = data(2,i);
        data(2,i) = data(3,i);
        data(3,i) = a[i];
      }
    }

    MR::Math::Matrix<T>& interpolate (Math::Matrix<T>& result) const 
    {
      return (Math::mult (result, M, data));
    }

  private:
    const Math::Matrix<T>& M;
    size_t columns;
    Math::Matrix<T> data;
};



class TrackLoader
{
  public:
    TrackLoader (TrackQueue& queue, DWI::Tractography::Reader<float>& file, size_t count) :
      writer (queue),
      reader (file),
      total_count (count)
    {
    }

    void execute ()
    {
      TrackQueue::Writer::Item item (writer);

      ProgressBar progress ("mapping tracks to image...", total_count);
      while (reader.next (*item)) {
        if (!item.write())
          throw Exception ("error writing to track-mapping queue!");
        ++progress;
      }
    }

  private:
    TrackQueue::Writer writer;
    DWI::Tractography::Reader<float>& reader;
    size_t total_count;

};






template <typename Cont> class TrackMapper
{
  public:
    typedef DataSet::Interp::Linear<const Image::Header> HeaderInterp;

    TrackMapper (TrackQueue& input, Thread::Queue<Cont>& output, const Image::Header& header, const Math::Matrix<float>& interp_matrix) :
      reader (input),
      writer (output),
      H (header),
      resample_matrix (interp_matrix)
    {
    }

    void execute ()
    {
      TrackQueue::Reader::Item in (reader);
      typename Thread::Queue<Cont>::Writer::Item out (writer);
      HeaderInterp interp (H);
      Resampler<float> R (resample_matrix, 3);
      Math::Matrix<float> data;
      if (R.valid()) {
        assert (resample_matrix.is_set());
        assert (resample_matrix.rows());
        data.allocate (resample_matrix.rows(), 3);
      }

      while (in.read()) {
      	out->clear();
        if (R.valid()) 
          interp_track (*in, R, data);
      	voxelise (*out, *in, interp);
        out->num_points = in->size();

      	if (!out.write())
          throw Exception ("error writing to write-back queue!");
      }
    }


  private:
    TrackQueue::Reader reader;
    typename Thread::Queue<Cont>::Writer writer;
    const Image::Header& H;
    const Math::Matrix<float>& resample_matrix;


    void interp_prepare (std::vector<Point<float> >& v) 
    {
      const size_t s = v.size();
      v.insert   (v.begin(), v[ 0 ] + (float(2.0) * (v[ 0 ] - v[ 1 ])) - (v[ 1 ] - v[ 2 ]));
      v.push_back(           v[ s ] + (float(2.0) * (v[ s ] - v[s-1])) - (v[s-1] - v[s-2]));
    }

    void interp_track (
        std::vector<Point<float> >& tck,
        Resampler<float>& R,
        Math::Matrix<float>& data)
    {
      std::vector<Point<float> > out;
      interp_prepare (tck);
      R.init (tck[0].get(), tck[1].get(), tck[2].get());
      for (size_t i = 3; i < tck.size(); ++i) {
        out.push_back (tck[i-2]);
        R.increment (tck[i].get());
        R.interpolate (data);
        for (size_t row = 0; row != data.rows(); ++row)
          out.push_back (Point<float> (data(row,0), data(row,1), data(row,2)));
      }
      out.push_back (tck[tck.size() - 2]);
      out.swap (tck);
    }


    void voxelise (Cont& voxels, const std::vector< Point<float> >& tck, const HeaderInterp& interp) const;

};




template<> void TrackMapper<VectorVoxel>::voxelise (VectorVoxel& voxels, const std::vector< Point<float> >& tck, const HeaderInterp& interp) const
{
  Voxel vox;
  for (std::vector<Point<float> >::const_iterator i = tck.begin(); i != tck.end(); ++i) {
    vox = round (interp.scanner2voxel (*i));
    if (check (vox, H)) 
      voxels.push_back (vox);
  }
}

template<> void TrackMapper<SetVoxel>::voxelise (SetVoxel& voxels, const std::vector< Point<float> >& tck, const HeaderInterp& interp) const
{
  Voxel vox;
  for (std::vector<Point<float> >::const_iterator i = tck.begin(); i != tck.end(); ++i) {
    vox = round (interp.scanner2voxel (*i));
    if (check (vox, H))
      voxels.insert (vox);
  }
}

template<> void TrackMapper<VectorVoxelDir>::voxelise (VectorVoxelDir& voxels, const std::vector< Point<float> >& tck, const HeaderInterp& interp) const
{
  std::vector< Point<float> >::const_iterator prev = tck.begin();
  const std::vector< Point<float> >::const_iterator last = tck.end() - 1;

  VoxelDir vox;
  for (std::vector< Point<float> >::const_iterator i = tck.begin(); i != last; ++i) {
    vox = round (interp.scanner2voxel (*i));
    if (check (vox, H)) {
      vox.dir = (*(i+1) - *prev).normalise();
      voxels.push_back (vox);
    }
    prev = i;
  }
  vox = round (interp.scanner2voxel (*last));
  if (check (vox, H)) {
    vox.dir = (*last - *prev).normalise();
    voxels.push_back (vox);
  }
}

template<> void TrackMapper<SetVoxelDir>::voxelise (SetVoxelDir& voxels, const std::vector< Point<float> >& tck, const HeaderInterp& interp) const
{
  std::vector< Point<float> >::const_iterator prev = tck.begin();
  const std::vector< Point<float> >::const_iterator last = tck.end() - 1;

  VoxelDir vox;
  VoxelDir current_vox (round (interp.scanner2voxel (*(tck.begin()))));
  for (std::vector< Point<float> >::const_iterator i = tck.begin() + 1; i != last; ++i) {
    vox = round (interp.scanner2voxel (*i));
    if (check (vox, H)) {
      if (vox != current_vox) {
        current_vox.dir = (*(i+1) - *prev).normalise();
        voxels.insert (current_vox);
        prev = i-1;
        current_vox = vox;
      }
    }
  }
  vox = round (interp.scanner2voxel (*last));
  if (check (vox, H)) {
    current_vox.dir = (*last - *prev).normalise();
    voxels.insert (current_vox);
    if (vox != current_vox) {
      vox.dir = (*last - *(last - 1)).normalise();
      voxels.insert (vox);
    }
  }
}







template <typename Cont>
class MapWriterBase
{
  public:
    MapWriterBase (Thread::Queue<Cont>& queue, const Image::Header& header, const float fraction_scaling_factor, bool use_length) :
      reader (queue),
      H (header),
      scale (fraction_scaling_factor),
      weight_by_length (use_length)
    {
    }

    virtual void execute () = 0;

   protected:
    typename Thread::Queue<Cont>::Reader reader;
    const Image::Header& H;
    float scale;
    bool weight_by_length;

};


template <typename Cont>
class MapWriter : public MapWriterBase<Cont>
{
  public:
    MapWriter (Thread::Queue<Cont>& queue, const Image::Header& header, const float fraction_scaling_factor, bool use_length) :
      MapWriterBase<Cont> (queue, header, fraction_scaling_factor, use_length),
      buffer (MapWriterBase<Cont>::H, 3, "buffer")
    {
    }

    ~MapWriter ()
    {
      Image::Voxel<float> vox (MapWriterBase<Cont>::H);
      DataSet::LoopInOrder loop (vox, "writing image to file...", 0, 3);
      for (loop.start (vox, buffer); loop.ok(); loop.next (vox, buffer))
        vox.value() = MapWriterBase<Cont>::scale * buffer.value();
    }

    void execute ()
    {
      typename Thread::Queue<Cont>::Reader::Item item (MapWriterBase<Cont>::reader);
      while (item.read()) {
        for (typename Cont::const_iterator i = item->begin(); i != item->end(); ++i) {
          buffer[0] = (*i)[0];
          buffer[1] = (*i)[1];
          buffer[2] = (*i)[2];
          buffer.value() += this->weight_by_length ? 1.0 / item->num_points : 1.0;
        }
      }
    }

  private:
    DataSet::Buffer<float> buffer;

};


template <typename Cont>
class MapWriterColour : public MapWriterBase<Cont> {

  public:
    MapWriterColour(Thread::Queue<Cont>& queue, const Image::Header& header, const float fraction_scaling_factor, bool use_length) :
      MapWriterBase<Cont>(queue, header, fraction_scaling_factor, use_length),
      buffer (MapWriterBase<Cont>::H, 3, "directional_buffer")
    {
    }

    ~MapWriterColour ()
    {
      Image::Voxel<float> vox (MapWriterBase<Cont>::H);
      DataSet::LoopInOrder loop (vox, "writing image to file...", 0, 3);
      for (loop.start (vox, buffer); loop.ok(); loop.next (vox, buffer)) {
        const Point<float>  temp = buffer.value();
        vox[3] = 0; vox.value() = MapWriterBase<Cont>::scale * temp[0];
        vox[3] = 1; vox.value() = MapWriterBase<Cont>::scale * temp[1];
        vox[3] = 2; vox.value() = MapWriterBase<Cont>::scale * temp[2];
      }
    }

    void execute ()
    {
      typename Thread::Queue<Cont>::Reader::Item item (MapWriterBase<Cont>::reader);
      while (item.read()) {
        for (typename Cont::const_iterator i = item->begin(); i != item->end(); ++i) {
          buffer[0] = (*i)[0];
          buffer[1] = (*i)[1];
          buffer[2] = (*i)[2];
          Point<float> val = abs (i->dir);
          if (this->weight_by_length)
            val /= item->num_points;
          buffer.value() += val;
        }
      }
    }


  private:
    DataSet::Buffer<Point<float> > buffer;
};




void generate_header (Image::Header& header, Tractography::Reader<float>& file, const std::vector<float>& voxel_size) 
{

  std::vector<Point<float> > tck;
  size_t track_counter = 0;

  Point<float>  min_values ( INFINITY,  INFINITY,  INFINITY);
  Point<float>  max_values (-INFINITY, -INFINITY, -INFINITY);

  {
    ProgressBar progress ("creating new template image...", 0);
    while (file.next(tck) && track_counter++ < MAX_TRACKS_READ_FOR_HEADER) {
      for (std::vector<Point<float> >::const_iterator i = tck.begin(); i != tck.end(); ++i) {
        min_values[0] = std::min (min_values[0], (*i)[0]);
        max_values[0] = std::max (max_values[0], (*i)[0]);
        min_values[1] = std::min (min_values[1], (*i)[1]);
        max_values[1] = std::max (max_values[1], (*i)[1]);
        min_values[2] = std::min (min_values[2], (*i)[2]);
        max_values[2] = std::max (max_values[2], (*i)[2]);
      }
      ++progress;
    }
  }

  min_values -= Point<float>  (3.0*voxel_size[0], 3.0*voxel_size[1], 3.0*voxel_size[2]);
  max_values += Point<float>  (3.0*voxel_size[0], 3.0*voxel_size[1], 3.0*voxel_size[2]);

  header.name() = "tckmap image header";
  header.axes.ndim() = 3;

  for (size_t i = 0; i != 3; ++i) {
    header.axes.dim(i)    = Math::ceil((max_values[i] - min_values[i]) / voxel_size[i]);
    header.axes.vox(i)    = voxel_size[i];
    header.axes.stride(i) = i+1;
    header.axes.units(i)  = Image::Axes::millimeters;
  }

  header.axes.description(0) = Image::Axes::left_to_right;
  header.axes.description(1) = Image::Axes::posterior_to_anterior;
  header.axes.description(2) = Image::Axes::inferior_to_superior;

  header.transform().allocate (4,4);
  header.transform().identity();
  header.transform()(0,3) = min_values[0];
  header.transform()(1,3) = min_values[1];
  header.transform()(2,3) = min_values[2];

}






template <typename T> Math::Matrix<T> gen_interp_matrix (const size_t os_factor) 
{
  Math::Matrix<T> M;
  if (os_factor > 1) {
    const size_t dim = os_factor - 1;
    Math::Hermite<T> interp (HERMITE_TENSION);
    M.allocate (dim, 4);
    for (size_t i = 0; i != dim; ++i) {
      interp.set ((i+1.0) / float(os_factor));
      for (size_t j = 0; j != 4; ++j)
        M(i,j) = interp.coef(j);
    }
  }
  return (M);
}





void oversample_header (Image::Header& header, const std::vector<float>& voxel_size) 
{
  info ("oversampling header...");

  for (size_t i = 0; i != 3; ++i) {
    header.transform()(i, 3) += 0.5 * (voxel_size[i] - header.axes.vox(i));
    header.axes.dim(i) = Math::ceil(header.axes.dim(i) * header.axes.vox(i) / voxel_size[i]);
    header.axes.vox(i) = voxel_size[i];
  }
}





EXECUTE {

  Tractography::Properties properties;
  Tractography::Reader<float> file;
  file.open (argument[0].get_string(), properties);

  const size_t num_tracks = properties["count"]    .empty() ? 0   : to<size_t> (properties["count"]);
  const float  step_size  = properties["step_size"].empty() ? 0.0 : to<float>  (properties["step_size"]);

  const bool colour           = get_options("colour")    .size();
  const bool fibre_fraction   = get_options("fraction")  .size();
  const bool alt_mode         = get_options("alt")       .size();
  const bool weight_by_length = get_options("lengthnorm").size();

  std::vector<float> voxel_size;
  OptionList opt = get_options("vox");
  if (opt.size())
    voxel_size = parse_floats(opt[0][0].get_string());

  if (voxel_size.size() == 1)
    voxel_size.assign (3, voxel_size.front());
  else if (!voxel_size.empty() && voxel_size.size() != 3)
    throw Exception ("voxel size must either be a single isotropic value, or a list of 3 comma-separated voxel dimensions");

  if (!voxel_size.empty())
    info("creating image with voxel dimensions [ " + str(voxel_size[0]) + " " + str(voxel_size[1]) + " " + str(voxel_size[2]) + " ]");

  Image::Header header;
  opt = get_options ("template");
  if (opt.size()) {
    header = opt[0][0].get_image();
    if (!voxel_size.empty())
      oversample_header (header, voxel_size);
  }
  else {
    if (voxel_size.empty())
      throw Exception ("please specify either a template image or the desired voxel size");
    generate_header (header, file, voxel_size);
    file.close();
    file.open (argument[0].get_string(), properties);
  }

  opt = get_options ("datatype");
  if (opt.size())
    header.datatype().parse (DataType::identifiers[opt[0][0].get_int()]);
  else
    header.datatype() = (fibre_fraction || colour || alt_mode || weight_by_length) ? DataType::Float32 : DataType::UInt32;

  for (Tractography::Properties::iterator i = properties.begin(); i != properties.end(); ++i)
    header.comments.push_back (i->first + ": " + i->second);
  for (std::multimap<std::string,std::string>::const_iterator i = properties.roi.begin(); i != properties.roi.end(); ++i)
    header.comments.push_back ("ROI: " + i->first + " " + i->second);
  for (std::vector<std::string>::iterator i = properties.comments.begin(); i != properties.comments.end(); ++i)
    header.comments.push_back ("comment: " + *i);

  float scaling_factor = fibre_fraction ? 1.0 / float(num_tracks) : 1.0;

  size_t resample_factor;
  opt = get_options ("resample");
  if (opt.size()) {
    resample_factor = opt[0][0].get_int();
    info ("track interpolation factor manually set to " + str(resample_factor));
  } 
  else if (step_size) {
    resample_factor = alt_mode ?
                      Math::ceil<size_t> (step_size /  minvalue (header.vox(0), header.vox(1), header.vox(2))) :
                      Math::ceil<size_t> (step_size / (minvalue (header.vox(0), header.vox(1), header.vox(2)) * MAX_VOXEL_STEP_RATIO_DEFAULT_MODE));
    info ("track interpolation factor automatically set to " + str(resample_factor));
  } 
  else {
    resample_factor = 1;
    info ("track interpolation off; no track step size information in header");
  }

  if (alt_mode) {
    scaling_factor *= (step_size / float(resample_factor)) / minvalue (header.vox(0), header.vox(1), header.vox(2));
    info ("intensity scaling factor set to " + str(scaling_factor));
  }

  header.comments.push_back("scaling_factor: " + str(scaling_factor));

  Math::Matrix<float> interp_matrix (gen_interp_matrix<float> (resample_factor));

  TrackQueue queue1 ("loaded tracks");

  if (colour) {

    header.axes.ndim() = 4;
    header.axes.dim(3) = 3;
    header.axes.stride(0) += header.axes.stride(0) > 0 ? 1 : -1;
    header.axes.stride(1) += header.axes.stride(1) > 0 ? 1 : -1;
    header.axes.stride(2) += header.axes.stride(2) > 0 ? 1 : -1;
    header.axes.stride(3) = 1;
    header.axes.description(3) = "direction";
    header.comments.push_back (std::string ("coloured track density map"));

    const Image::Header header_out = argument[1].get_image (header);

    if (alt_mode) {

      Thread::Queue   <VectorVoxelDir> queue2 ("processed tracks");
      TrackLoader                               loader (queue1, file, num_tracks);
      TrackMapper     <VectorVoxelDir> mapper (queue1, queue2, header_out, interp_matrix);
      MapWriterColour <VectorVoxelDir> writer (queue2, header_out, scaling_factor, weight_by_length);

      Thread::Exec loader_thread (loader, "loader");
      Thread::Array< TrackMapper<VectorVoxelDir> > mapper_list (mapper);
      Thread::Exec mapper_threads (mapper_list, "mapper");

      writer.execute();

    }
    else {

      Thread::Queue   <SetVoxelDir> queue2 ("processed tracks");
      TrackLoader                            loader (queue1, file, num_tracks);
      TrackMapper     <SetVoxelDir> mapper (queue1, queue2, header_out, interp_matrix);
      MapWriterColour <SetVoxelDir> writer (queue2, header_out, scaling_factor, weight_by_length);

      Thread::Exec loader_thread (loader, "loader");
      Thread::Array< TrackMapper<SetVoxelDir> > mapper_list (mapper);
      Thread::Exec mapper_threads (mapper_list, "mapper");

      writer.execute();

    }

  } 
  else {

    header.axes.ndim() = 3;
    header.comments.push_back (std::string (("track ") + str(fibre_fraction ? "fraction" : "count") + " map"));

    const Image::Header header_out = argument[1].get_image (header);

    if (alt_mode) {

      Thread::Queue<VectorVoxel> queue2 ("processed tracks");
      TrackLoader                         loader (queue1, file, num_tracks);
      TrackMapper  <VectorVoxel> mapper (queue1, queue2, header_out, interp_matrix);
      MapWriter    <VectorVoxel> writer (queue2, header_out, scaling_factor, weight_by_length);

      Thread::Exec loader_thread (loader, "loader");
      Thread::Array< TrackMapper<VectorVoxel> > mapper_list (mapper);
      Thread::Exec mapper_threads (mapper_list, "mapper");

      writer.execute();

    }
    else {

      Thread::Queue<SetVoxel> queue2 ("processed tracks");
      TrackLoader                      loader (queue1, file, num_tracks);
      TrackMapper  <SetVoxel> mapper (queue1, queue2, header_out, interp_matrix);
      MapWriter    <SetVoxel> writer (queue2, header_out, scaling_factor, weight_by_length);

      Thread::Exec loader_thread (loader, "loader");
      Thread::Array< TrackMapper<SetVoxel> > mapper_list (mapper);
      Thread::Exec mapper_threads (mapper_list, "mapper");

      writer.execute();

    }

  }

}

