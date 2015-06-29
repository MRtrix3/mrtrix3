/*
   Copyright 2011 Brain Research Institute, Melbourne, Australia

   Written by Robert E. Smith, 2011.

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

#ifndef __dwi_tractography_mapping_writer_h__
#define __dwi_tractography_mapping_writer_h__

#include "memory.h"
#include "file/path.h"
#include "file/utils.h"
#include "image.h"
#include "algo/loop.h"
#include "thread_queue.h"

#include "dwi/tractography/mapping/twi_stats.h"
#include "dwi/tractography/mapping/voxel.h"
#include "dwi/tractography/mapping/gaussian/voxel.h"



#include <typeinfo>


namespace MR {
  namespace DWI {
    namespace Tractography {
      namespace Mapping {



        enum writer_dim { UNDEFINED, GREYSCALE, DEC, DIXEL, TOD };
        extern const char* writer_dims[];



        class MapWriterBase
        {

          protected:
            // counts needs to be floating-point to cover possibility of weighted streamlines
            //typedef Image::BufferScratch<float> counts_buffer_type;
            //typedef Image::BufferScratch<float>::voxel_type counts_voxel_type;

          public:
            MapWriterBase (Header& header, const std::string& name, const vox_stat_t s = V_SUM, const writer_dim t = GREYSCALE) :
              H (header),
              output_image_name (name),
              direct_dump (false),
              voxel_statistic (s),
              type (t)
          {
            assert (type != UNDEFINED);
          }

            MapWriterBase (const MapWriterBase&) = delete;

            // can't do this in destructor since it could potentially throw,
            // and throwing in destructor is most uncool (invokes
            // std::terminate() with no further ado).
            virtual void finalise() { }



            virtual bool operator() (const SetVoxel&)    { return false; }
            virtual bool operator() (const SetVoxelDEC&) { return false; }
            virtual bool operator() (const SetDixel&)    { return false; }
            virtual bool operator() (const SetVoxelTOD&) { return false; }

            virtual bool operator() (const Gaussian::SetVoxel&)    { return false; }
            virtual bool operator() (const Gaussian::SetVoxelDEC&) { return false; }
            virtual bool operator() (const Gaussian::SetDixel&)    { return false; }
            virtual bool operator() (const Gaussian::SetVoxelTOD&) { return false; }


          protected:
            Header& H;
            const std::string output_image_name;
            bool direct_dump;
            const vox_stat_t voxel_statistic;
            const writer_dim type;

            // This gets used with mean voxel statistic for some (but not all) writers,
            //   or if the output is a voxel_summed DEC image.
            // It's also hijacked to store per-voxel min/max factors in the case of TOD
            std::unique_ptr<Image<float>> counts;
            //std::unique_ptr<counts_voxel_type > v_counts;

        };






        template <typename value_type>
          class MapWriter : public MapWriterBase
        {

          //typedef typename Image::Buffer<value_type> image_type;
          //typedef typename Image::Buffer<value_type>::voxel_type image_voxel_type;
          //typedef typename Mapping::BufferScratchDump<value_type> buffer_type;
          //typedef typename Mapping::BufferScratchDump<value_type>::voxel_type buffer_voxel_type;

          public:
          MapWriter (Header& header, const std::string& name, const vox_stat_t voxel_statistic = V_SUM, const writer_dim type = GREYSCALE) :
            MapWriterBase (header, name, voxel_statistic, type),
            buffer (Image<value_type>::scratch (header, "TWI " + str(writer_dims[type]) + " buffer"))
            //v_buffer (buffer)
          {
            LoopInOrder loop (buffer);
            if (type == DEC || type == TOD) {

              if (voxel_statistic == V_MIN) {
                for (auto l = loop (buffer); l; ++l )
                  buffer.value() = std::numeric_limits<value_type>::max();
              }
/* shouldn't be needed: scratch IO class memset to zero already:
              else {
                buffer.zero();
              } */

            } else { // Greyscale and dixel

              if (voxel_statistic == V_MIN) {
                for (auto l = loop (buffer); l; ++l )
                  buffer.value() = std::numeric_limits<value_type>::max();
              } else if (voxel_statistic == V_MAX) {
                for (auto l = loop (buffer); l; ++l )
                  buffer.value() = std::numeric_limits<value_type>::lowest();
              } 
/* shouldn't be needed: scratch IO class memset to zero already:
              else {
                buffer.zero();
              }*/

            }

            // With TOD, hijack the counts buffer in voxel statistic min/max mode
            //   (use to store maximum / minimum factors and hence decide when to update the TOD)
            if ((voxel_statistic == V_MEAN) ||
                (type == TOD && (voxel_statistic == V_MIN || voxel_statistic == V_MAX)) ||
                (type == DEC && voxel_statistic == V_SUM))
            {
              Header H_counts (header);
              if (type == DEC || type == TOD) 
                H_counts.set_ndim (3);
              counts.reset (new Image<float> (Image<float>::scratch (H_counts, "TWI streamline count buffer")));
            }
          }

          MapWriter (const MapWriter&) = delete;

          void finalise () {

            LoopInOrder loop (buffer, 0, 3);
            switch (voxel_statistic) {

              case V_SUM:
                if (type == DEC) {
                  for (auto l = loop (buffer, *counts); l; ++l) {
                    if (counts->value()) {
                      auto value = get_dec();
                      value *= counts->value() / value.norm();
                      set_dec (value);
                    }
                  }
                }
                break;

              case V_MIN:
                for (auto l = loop (buffer); l; ++l ) {
                  if (buffer.value() == std::numeric_limits<value_type>::max())
                    buffer.value() = value_type(0);
                }
                break;

              case V_MEAN:
                if (type == GREYSCALE) {
                  for (auto l = loop (buffer, *counts); l; ++l) {
                    if (counts->value())
                      buffer.value() /= value_type(counts->value());
                  }
                } 
                else if (type == DEC) {
                  for (auto l = loop (buffer, *counts); l; ++l) {
                    auto value = get_dec();
                    if (value.squaredNorm()) {
                      value /= counts->value();
                      set_dec (value);
                    }
                  }
                } 
                else if (type == TOD) {
                  for (auto l = loop (buffer, *counts); l; ++l) {
                    if (counts->value()) {
                      Eigen::VectorXf value;
                      get_tod (value);
                      value *= (1.0 / counts->value());
                      set_tod (value);
                    }
                  }
                } else { // Dixel
                  // TODO For dixels, should this be a voxel mean i.e. normalise each non-zero voxel to unit density,
                  //   rather than a per-dixel mean?
                  LoopInOrder loop_dixel (buffer);
                  for (auto l = loop_dixel (buffer, *counts); l; ++l) {
                    if (counts->value())
                      buffer.value() /= float(counts->value());
                  }
                }
                break;

              case V_MAX:
                if (type == GREYSCALE) {
                  for (auto l = loop (buffer); l; ++l) {
                    if (buffer.value() == -std::numeric_limits<value_type>::max())
                      buffer.value() = value_type(0);
                  }
                } else if (type == DIXEL) {
                  LoopInOrder loop_dixel (buffer);
                  for (auto l = loop_dixel (buffer); l; ++l) {
                    if (buffer.value() == -std::numeric_limits<value_type>::max())
                      buffer.value() = value_type(0);
                  }
                }
                break;

              default:
                throw Exception ("Unknown / unhandled voxel statistic in ~MapWriter()");

            }

            save (buffer, output_image_name);
            
            /* TODO: pretty sure none of this is needed any longer, but need
             * to check...

            if (direct_dump) {

              if (App::log_level)
                std::cerr << App::NAME << ": dumping image contents to file... ";
              buffer.dump_to_file (output_image_name, H);
              if (App::log_level)
                std::cerr << "done.\n";

            } else {

              image_type out (output_image_name, H);
              image_voxel_type v_out (out);
              if (type == DEC) {
                Image::LoopInOrder loop_out (v_out, "writing image to file...", 0, 3);
                for (auto l = loop_out (v_out, v_buffer); l; ++l) {
                  Point<value_type> value (get_dec());
                  v_out[3] = 0; v_out.value() = value[0];
                  v_out[3] = 1; v_out.value() = value[1];
                  v_out[3] = 2; v_out.value() = value[2];
                }
              } else if (type == TOD) {
                Image::LoopInOrder loop_out (v_out, "writing image to file...", 0, 3);
                for (auto l = loop_out (v_out, v_buffer); l; ++l) {
                  Math::Vector<float> value;
                  get_tod (value);
                  for (v_out[3] = 0; v_out[3] != v_out.dim(3); ++v_out[3])
                    v_out.value() = value[size_t(v_out[3])];
                }
              } else { // Greyscale and Dixel
                Image::LoopInOrder loop_out (v_out, "writing image to file...");
                for (auto l = loop_out (v_out, v_buffer); l; ++l) 
                  v_out.value() = v_buffer.value();
              }

            } */
          }


          bool operator() (const SetVoxel& in)    { receive_greyscale (in); return true; }
          bool operator() (const SetVoxelDEC& in) { receive_dec       (in); return true; }
          bool operator() (const SetDixel& in)    { receive_dixel     (in); return true; }
          bool operator() (const SetVoxelTOD& in) { receive_tod       (in); return true; }

          bool operator() (const Gaussian::SetVoxel& in)    { receive_greyscale (in); return true; }
          bool operator() (const Gaussian::SetVoxelDEC& in) { receive_dec       (in); return true; }
          bool operator() (const Gaussian::SetDixel& in)    { receive_dixel     (in); return true; }
          bool operator() (const Gaussian::SetVoxelTOD& in) { receive_tod       (in); return true; }


          private:
          Image<value_type> buffer;

          // Template functions used so that the functors don't have to be written twice
          //   (once for standard TWI and one for Gaussian track-wise statistic)
          template <class Cont> void receive_greyscale (const Cont&);
          template <class Cont> void receive_dec       (const Cont&);
          template <class Cont> void receive_dixel     (const Cont&);
          template <class Cont> void receive_tod       (const Cont&);

          // These acquire the TWI factor at any point along the streamline;
          //   For the standard SetVoxel classes, this is a single value 'factor' for the set as
          //     stored in SetVoxelExtras
          //   For the Gaussian SetVoxel classes, there is a factor per mapped element
          float get_factor (const Voxel&    element, const SetVoxel&    set) const { return set.factor; }
          float get_factor (const VoxelDEC& element, const SetVoxelDEC& set) const { return set.factor; }
          float get_factor (const Dixel&    element, const SetDixel&    set) const { return set.factor; }
          float get_factor (const VoxelTOD& element, const SetVoxelTOD& set) const { return set.factor; }
          float get_factor (const Gaussian::Voxel&    element, const Gaussian::SetVoxel&    set) const { return element.get_factor(); }
          float get_factor (const Gaussian::VoxelDEC& element, const Gaussian::SetVoxelDEC& set) const { return element.get_factor(); }
          float get_factor (const Gaussian::Dixel&    element, const Gaussian::SetDixel&    set) const { return element.get_factor(); }
          float get_factor (const Gaussian::VoxelTOD& element, const Gaussian::SetVoxelTOD& set) const { return element.get_factor(); }


          // Convenience functions for Directionally-Encoded Colour processing
          Eigen::Vector3f get_dec ();
          void            set_dec (const Eigen::Vector3f&);

          // Convenience functions for Track Orientation Distribution processing
          void get_tod (      Eigen::VectorXf&);
          void set_tod (const Eigen::VectorXf&);

        };







        template <typename value_type>
          template <class Cont>
          void MapWriter<value_type>::receive_greyscale (const Cont& in)
          {
            assert (MapWriterBase::type == GREYSCALE);
            for (const auto& i : in) { 
              assign_pos_of (i).to (buffer);
              const float factor = get_factor (i, in);
              const float weight = in.weight * i.get_length();
              switch (voxel_statistic) {
                case V_SUM:  buffer.value() += weight * factor;              break;
                case V_MIN:  buffer.value() = MIN(buffer.value(), factor); break;
                case V_MAX:  buffer.value() = MAX(buffer.value(), factor); break;
                case V_MEAN:
                             buffer.value() += weight * factor;
                             assert (counts);
                             assign_pos_of (i).to (*counts);
                             counts->value() += weight;
                             break;
                default:
                             throw Exception ("Unknown / unhandled voxel statistic in MapWriter::receive_greyscale()");
              }
            }
          }



        template <typename value_type>
          template <class Cont>
          void MapWriter<value_type>::receive_dec (const Cont& in)
          {
            assert (type == DEC);
            for (const auto& i : in) { 
              assign_pos_of (i).to (buffer);
              const float factor = get_factor (i, in);
              const float weight = in.weight * i.get_length();
              auto scaled_colour = i.get_colour();
              scaled_colour *= factor;
              const auto current_value = get_dec();
              switch (voxel_statistic) {
                case V_SUM:
                  set_dec (current_value + (scaled_colour * weight));
                  assert (counts);
                  assign_pos_of (i).to (*counts);
                  counts->value() += weight;
                  break;
                case V_MIN:
                  if (scaled_colour.squaredNorm() < current_value.squaredNorm())
                    set_dec (scaled_colour);
                  break;
                case V_MEAN:
                  set_dec (current_value + (scaled_colour * weight));
                  assign_pos_of (i).to (*counts);
                  counts->value() += weight;
                  break;
                case V_MAX:
                  if (scaled_colour.squaredNorm() > current_value.squaredNorm())
                    set_dec (scaled_colour);
                  break;
                default:
                  throw Exception ("Unknown / unhandled voxel statistic in MapWriter::receive_dec()");
              }
            }
          }



        template <typename value_type>
          template <class Cont>
          void MapWriter<value_type>::receive_dixel (const Cont& in)
          {
            assert (type == DIXEL);
            for (const auto& i : in) { 
              assign_pos_of (i, 0, 3).to (buffer);
              buffer.index(3) = i.get_dir();
              const float factor = get_factor (i, in);
              const float weight = in.weight * i.get_length();
              switch (voxel_statistic) {
                case V_SUM:  buffer.value() += weight * factor;              break;
                case V_MIN:  buffer.value() = MIN(buffer.value(), factor); break;
                case V_MAX:  buffer.value() = MAX(buffer.value(), factor); break;
                case V_MEAN:
                             buffer.value() += weight * factor;
                             assert (counts);
                             assign_pos_of (i, 0, 3).to (*counts);
                             counts->index(3) = i.get_dir();
                             counts->value() += weight;
                             break;
                default:
                             throw Exception ("Unknown / unhandled voxel statistic in MapWriter::receive_dixel()");
              }
            }
          }



        template <typename value_type>
          template <class Cont>
          void MapWriter<value_type>::receive_tod (const Cont& in)
          {
            assert (type == TOD);
            Eigen::VectorXf sh_coefs;
            for (const auto& i : in) { 
              assign_pos_of (i, 0, 3).to (buffer);
              const float factor = get_factor (i, in);
              const float weight = in.weight * i.get_length();
              get_tod (sh_coefs);
              if (counts)
                assign_pos_of (i, 0, 3).to (*counts);
              switch (voxel_statistic) {
                case V_SUM:
                  for (ssize_t index = 0; index != sh_coefs.size(); ++index)
                    sh_coefs[index] += i.get_tod()[index] * weight * factor;
                  set_tod (sh_coefs);
                  break;
                  // For TOD, need to store min/max factors - counts buffer is hijacked to do this
                case V_MIN:
                  assert (counts);
                  if (factor < counts->value()) {
                    counts->value() = factor;
                    auto tod = i.get_tod();
                    tod *= factor;
                    set_tod (tod);
                  }
                  break;
                case V_MAX:
                  assert (counts);
                  if (factor > counts->value()) {
                    counts->value() = factor;
                    auto tod = i.get_tod();
                    tod *= factor;
                    set_tod (tod);
                  }
                  break;
                case V_MEAN:
                  assert (counts);
                  for (ssize_t index = 0; index != sh_coefs.size(); ++index)
                    sh_coefs[index] += i.get_tod()[index] * weight * factor;
                  set_tod (sh_coefs);
                  counts->value() += weight;
                  break;
                default:
                  throw Exception ("Unknown / unhandled voxel statistic in MapWriter::receive_tod()");
              }
            }
          }





        template <typename value_type>
          Eigen::Vector3f MapWriter<value_type>::get_dec ()
          {
            assert (type == DEC);
            Eigen::Vector3f value;
            buffer.index(3) = 0; value[0] = buffer.value();
            ++buffer.index(3);   value[1] = buffer.value();
            ++buffer.index(3);   value[2] = buffer.value();
            return value;
          }

        template <typename value_type>
          void MapWriter<value_type>::set_dec (const Eigen::Vector3f& value)
          {
            assert (type == DEC);
            buffer.index(3) = 0; buffer.value() = value[0];
            ++buffer.index(3);   buffer.value() = value[1];
            ++buffer.index(3);   buffer.value() = value[2];
          }





        template <typename value_type>
          void MapWriter<value_type>::get_tod (Eigen::VectorXf& sh_coefs)
          {
            assert (type == TOD);
            sh_coefs.resize (buffer.size(3));
            for (auto l = Loop (3) (buffer); l; ++l) 
              sh_coefs[buffer.index(3)] = buffer.value();
          }

        template <typename value_type>
          void MapWriter<value_type>::set_tod (const Eigen::VectorXf& sh_coefs)
          {
            assert (type == TOD);
            assert (sh_coefs.size() == buffer.size(3));
            for (auto l = Loop (3) (buffer); l; ++l) 
              buffer.value() = sh_coefs[buffer.index(3)];
          }





      }
    }
  }
}

#endif



