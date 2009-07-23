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


    29-08-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * in create(), finalise the byte order after the handler's check() method 
      to allow different file formats to override the data type more easily.

    02-09-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * update temporary file handling (i.e. those sent via pipes)
    - switch to MRtrix format as the standard format for temporary files
    - handle any type of image supplied through the standard input

    01-10-2008 J-Donald Tournier <d.tournier@brain.org.au>
    * sanitise axes prior to creating an image 
*/

#include "app.h"
#include "file/path.h"
#include "image/object.h"
#include "image/misc.h"
#include "image/format/list.h"
#include "image/name_parser.h"

namespace MR {
  namespace Image {
    namespace {

      void merge (Header& D, const Header& H)
      {
        if (D.data_type != H.data_type) 
          throw Exception ("data types differ between image files for \"" + D.name + "\"");

        if (D.offset != H.offset || D.scale != H.scale) 
          throw Exception ("scaling coefficients differ between image files for \"" + D.name + "\"");

        if (D.axes.size() != H.axes.size()) 
          throw Exception ("dimension mismatch between image files for \"" + D.name + "\"");

        for (size_t n = 0; n < D.axes.size(); n++) {
          if (D.axes[n].dim != H.axes[n].dim) 
            throw Exception ("dimension mismatch between image files for \"" + D.name + "\"");

          if (D.axes[n].order != H.axes[n].order || D.axes[n].forward != H.axes[n].forward)
            throw Exception ("data layout differs image files for \"" + D.name + "\"");

          if (D.axes[n].vox != H.axes[n].vox)
            error ("WARNING: voxel dimensions differ between image files for \"" + D.name + "\"");
        }


        for (std::vector<std::string>::const_iterator item = H.comments.begin(); item != H.comments.end(); item++)
          if (find (D.comments.begin(), D.comments.end(), *item) == D.comments.end())
            D.comments.push_back (*item);

        if (!D.transform_matrix.is_set() && H.transform_matrix.is_set()) D.transform_matrix = H.transform_matrix;
        if (!D.DW_scheme.is_set() && H.DW_scheme.is_set()) D.DW_scheme = H.DW_scheme; 
      }


    }


    void Object::open (const std::string& imagename, bool is_read_only)
    {
      M.reset();
      H.read_only = is_read_only;

      if (imagename == "-") getline (std::cin, H.name);
      else H.name = imagename;

      if (H.name.empty()) throw Exception ("no name supplied to open image!");

      info ("opening image \"" + H.name + "\"...");

      ParsedNameList list;
      std::vector<int> num = list.parse_scan_check (H.name);

      try { 
        const Format::Base** handler = handlers;
        std::vector< RefPtr<ParsedName> >::iterator item = list.begin();
        Header header;
        header.name = (*item)->name();

        for (; *handler; handler++) if ((*handler)->read (M, header)) break;
        if (!*handler) throw Exception ("unknown format for image \"" + header.name + "\"");

        std::string old_name (H.name);
        H = header;
        if (header.name == (*item)->name()) H.name = old_name;

        while (++item != list.end()) {
          header.name = (*item)->name();
          if (!(*handler)->read (M, header)) throw Exception ("image specifier contains mixed format files");
          merge (H, header);
        }
      }
      catch (...) { throw Exception ("error opening image \"" + H.name + "\""); }

      if (num.size()) {
        int a = 0, n = 0;
        for (size_t i = 0; i < H.axes.size(); i++) if (H.axes[i].order != Axis::undefined) n++;
        H.axes.resize (n + num.size());

        for (std::vector<int>::const_iterator item = num.begin(); item != num.end(); item++) {
          while (H.axes[a].order != Order::undefined) a++;
          H.axes[a].dim = *item;
          H.axes[a].order = n++;
        }
      }

      if (Path::is_temporary (H.name)) M.set_temporary (true);
      setup();
    }





    void Object::create (const std::string& imagename, Image::Header&template_header)
    {
      M.reset();
      H = template_header;
      H.read_only = false;
      H.axes.sanitise();

      if (imagename.empty()) {

        H.name = "scratch image";
        try { M.add (new uint8_t [memory_footprint(H.data_type, voxel_count (H.axes))]); }
        catch (...) { throw Exception ("failed to allocate memory for scratch image data!"); }

      }
      else {

        if (imagename == "-") {
          File::MMap fmap ("", 1024, "mif");
          H.name = fmap.name();
        }
        else H.name = imagename;

        info ("creating image \"" + name() + "\"...");

        NameParser parser;
        parser.parse (H.name);
        std::vector<int> dim (parser.ndim());
        const Format::Base** handler = handlers;

        Axes axes = H.axes;

        try {
          for (; *handler; handler++) 
            if ((*handler)->check (H, H.axes.ndim() - dim.size())) break;
          if (!*handler) throw Exception ("unknown format for image \"" + H.name + "\"");
        }
        catch (...) { throw Exception ("error creating image \"" + H.name + "\""); }

        H.data_type.set_byte_order_native();
        int a = 0;
        for (size_t n = 0; n < dim.size(); n++) {
          while (H.axes[a].order != Axis::undefined) a++;
          dim[n] = axes[a].dim;
        }
        parser.calculate_padding (dim);

        try { 
          std::vector<int> num (dim.size());
          do {
            H.name = parser.name (num);
            (*handler)->create (M, H);
          } while (get_next (num, dim));
        }
        catch (...) { throw Exception ("error creating image \"" + H.name + "\""); }

        if (dim.size()) {
          int a = 0, n = 0;
          for (size_t i = 0; i < H.axes.size(); i++) if (H.axes[i].order != Axis::undefined) n++;
          H.axes.resize (n + dim.size());

          for (std::vector<int>::const_iterator item = dim.begin(); item != dim.end(); item++) {
            while (H.axes[a].order != Axis::undefined) a++;
            H.axes[a].dim = *item;
            H.axes[a].order = n++;
          }
        }

        if (Path::is_temporary (H.name)) M.output_name = H.name;
      }

      setup();
    }







    void Object::concatenate (std::vector<RefPtr<Object> >& images)
    {
      M.reset();
      if (!images.front() || ! images.back()) throw Exception ("cannot concatenate images: some images are NULL");
      debug ("concatenating images \"" + images.front()->name() + " -> " + images.back()->name() + "\"...");
      M.optimised = false;

      Image::Object& ref (*images[0]);
      for (std::vector<RefPtr<Object> >::iterator it = images.begin()+1; it != images.end(); ++it) {
        if (!*it) throw Exception ("cannot concatenate images: some images are NULL");
        Image::Object& ima (**it);
        if (ima.ndim() != ref.ndim()) throw Exception ("cannot concatenate images: number of dimensions do not match");
        if (ima.header().data_type != ref.header().data_type) throw Exception ("cannot concatenate images: data types do not match");
        for (size_t n = 0; n < ref.ndim(); n++) {
          if (ima.header().axes[n].dim != ref.header().axes[n].dim) throw Exception ("cannot concatenate images: dimensions do not match");
          if (ima.header().axes[n].order != ref.header().axes[n].order || 
              ima.header().axes[n].forward != ref.header().axes[n].forward) throw Exception ("cannot concatenate images: data layouts do not match");
        }
        if (ima.M.list.size() != ref.M.list.size()) throw Exception ("cannot concatenate images: number of files do not match");
      }

      H = ref.header();
      H.name = "{ " + images.front()->name() + " -> " + images.back()->name() + " }";
      H.axes.resize (ref.ndim()+1);
      H.axes[ref.ndim()].dim = images.size();
      M.optimised = false;
      M.temporary = false;
      M.set_data_type (H.data_type);

      for (std::vector<RefPtr<Object> >::iterator it = images.begin(); it != images.end(); ++it) 
        for (std::vector<Mapper::Entry>::iterator f = (*it)->M.list.begin(); f != (*it)->M.list.end(); ++f) 
          M.add (f->fmap, f->offset);


      start = ref.start;
      memcpy (stride, ref.stride, MRTRIX_MAX_NDIMS*sizeof(ssize_t));
      stride[ref.ndim()] = voxel_count(ref);
      if (H.data_type.is_complex()) stride[ref.ndim()] *= 2; 

      if (App::log_level > 2) {
        std::string string ("data increments initialised with start = " + str (start) + ", stride = [ ");
        for (size_t i = 0; i < ndim(); i++) string += str (stride[i]) + " "; 
        debug (string + "]");
      }
    }







    void Object::setup ()
    {
      if (H.name == "-") H.name = M.list[0].fmap.name();

      debug ("setting up image \"" + H.name + "\"...");
      M.optimised = false;

      set_temporary (M.temporary);

      M.set_read_only (H.read_only);
      M.set_data_type (H.data_type);
      H.sanitise();

      if (M.list.size() == 1 && H.data_type == DataType::Native) M.optimised = true;

      debug ("setting up data increments for \"" + H.name + "\"...");

      start = 0;
      memset (stride, 0, MRTRIX_MAX_NDIMS*sizeof(ssize_t));

      uint order[ndim()];
      uint last = ndim()-1;
      for (size_t i = 0; i < ndim(); i++) {
        if (H.axes[i].order != Axis::undefined) order[H.axes[i].order] = i; 
        else order[last--] = i;
      }

      size_t axis;
      ssize_t mult = 1;

      for (size_t i = 0; i < ndim(); i++) {
        axis = order[i];
        assert (axis < ndim());
        if (stride[axis]) throw Exception ("invalid data order specifier for image \"" + H.name + "\": same dimension specified twice");

        stride[axis] = mult * ssize_t(H.axes[axis].direction());
        if (stride[axis] < 0) start += size_t(-stride[axis]) * size_t(H.axes[axis].dim-1);
        mult *= ssize_t(H.axes[axis].dim);
      }

      if (H.data_type.is_complex()) {
        start *= 2;
        for (size_t i = 0; i < ndim(); i++) stride[i] *= 2;
      }


      if (App::log_level > 2) {
        std::string string ("data increments initialised with start = " + str (start) + ", stride = [ ");
        for (size_t i = 0; i < ndim(); i++) string += str (stride[i]) + " "; 
        debug (string + "]");
      }
    }




    std::ostream& operator<< (std::ostream& stream, const Object& obj)
    {
      stream << "Image object: \"" << obj.name() << "\" [ ";
      for (size_t n = 0; n < obj.ndim(); n++) stream << obj.dim(n) << " ";
      stream << "]\n Offset: start = " << obj.start << ", stride = [ ";
      for (size_t n = 0; n < obj.ndim(); n++) stream << obj.stride[n] << " ";
      stream << "]\nHeader:\n" << obj.H << obj.M;
      return (stream);
    }


  }
}
