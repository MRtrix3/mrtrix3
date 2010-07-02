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
#include "math/matrix.h"
#include "dwi/tractography/file.h"
#include "dwi/tractography/properties.h"

using namespace MR;

SET_VERSION_DEFAULT;
SET_AUTHOR ("Robert E. Smith (r.smith@brain.org.au) and J-Donald Tournier (d.tournier@brain.org.au)");
SET_COPYRIGHT (NULL);

DESCRIPTION = {
  "convert a tracks file into a map of the fraction of tracks to enter each voxel.",
  NULL
};

ARGUMENTS = {
  Argument ("tracks", "track file", "the input track file.").type_file (),
  Argument ("output", "output image", "the output fraction image").type_image_out(),
  Argument::End
};


OPTIONS = {

  Option ("template", "template image", "an image file to be used as a template for the output (the output image wil have the same transform and field of view).")
    .append (Argument ("image", "template image", "the input template image").type_image_in()),

  Option ("vox", "voxel size", "provide either an isotropic voxel size, or comma-separated list of 3 voxel dimensions.")
    .append (Argument ("size", "voxel size", "the voxel size (in mm).").type_sequence_float()),

  Option ("colour", "coloured map", "add colour to the output image according to the direction of the tracks."),

  Option ("fraction", "output fibre fraction", "produce an image of the fraction of fibres through each voxel (as a proportion of the total number in the file), rather than the count."),

  Option ("datatype", "data type", "specify output image data type.")
    .append (Argument ("spec", "specifier", "the data type specifier.").type_choice (DataType::identifiers)),

  Option ("resample", "resample tracks", "resample the tracks at regular intervals using Hermite interpolation.")
    .append (Argument ("factor", "factor", "the factor by which to resample.").type_integer (1, INT_MAX, 1)),

  Option::End
};



using namespace MR::DWI;
using namespace std;


#define HERMITE_TENSION 0.1
#define DIRECTION_INTEGER_MULTIPLIER 100
#define MAX_TRACKS_READ_FOR_HEADER 1000000



typedef Thread::Queue<vector<Point> > Queue_tracks;




template <typename T> class Gen_interp_matrix {

  public:
    Gen_interp_matrix(const size_t os_factor) {
      if (os_factor > 1) {
        const size_t dim = os_factor - 1;
        Math::Hermite<T> interp(HERMITE_TENSION);
        M = new Math::Matrix<T>(dim, 4);
        for (size_t i = 0; i != dim; ++i) {
          interp.set((float)(i + 1) / (float)os_factor);
          for (size_t j = 0; j != 4; ++j)
            (*M)(i, j) = interp.coef(j);
        }
      } else {
        M = NULL;
      }
    }

    ~Gen_interp_matrix() {
      if (M) {
        delete M;
        M = NULL;
      }
    }

    const Math::Matrix<T>* get_matrix() const {return M;}

  private:
    Math::Matrix<T>* M;

};



template <typename T> class Resampler {

  public:
    Resampler(const Math::Matrix<T>* matrix, const size_t c) :
      M (matrix),
      columns (c),
      data (4, c)
    {
    }

    ~Resampler() {}

    size_t get_columns() const {return columns;}
    bool valid() const {return M;}

    void init(T* a, T* b, T* c) {
      for (size_t i = 0; i != columns; ++i) {
        data(0, i) = 0.0;
        data(1, i) = *a; ++a;
        data(2, i) = *b; ++b;
        data(3, i) = *c; ++c;
      }
    }

    void increment(T* a) {
      for (size_t i = 0; i != columns; ++i) {
        data(0, i) = data(1, i);
        data(1, i) = data(2, i);
        data(2, i) = data(3, i);
        data(3, i) = *a; ++a;
      }
    }

    MR::Math::Matrix<T>& interpolate(Math::Matrix<T>& result) {
      return (Math::mult(result, *M, data));
    }

  private:
    const Math::Matrix<T>* M;
    size_t columns;
    Math::Matrix<T> data;

};



class Voxel
{
  public:
    Voxel() : x (0), y (0), z (0) {}

    Voxel(const size_t _x_, const size_t _y_, const size_t _z_) :
      x (_x_),
      y (_y_),
      z (_z_)
    {
    }

    Voxel(const Point& p) :
      x (Math::round<size_t> (p[0])),
      y (Math::round<size_t> (p[1])),
      z (Math::round<size_t> (p[2]))
    {
    }

    ~Voxel() {}

    Voxel& operator+=(const Voxel& source) {
      x += source.x;
      y += source.y;
      z += source.z;
      return *this;
    }

    bool test_bounds(const Image::Header& H) const {
      return (x >= 0 && x < size_t(H.dim(0)) && y >= 0 && y < size_t(H.dim(1)) && z >= 0 && z < size_t(H.dim(2)));
    }

    size_t x, y, z;

};


class Voxel_dir : public Voxel
{
  public:
    Voxel_dir() :
      Voxel()
    {
    }

    Voxel_dir (const Point& p) :
      Voxel(p),
      dx (0),
      dy (0),
      dz (0)
    {
    }

    Voxel_dir (const Point& p, const Point& dp) :
      Voxel (p),
      dx (abs<int>(Math::round<size_t> (dp[0]))),
      dy (abs<int>(Math::round<size_t> (dp[1]))),
      dz (abs<int>(Math::round<size_t> (dp[2])))
    {
    }

    size_t dx, dy, dz;

};




class TrackLoader
{
  public:
    TrackLoader (Queue_tracks& queue, DWI::Tractography::Reader& file, size_t count) :
      writer (queue),
      reader (file),
      total_count (count)
    {
    }

    void execute ()
    {
      Queue_tracks::Writer::Item item (writer);

      ProgressBar progress ("mapping tracks to image...", total_count);
      while (reader.next (*item)) {
        if (!item.write())
          throw Exception ("error writing to track-mapping queue!");
        ++progress;
      }
    }

  private:
    Queue_tracks::Writer writer;
    DWI::Tractography::Reader& reader;
    size_t total_count;

};






template <class T> class TrackMapper {

  public:
    TrackMapper (Queue_tracks& input, Thread::Queue< vector<T> >& output, const Image::Header& header, const Math::Matrix<float>* r_m = NULL) :
      reader (input),
      writer (output),
      H (header),
      resample_matrix (r_m)
    {
    }

    ~TrackMapper() {}

    void execute ()
    {
      Queue_tracks::Reader::Item in (reader);
      typename Thread::Queue< vector<T> >::Writer::Item out (writer);
      DataSet::Interp::Linear<const Image::Header> interp (H);

      while (in.read()) {
      	out->clear();
      	vector<Point> tck;
      	interp_track(tck, *in, interp);
      	output(tck, H, out);

      	if (!out.write())
      		throw Exception ("error writing to write-back queue!");
      }
    }


  private:
    Queue_tracks::Reader                       reader;
    typename Thread::Queue<vector<T> >::Writer writer;
    const Image::Header& H;
    const Math::Matrix<float>* resample_matrix;


    void interp_prepare(vector<Point>& v) {
      const size_t s = v.size();
      v.insert   (v.begin(), v[ 0 ] + (2 * (v[ 0 ] - v[ 1 ])) - (v[ 1 ] - v[ 2 ]));
      v.push_back(           v[ s ] + (2 * (v[ s ] - v[s-1])) - (v[s-1] - v[s-2]));
    }

    void interp_track(vector<Point>& output, vector<Point>& input, DataSet::Interp::Linear<const Image::Header>& interp) {
      Resampler<float> R (resample_matrix, 3);
      Math::Matrix<float> data (resample_matrix ? resample_matrix->rows() : 0, 3);
      if (R.valid()) {
        interp_prepare(input);
        R.init(input[0].get(), input[1].get(), input[2].get());
        for (size_t i = 3; i < input.size(); ++i) {
          output.push_back (interp.scanner2voxel (input[i-2]));
          R.increment(input[i].get());
          R.interpolate(data);
          for (size_t row = 0; row != data.rows(); ++row)
            output.push_back (interp.scanner2voxel (Point (data(row,0), data(row,1), data(row,2))));
        }
        output.push_back (interp.scanner2voxel (input[input.size() - 2]));
      } else {
        for (vector<Point>::const_iterator i = input.begin(); i != input.end(); ++i)
          output.push_back (interp.scanner2voxel (*i));
      }
    }

    void output(vector<Point>&, const Image::Header& H, typename Thread::Queue< vector<T> >::Writer::Item&);

};


template<> void TrackMapper<Voxel>::output(vector<Point>& tck, const Image::Header& H, Thread::Queue< vector<Voxel> >::Writer::Item& out) {
  for (vector<Point>::const_iterator i = tck.begin(); i != tck.end(); ++i)
    out->push_back(Voxel(*i));
}

template<> void TrackMapper<Voxel_dir>::output(vector<Point>& tck, const Image::Header& H, Thread::Queue< vector<Voxel_dir> >::Writer::Item& out) {
  Point prev_point = tck.front();
  const vector<Point>::const_iterator last_point = tck.end() - 1;
  for (vector<Point>::const_iterator i = tck.begin(); i != last_point; ++i) {
    const Point di = (*(i + 1) - prev_point).normalise() * DIRECTION_INTEGER_MULTIPLIER;
    out->push_back(Voxel_dir(*i, di));
    prev_point = *i;
  }
  out->push_back(Voxel_dir(*last_point, (*last_point - prev_point).normalise() * DIRECTION_INTEGER_MULTIPLIER));
}







template <class T> class MapWriter_Base
{
  public:
    MapWriter_Base (Thread::Queue<vector<T> >& queue, const Image::Header& header, const float fraction_scaling_factor) :
      reader (queue),
      H (header),
      interp (H),
      scale (fraction_scaling_factor)
    {
    }
    ~MapWriter_Base() {}

    virtual void execute() {throw Exception ("Running empty virtual function MapWriter_Base::execute()");}

   protected:
    typename Thread::Queue<vector<T> >::Reader reader;
    const Image::Header& H;
    DataSet::Interp::Linear<const Image::Header> interp;
    float scale;

};


class MapWriter : public MapWriter_Base<Voxel> {

  public:
    MapWriter(Thread::Queue<vector<Voxel> >& queue, const Image::Header& header, const float fraction_scaling_factor) :
      MapWriter_Base<Voxel>(queue, header, fraction_scaling_factor),
      buffer (NULL)
    {
    }

    ~MapWriter ()
    {
      Image::Voxel<float> vox (H);
      DataSet::LoopInOrder loop (vox, "writing image to file...", 0, 3);
      for (loop.start (vox, *buffer); loop.ok(); loop.next (vox, *buffer))
        vox.value() = scale * buffer->value();
      delete buffer; buffer = NULL;
    }

    void execute ()
    {
      Thread::Queue<vector<Voxel> >::Reader::Item item (reader);
      buffer = new DataSet::Buffer<size_t>(H, 3, "buffer");
      while (item.read()) {
        for (vector<Voxel>::const_iterator i = item->begin(); i != item->end(); ++i) {
          if (i->test_bounds(H)) {
            (*buffer)[0] = i->x;
            (*buffer)[1] = i->y;
            (*buffer)[2] = i->z;
            buffer->value() += 1;
          } else {
            debug("Mapping voxel outside image bounds: [" + str(i->x) + " " + str(i->y) + " " + str(i->z) + "]");
          }
        }
      }
    }

  private:
    DataSet::Buffer<size_t>* buffer;

};


class MapWriter_Colour : public MapWriter_Base<Voxel_dir> {

  public:
    MapWriter_Colour(Thread::Queue<vector<Voxel_dir> >& queue, const Image::Header& header, const float fraction_scaling_factor) :
      MapWriter_Base<Voxel_dir>(queue, header, fraction_scaling_factor),
      buffer (NULL)
    {
    }

    ~MapWriter_Colour ()
    {
      scale /= (float)DIRECTION_INTEGER_MULTIPLIER;
      Image::Voxel<float> vox (H);
      DataSet::LoopInOrder loop (vox, "writing image to file...", 0, 3);
      for (loop.start (vox, *buffer); loop.ok(); loop.next (vox, *buffer)) {
        const Voxel temp = buffer->value();
        vox[3] = 0; vox.value() = scale * temp.x;
        vox[3] = 1; vox.value() = scale * temp.y;
        vox[3] = 2; vox.value() = scale * temp.z;
      }
      delete buffer; buffer = NULL;
    }

    void execute ()
    {
      Thread::Queue<vector<Voxel_dir> >::Reader::Item item (reader);
      buffer = new DataSet::Buffer<Voxel>(H, 3, "directional_buffer");
      while (item.read()) {
        for (vector<Voxel_dir>::const_iterator i = item->begin(); i != item->end(); ++i) {
          if (i->test_bounds(H)) {
            (*buffer)[0] = i->x;
            (*buffer)[1] = i->y;
            (*buffer)[2] = i->z;
            buffer->value() += Voxel(i->dx, i->dy, i->dz);
          } else {
            debug("Mapping voxel outside image bounds: [" + str(i->x) + " " + str(i->y) + " " + str(i->z) + "]");
          }
        }
      }
    }


  private:
    DataSet::Buffer<Voxel>*  buffer;

};




void generate_header(Image::Header& header, Tractography::Reader& file, const std::vector<float>& voxel_size) {

  ProgressBar progress ("creating new template image...", 0);

  vector<Point> tck;
  size_t track_counter = 0;

  Point min_values( INFINITY,  INFINITY,  INFINITY);
  Point max_values(-INFINITY, -INFINITY, -INFINITY);

  while (file.next(tck) && track_counter++ < MAX_TRACKS_READ_FOR_HEADER) {
    for (vector<Point>::const_iterator i = tck.begin(); i != tck.end(); ++i) {
      min_values[0] = MIN(min_values[0], (*i)[0]);
      max_values[0] = MAX(max_values[0], (*i)[0]);
      min_values[1] = MIN(min_values[1], (*i)[1]);
      max_values[1] = MAX(max_values[1], (*i)[1]);
      min_values[2] = MIN(min_values[2], (*i)[2]);
      max_values[2] = MAX(max_values[2], (*i)[2]);
    }
    ++progress;
  }

  min_values -= Point(3*voxel_size[0], 3*voxel_size[1], 3*voxel_size[2]);
  max_values += Point(3*voxel_size[0], 3*voxel_size[1], 3*voxel_size[2]);

  header.name() = "tckmap_image_header";
  header.axes.ndim() = 3;

  for (size_t i = 0; i != 3; ++i) {
    header.axes.dim(i)   = Math::ceil((max_values[i] - min_values[i]) / voxel_size[i]);
    header.axes.vox(i)   = voxel_size[i];
    header.axes.units(i) = Image::Axes::millimeters;
  }

  header.axes.description(0) = Image::Axes::left_to_right;
  header.axes.description(1) = Image::Axes::posterior_to_anterior;
  header.axes.description(2) = Image::Axes::inferior_to_superior;

  header.axes.stride(0) = 1;
  header.axes.stride(1) = header.axes.dim(0);
  header.axes.stride(2) = header.axes.dim(0) * header.axes.dim(1);

  Math::Matrix<float> t_matrix(4, 4);
  t_matrix(0, 0) = 1.0; t_matrix(0, 1) = 0.0; t_matrix(0, 2) = 0.0; t_matrix(0, 3) = min_values[0];
  t_matrix(1, 0) = 0.0; t_matrix(1, 1) = 1.0; t_matrix(1, 2) = 0.0; t_matrix(1, 3) = min_values[1];
  t_matrix(2, 0) = 0.0; t_matrix(2, 1) = 0.0; t_matrix(2, 2) = 1.0; t_matrix(2, 3) = min_values[2];
  t_matrix(3, 0) = 0.0; t_matrix(3, 1) = 0.0; t_matrix(3, 2) = 0.0; t_matrix(3, 3) = 1.0;
  header.transform() = t_matrix;

}



void oversample_header (Image::Header& header, const std::vector<float>& voxel_size) {

  info("Oversampling header...");

  // FIXME Introducing voxel anisotropy to a template image produces shear distortion

  for (size_t i = 0; i != 3; ++i) {

    header.transform()(i, 3) += 0.5 * (voxel_size[i] - header.axes.vox(i));

    header.axes.dim(i) = Math::ceil(header.axes.dim(i) * header.axes.vox(i) / voxel_size[i]);
    header.axes.vox(i) = voxel_size[i];

  }

}


EXECUTE {

  Tractography::Properties properties;
  Tractography::Reader file;
  file.open (argument[0].get_string(), properties);

  const size_t num_tracks = properties["count"]    .empty() ? 0   : to<size_t> (properties["count"]);
  const float  step_size  = properties["step_size"].empty() ? 0.0 : to<float>  (properties["step_size"]);

  const bool colour         = get_options("colour")  .size();
  const bool fibre_fraction = get_options("fraction").size();
  const bool template_image = get_options("template").size();

  std::vector<float> voxel_size;
  if (get_options("vox").size())
    voxel_size = parse_floats(get_options("vox")[0][0].get_string());

  if (voxel_size.size() == 1)
    voxel_size.assign(3, voxel_size.front());
  else if (!voxel_size.empty() && voxel_size.size() != 3)
    throw Exception ("The voxel size parameter must either be a single isotropic value, or a list of 3 comma-separated voxel dimensions");

  if (!voxel_size.empty())
    info("Creating image with voxel dimensions [" + str(voxel_size[0]) + " " + str(voxel_size[1]) + " " + str(voxel_size[2]) + "]");

  Image::Header header;
  if (template_image) {
    header = get_options("template")[0][0].get_image();
    if (!voxel_size.empty())
      oversample_header(header, voxel_size);
  } else {
    if (voxel_size.empty())
      throw Exception ("You must provide at least one of either a template image, or a desired voxel size, for the output image");
    generate_header(header, file, voxel_size);
    file.close();
    file.open(argument[0].get_string(), properties);
  }

  vector<OptBase> opt = get_options ("datatype");
  if (opt.size())
    header.datatype().parse (DataType::identifiers[opt[0][0].get_int()]);
  else
    header.datatype() = (fibre_fraction || colour) ? DataType::Float32 : DataType::UInt32;

  for (Tractography::Properties::iterator i = properties.begin(); i != properties.end(); ++i)
    header.comments.push_back (i->first + ": " + i->second);
  for (multimap<std::string,std::string>::const_iterator i = properties.roi.begin(); i != properties.roi.end(); ++i)
    header.comments.push_back ("ROI: " + i->first + " " + i->second);
  for (vector<std::string>::iterator i = properties.comments.begin(); i != properties.comments.end(); ++i)
    header.comments.push_back ("comment: " + *i);

  float scaling_factor = fibre_fraction ? 1.0 / (float)num_tracks : 1.0;
  scaling_factor *= properties["step_size"].empty() ? 1.0 : (to<float>(properties["step_size"]) / minvalue(header.vox(0), header.vox(1), header.vox(2)));
  header.comments.push_back("scaling_factor: " + str(scaling_factor));
  info ("Intensity scaling factor set to " + str(scaling_factor));

  unsigned int resample_factor;
  opt = get_options("resample");
  if (opt.size()) {
    resample_factor = opt[0][0].get_int();
    info ("Track interpolation factor manually set to " + str(resample_factor));
  } else if (step_size) {
    const float min_vox = minvalue(header.vox(0), header.vox(1), header.vox(2));
    resample_factor = Math::ceil(step_size / min_vox);
    info ("Track interpolation factor automatically set to " + str(resample_factor));
    properties["step_size"] = str(step_size / resample_factor);
  } else {
    resample_factor = 1;
    info ("Track interpolation off; no track step size information in header");
  }

  Gen_interp_matrix<float> coef_matrix(resample_factor);


  // TODO Enable segmentwise processing in scenario where not enough memory is available for both the buffer and the memory-mapped output file

  if (colour) {

    header.axes.ndim() = 4;
    header.axes.dim(3) = 3;
    header.axes.description(3) = "direction";
    for (size_t i = 0; i != 3; ++i) {
      if (abs(header.axes.stride(i)) == 1)
        header.axes.stride(3) = header.axes.stride(i) < 0 ? -1 : 1;
      header.axes.stride(i) *= 3;
    }
    header.comments.push_back (std::string ("coloured track density map"));

    const Image::Header header_out = argument[1].get_image (header);

    Queue_tracks queue1 ("loaded tracks");
    Thread::Queue< vector<Voxel_dir> > queue2 ("processed tracks");

    TrackLoader            loader (queue1, file, num_tracks);
    TrackMapper<Voxel_dir> mapper (queue1, queue2, header_out, coef_matrix.get_matrix());
    MapWriter_Colour       writer (queue2, header_out, scaling_factor);

    Thread::Exec loader_thread (loader, "loader");
    Thread::Array< TrackMapper<Voxel_dir> > mapper_list (mapper);
    Thread::Exec mapper_threads (mapper_list, "mapper");

    writer.execute();

  } else {

    header.axes.ndim() = 3;
    header.comments.push_back (std::string (("track ") + str(fibre_fraction ? "fraction" : "count") + " map"));

    const Image::Header header_out = argument[1].get_image (header);

    Queue_tracks queue1 ("loaded tracks");
    Thread::Queue< vector<Voxel> > queue2 ("processed tracks");

    TrackLoader        loader (queue1, file, num_tracks);
    TrackMapper<Voxel> mapper (queue1, queue2, header_out, coef_matrix.get_matrix());
    MapWriter          writer (queue2, header_out, scaling_factor);

    Thread::Exec loader_thread (loader, "loader");
    Thread::Array< TrackMapper<Voxel> > mapper_list (mapper);
    Thread::Exec mapper_threads (mapper_list, "mapper");

    writer.execute();

  }

}

