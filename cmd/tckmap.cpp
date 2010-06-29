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
    .append (Argument ("image", "template image", "the input template image").type_image_in())
    .append (Argument ("os", "oversampling ratio", "the oversampling ratio by which the voxel size will be reduced").type_float (1e-6, 1e6, 1.0)),

  Option ("vox", "voxel size", "generate the map at a given isotropic voxel size without the use of a template image.")
    .append (Argument ("size", "voxel size", "the voxel size (in mm).").type_float(1e-6, 1e6, 1.0)),

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
    Voxel () { }
    Voxel (const Point& p) :
      x (Math::round<size_t> (p[0])),
      y (Math::round<size_t> (p[1])),
      z (Math::round<size_t> (p[2])),
      dx (0),
      dy (0),
      dz (0)
  {
  }

    Voxel (const Point& p, const Point& dp) :
      x (Math::round<size_t> ( p[0])),
      y (Math::round<size_t> ( p[1])),
      z (Math::round<size_t> ( p[2])),
      dx (abs<int>(Math::round<size_t> (dp[0]))),
      dy (abs<int>(Math::round<size_t> (dp[1]))),
      dz (abs<int>(Math::round<size_t> (dp[2])))
  {
  }



    /*
       bool operator< (const Voxel& v) const
       {
       if (x < v.x) return (true);
       if (x > v.x) return (false);
       if (y < v.y) return (true);
       if (y > v.y) return (false);
       if (z < v.z) return (true);
       return (false);
       }
     */

    bool test_bounds(const Image::Header& H) const {
      return (x >= 0 && x < size_t(H.dim(0)) && y >= 0 && y < size_t(H.dim(1)) && z >= 0 && z < size_t(H.dim(2)));
    }


    size_t x, y, z;
    size_t dx, dy, dz;



};



class Xyz
{
  public:
    Xyz() : x (0), y (0), z (0) {}

    Xyz(const size_t _x_, const size_t _y_, const size_t _z_) :
      x (_x_),
      y (_y_),
      z (_z_)
  {
  }

    ~Xyz() {}

    friend Xyz& operator+=(Xyz& dest, const Xyz& source) {
      dest.x += source.x;
      dest.y += source.y;
      dest.z += source.z;
      return dest;
    }

    size_t x, y, z;
};



typedef Thread::Queue<vector<Point> > Queue1;
typedef Thread::Queue<vector<Voxel> > Queue2;




class TrackLoader
{
  public:
    TrackLoader (Queue1& queue, DWI::Tractography::Reader& file, size_t count) :
      writer (queue),
      reader (file),
      total_count (count)
  {
  }

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
    TrackMapper (Queue1& input, Queue2& output, const Image::Header& header, const Math::Matrix<float>* r_m = NULL) :
      reader (input),
      writer (output),
      H (header),
      resample_matrix (r_m)
    {
    }

    ~TrackMapper() {}

    void execute ()
    {
      Queue1::Reader::Item in (reader);
      Queue2::Writer::Item out (writer);
      DataSet::Interp::Linear<const Image::Header> interp (H);

      while (in.read()) {
      	out->clear();
      	vector<Point> tck;
      	interp_track(tck, *in, interp);
      	if (H.axes.ndim() == 4)
      	  output_colour  (tck, H, out);
      	else
      	  output_standard(tck, H, out);

      	if (!out.write())
      		throw Exception ("error writing to write-back queue!");
      }
    }


  private:
    Queue1::Reader reader;
    Queue2::Writer writer;
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

    void output_standard(vector<Point>& tck, const Image::Header& H, Queue2::Writer::Item& out) {
      for (vector<Point>::const_iterator i = tck.begin(); i != tck.end(); ++i) {
        const Voxel v(*i);
        if (v.test_bounds(H))
          out->push_back(v);
      }
    }

    void output_colour(vector<Point>& tck, const Image::Header& H, Queue2::Writer::Item& out) {
      Point prev_point = tck.front();
      const vector<Point>::const_iterator last_point = tck.end() - 1;
      for (vector<Point>::const_iterator i = tck.begin(); i != last_point; ++i) {
        const Point di = (*(i + 1) - prev_point).normalise() * DIRECTION_INTEGER_MULTIPLIER;
        const Voxel v(*i, di);
        if (v.test_bounds(H))
          out->push_back(v);
        prev_point = *i;
      }
      const Voxel v(*last_point, (*last_point - prev_point).normalise() * DIRECTION_INTEGER_MULTIPLIER);
      if (v.test_bounds(H))
        out->push_back(v);
    }


};







class MapWriter
{
  public:
    MapWriter (Queue2& queue, const Image::Header& header, const float fraction_scaling_factor) :
      reader (queue),
      H (header),
      buffer (NULL),
      dir_buffer (NULL),
      interp (H),
      scale (fraction_scaling_factor)
    {
    }

    ~MapWriter ()
    {
      if (buffer) {
        Image::Voxel<float> vox (H);
        DataSet::LoopInOrder loop (vox, "writing image to file...", 0, 3);
        for (loop.start (vox, *buffer); loop.ok(); loop.next (vox, *buffer))
          vox.value() = scale * buffer->value();
        delete buffer; buffer = NULL;
      }
      if (dir_buffer) {
        scale /= (float)DIRECTION_INTEGER_MULTIPLIER;
        Image::Voxel<float> vox (H);
        DataSet::LoopInOrder loop (vox, "writing image to file...", 0, 3);
        for (loop.start (vox, *dir_buffer); loop.ok(); loop.next (vox, *dir_buffer)) {
          const Xyz temp = dir_buffer->value();
          vox[3] = 0; vox.value() = scale * temp.x;
          vox[3] = 1; vox.value() = scale * temp.y;
          vox[3] = 2; vox.value() = scale * temp.z;
        }
        delete dir_buffer; dir_buffer = NULL;
      }
    }


    void execute ()
    {
      Queue2::Reader::Item item (reader);

      if (H.axes.ndim() == 3) {

        buffer = new DataSet::Buffer<size_t>(H, 3, "buffer");
        while (item.read()) {
          for (vector<Voxel>::const_iterator i = item->begin(); i != item->end(); ++i) {
            if (i->x >= 0 && i->x < (size_t)H.dim(0) && i->y >= 0 && i->y < (size_t)H.dim(1) && i->z >= 0 && i->z < (size_t)H.dim(2)) {
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
      else {

        dir_buffer = new DataSet::Buffer<Xyz>(H, 3, "dir_buffer");
        while (item.read()) {
          for (vector<Voxel>::const_iterator i = item->begin(); i != item->end(); ++i) {
            if (i->x >= 0 && i->x < (size_t)H.dim(0) && i->y >= 0 && i->y < (size_t)H.dim(1) && i->z >= 0 && i->z < (size_t)H.dim(2)) {
              (*dir_buffer)[0] = i->x;
              (*dir_buffer)[1] = i->y;
              (*dir_buffer)[2] = i->z;
              dir_buffer->value() += Xyz(i->dx, i->dy, i->dz);
            } else {
              debug("Mapping voxel outside image bounds: [" + str(i->x) + " " + str(i->y) + " " + str(i->z) + "]");
            }
          }
        }

      }

    }


   private:
    Queue2::Reader reader;
    const Image::Header& H;
    DataSet::Buffer<size_t>* buffer;
    DataSet::Buffer<Xyz>* dir_buffer;
    DataSet::Interp::Linear<const Image::Header> interp;
    float scale;
};




void generate_header(Image::Header& header, const char* path, const float voxel_size) {

  ProgressBar progress ("creating new template image...", 0);

  Tractography::Reader file;
  Tractography::Properties properties;

  vector<Point> tck;
  size_t track_counter = 0;

  Point min_values( INFINITY,  INFINITY,  INFINITY);
  Point max_values(-INFINITY, -INFINITY, -INFINITY);

  file.open(path, properties);
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
  file.close();

  min_values -= Point(3*voxel_size, 3*voxel_size, 3*voxel_size);
  max_values += Point(3*voxel_size, 3*voxel_size, 3*voxel_size);

  Point image_dim(max_values - min_values);
  image_dim *= (1.0 / voxel_size);
  image_dim = Point(Math::ceil(image_dim[0]), Math::ceil(image_dim[1]), Math::ceil(image_dim[2]));

  header.name() = "tckmap_image_header";
  header.axes.ndim() = 3;

  for (size_t i = 0; i != 3; ++i) {
    header.axes.dim(i)   = round(image_dim[i]);
    header.axes.vox(i)   = voxel_size;
    header.axes.units(i) = Image::Axes::millimeters;
  }

  header.axes.description(0) = Image::Axes::left_to_right;
  header.axes.description(1) = Image::Axes::posterior_to_anterior;
  header.axes.description(2) = Image::Axes::inferior_to_superior;

  header.axes.stride(0) = 1;
  header.axes.stride(1) = round(image_dim[0]);
  header.axes.stride(2) = round(image_dim[0]) * round(image_dim[1]);

  Math::Matrix<float> t_matrix(4, 4);
  t_matrix(0, 0) = 1.0; t_matrix(0, 1) = 0.0; t_matrix(0, 2) = 0.0; t_matrix(0, 3) = min_values[0];
  t_matrix(1, 0) = 0.0; t_matrix(1, 1) = 1.0; t_matrix(1, 2) = 0.0; t_matrix(1, 3) = min_values[1];
  t_matrix(2, 0) = 0.0; t_matrix(2, 1) = 0.0; t_matrix(2, 2) = 1.0; t_matrix(2, 3) = min_values[2];
  t_matrix(3, 0) = 0.0; t_matrix(3, 1) = 0.0; t_matrix(3, 2) = 0.0; t_matrix(3, 3) = 1.0;
  header.transform() = t_matrix;

}



void oversample_header (Image::Header& header, const float os_ratio) 
{
  if (os_ratio == 1.0)
    return;

  for (size_t i = 0; i != 3; ++i) {
    header.axes.vox(i) /= os_ratio;
    header.axes.dim(i) *= os_ratio;
  }

  header.comments.push_back (std::string (("image_oversampling: ") + str(os_ratio)));
}




EXECUTE {

  Tractography::Properties properties;
  Tractography::Reader file;
  file.open (argument[0].get_string(), properties);
  file.close();

  const size_t num_tracks  = properties["count"]      .empty() ? 0   : to<size_t> (properties["count"]);
  const float  step_size   = properties["step_size"]  .empty() ? 0.0 : to<float>  (properties["step_size"]);

  const bool  colour         = get_options("colour")  .size();
  const bool  fibre_fraction = get_options("fraction").size();
  const bool  template_image = get_options("template").size();
  const float voxel_size     = get_options("vox")     .size() ? get_options("vox")[0][0].get_float() : 0.0;

  if (!template_image && !voxel_size)
    throw Exception ("You must provide either a template image, or a desired voxel size, for the output image!");

  Image::Header header;
  if (template_image) {
    header = get_options("template")[0][0].get_image();
    oversample_header(header, get_options("template")[0][1].get_float());
  } 
  else 
    generate_header(header, argument[0].get_string(), voxel_size);

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
  }
  else {
    header.axes.ndim() = 3;
    header.comments.push_back (std::string (("track ") + str(fibre_fraction ? "fraction" : "count") + " map"));
  }

  vector<OptBase> opt = get_options ("datatype");
  if (opt.size())
    header.datatype().parse (DataType::identifiers[opt[0][0].get_int()]);
  else
    header.datatype() = (fibre_fraction || colour) ? DataType::Float32 : DataType::UInt32;

  unsigned int resample_factor;
  opt = get_options("resample");
  if (opt.size()) {
    resample_factor = opt[0][0].get_int();
    info ("Track interpolation factor manually set to " + str(resample_factor));
  }
  else if (step_size) {
    const float min_vox = minvalue(header.vox(0), header.vox(1), header.vox(2));
    resample_factor = Math::ceil(step_size / min_vox);
    info ("Track interpolation factor automatically set to " + str(resample_factor));
    properties["step_size"] = str(step_size / resample_factor);
  } 
  else {
    resample_factor = 1;
    info ("Track interpolation off; no track step size information in header");
  }

  // TODO Enable segmentwise processing in scenario where not enough memory is available for both the buffer and the memory-mapped output file

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

  const Image::Header header_out = argument[1].get_image (header);

  Gen_interp_matrix<float> coef_matrix(resample_factor);

  Queue1 queue1 ("loaded tracks");
  Queue2 queue2 ("processed tracks");

  file.open (argument[0].get_string(), properties);

  TrackLoader loader (queue1, file, num_tracks);
  TrackMapper mapper (queue1, queue2, header_out, coef_matrix.get_matrix());
  MapWriter writer (queue2, header_out, scaling_factor);

  Thread::Exec loader_thread (loader, "loader");
  Thread::Array<TrackMapper> mapper_list (mapper);
  Thread::Exec mapper_threads (mapper_list, "mapper");

  writer.execute();

}

