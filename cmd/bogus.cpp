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

#include "app.h"
#include "debug.h"
#include "image/voxel.h"
#include "thread/iterator.h"
#include "thread/exec.h"
#include "ptr.h"

using namespace MR;

SET_VERSION_DEFAULT;
SET_AUTHOR (NULL);
SET_COPYRIGHT (NULL);

DESCRIPTION = {
  "this is used to test stuff.",
  NULL
};

ARGUMENTS = {
 Argument ("input", "input").type_image_in(),
 Argument ("output", "output").type_image_out(),
 Argument() 
};

OPTIONS = { Option() };





typedef float T;

/*
class Processor {
  public:
    Processor (Thread::Iterator<DataSet::Loop>& nextvoxel, Image::Header& input, Image::Header& output) :
      next (nextvoxel), in (input), out (output) { }

    void execute () {
      DataSet::Loop loop (1,3);
      while (next (in, out)) {
        for (loop.start (in, out); loop.ok(); loop.next (in, out))
          out.value() = Math::exp (-0.01*in.value());
      }
    }

  private:
    Thread::Iterator<DataSet::Loop>& next;
    Image::Voxel<T> in, out;
};

*/
class S {
  public:
    int i;
    float f;
    std::string txt;
};

EXECUTE {
  Ptr<T> p (new T (10.2));
  VAR (*p);
  VAR (p);

  Ptr<T> a (new T (5.2));
  VAR (*a);
  VAR (a);

  a = new T (1.5);
  VAR (*a);
  VAR (a);

  VAR (a == p);

  p = a;
  VAR (*a);
  VAR (a);
  VAR (*p);
  VAR (p);
  VAR (a == p);
  VAR (a != p);

  VAR (a[0]);

  Ptr<S> s (new S);
  VAR (s);
  VAR (s->i);

  if (s) VAR ("s set");
  if (!s) VAR ("s not set");

  s = NULL;

  if (s) VAR ("s set");
  if (!s) VAR ("s not set");

/*
  Image::Header in (argument[0]);
  Image::Header out (in);
  out.set_datatype (DataType::Float32);
  out.create (argument[1]);

  DataSet::Loop loop ("processing...", 0, 1);
  Thread::Iterator<DataSet::Loop> next (loop, in);

  Processor processor (next, in, out);
  Thread::Array<Processor> array (processor);
  Thread::Exec threads (array);
  */
}

