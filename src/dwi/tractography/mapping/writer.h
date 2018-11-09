/*
 * Copyright (c) 2008-2018 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/
 *
 * MRtrix3 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/
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
        { MEMALIGN(MapWriterBase)

          public:
            MapWriterBase (const Header& header, const std::string& name, const vox_stat_t s = V_SUM, const writer_dim t = GREYSCALE) :
              H (header),
              output_image_name (name),
              voxel_statistic (s),
              type (t) {
                assert (type != UNDEFINED);
              }

            MapWriterBase (const MapWriterBase&) = delete;

            virtual ~MapWriterBase () { }

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
            const Header& H;
            const std::string output_image_name;
            const vox_stat_t voxel_statistic;
            const writer_dim type;

            // This gets used with mean voxel statistic for some (but not all) writers,
            //   or if the output is a voxel_summed DEC image.
            // counts needs to be floating-point to cover possibility of weighted streamlines
            // It's also hijacked to store per-voxel min/max factors in the case of TOD
            std::unique_ptr<Image<float>> counts;

        };






        template <typename value_type>
          class MapWriter : public MapWriterBase
        { MEMALIGN(MapWriter<value_type>)

          public:
          MapWriter (const Header& header, const std::string& name, const vox_stat_t voxel_statistic = V_SUM, const writer_dim type = GREYSCALE) :
              MapWriterBase (header, name, voxel_statistic, type),
              buffer (Image<value_type>::scratch (header, "TWI " + str(writer_dims[type]) + " buffer"))
          {
            auto loop = Loop (buffer);
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
            if ((type != DEC && voxel_statistic == V_MEAN) ||
                (type == TOD && (voxel_statistic == V_MIN || voxel_statistic == V_MAX)) ||
                (type == DEC && voxel_statistic == V_SUM))
            {
              Header H_counts (header);
              if (type == DEC || type == TOD)
                H_counts.ndim() = 3;
              counts.reset (new Image<float> (Image<float>::scratch (H_counts, "TWI streamline count buffer")));
            }
          }

          MapWriter (const MapWriter&) = delete;

          void finalise () override {

            auto loop = Loop (buffer, 0, 3);
            switch (voxel_statistic) {

              case V_SUM:
                if (type == DEC) {
                  assert (counts);
                  for (auto l = loop (buffer, *counts); l; ++l) {
                    const float total_weight = counts->value();
                    if (total_weight) {
                      auto value = get_dec();
                      const default_type norm = value.norm();
                      if (norm)
                        value *= total_weight / norm;
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
                  assert (counts);
                  for (auto l = loop (buffer, *counts); l; ++l) {
                    if (counts->value())
                      buffer.value() /= value_type(counts->value());
                  }
                }
                else if (type == DEC) {
                  for (auto l = loop (buffer); l; ++l) {
                    auto value = get_dec();
                    if (value.squaredNorm())
                      set_dec (value.normalized());
                  }
                }
                else if (type == TOD) {
                  assert (counts);
                  for (auto l = loop (buffer, *counts); l; ++l) {
                    if (counts->value()) {
                      VoxelTOD::vector_type value;
                      get_tod (value);
                      value *= (1.0 / counts->value());
                      set_tod (value);
                    }
                  }
                } else { // Dixel
                  assert (counts);
                  // TODO For dixels, should this be a voxel mean i.e. normalise each non-zero voxel to unit density,
                  //   rather than a per-dixel mean?
                  for (auto l = Loop (buffer) (buffer, *counts); l; ++l) {
                    if (counts->value())
                      buffer.value() /= default_type(counts->value());
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
                  for (auto l = Loop (buffer) (buffer); l; ++l) {
                    if (buffer.value() == -std::numeric_limits<value_type>::max())
                      buffer.value() = value_type(0);
                  }
                }
                break;

              default:
                throw Exception ("Unknown / unhandled voxel statistic in MapWriter::finalise()");

            }

            save (buffer, output_image_name);
          }


          bool operator() (const SetVoxel& in)    override { receive_greyscale (in); return true; }
          bool operator() (const SetVoxelDEC& in) override { receive_dec       (in); return true; }
          bool operator() (const SetDixel& in)    override { receive_dixel     (in); return true; }
          bool operator() (const SetVoxelTOD& in) override { receive_tod       (in); return true; }

          bool operator() (const Gaussian::SetVoxel& in)    override { receive_greyscale (in); return true; }
          bool operator() (const Gaussian::SetVoxelDEC& in) override { receive_dec       (in); return true; }
          bool operator() (const Gaussian::SetDixel& in)    override { receive_dixel     (in); return true; }
          bool operator() (const Gaussian::SetVoxelTOD& in) override { receive_tod       (in); return true; }


          private:
          Image<value_type> buffer;

          // Template functions used so that the functors don't have to be written twice
          //   (once for standard TWI and one for Gaussian track-wise statistic)
          template <class Cont> void receive_greyscale (const Cont&);
          template <class Cont> void receive_dec       (const Cont&);
          template <class Cont> void receive_dixel     (const Cont&);
          template <class Cont> void receive_tod       (const Cont&);

          // Partially specialized template function to shut up modern compilers
          //   regarding using multiplication in a boolean context
          inline void add (const default_type, const default_type);

          // These acquire the TWI factor at any point along the streamline;
          //   For the standard SetVoxel classes, this is a single value 'factor' for the set as
          //     stored in SetVoxelExtras
          //   For the Gaussian SetVoxel classes, there is a factor per mapped element
          default_type get_factor (const Voxel&    element, const SetVoxel&    set) const { return set.factor; }
          default_type get_factor (const VoxelDEC& element, const SetVoxelDEC& set) const { return set.factor; }
          default_type get_factor (const Dixel&    element, const SetDixel&    set) const { return set.factor; }
          default_type get_factor (const VoxelTOD& element, const SetVoxelTOD& set) const { return set.factor; }
          default_type get_factor (const Gaussian::Voxel&    element, const Gaussian::SetVoxel&    set) const { return element.get_factor(); }
          default_type get_factor (const Gaussian::VoxelDEC& element, const Gaussian::SetVoxelDEC& set) const { return element.get_factor(); }
          default_type get_factor (const Gaussian::Dixel&    element, const Gaussian::SetDixel&    set) const { return element.get_factor(); }
          default_type get_factor (const Gaussian::VoxelTOD& element, const Gaussian::SetVoxelTOD& set) const { return element.get_factor(); }


          // Convenience functions for Directionally-Encoded Colour processing
          Eigen::Vector3 get_dec ();
          void           set_dec (const Eigen::Vector3&);

          // Convenience functions for Track Orientation Distribution processing
          void get_tod (      VoxelTOD::vector_type&);
          void set_tod (const VoxelTOD::vector_type&);

        };







        template <typename value_type>
          template <class Cont>
          void MapWriter<value_type>::receive_greyscale (const Cont& in)
          {
            assert (MapWriterBase::type == GREYSCALE);
            for (const auto& i : in) {
              assign_pos_of (i).to (buffer);
              const default_type factor = get_factor (i, in);
              const default_type weight = in.weight * i.get_length();
              switch (voxel_statistic) {
                case V_SUM:  add (weight, factor); break;
                case V_MIN:  buffer.value() = std::min (default_type (buffer.value()), factor); break;
                case V_MAX:  buffer.value() = std::max (default_type (buffer.value()), factor); break;
                case V_MEAN:
                             add (weight, factor);
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
              const default_type factor = get_factor (i, in);
              const default_type weight = in.weight * i.get_length();
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
              const default_type factor = get_factor (i, in);
              const default_type weight = in.weight * i.get_length();
              switch (voxel_statistic) {
                case V_SUM:  add (weight, factor); break;
                case V_MIN:  buffer.value() = std::min (default_type (buffer.value()), factor); break;
                case V_MAX:  buffer.value() = std::max (default_type (buffer.value()), factor); break;
                case V_MEAN:
                             add (weight, factor);
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
            VoxelTOD::vector_type sh_coefs;
            for (const auto& i : in) {
              assign_pos_of (i, 0, 3).to (buffer);
              const default_type factor = get_factor (i, in);
              const default_type weight = in.weight * i.get_length();
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




        template <>
        inline void MapWriter<bool>::add (const default_type weight, const default_type factor)
        {
          if (weight && factor)
            buffer.value() = true;
        }

        template <typename value_type>
        inline void MapWriter<value_type>::add (const default_type weight, const default_type factor)
        {
          buffer.value() += weight * factor;
        }





        template <typename value_type>
          Eigen::Vector3 MapWriter<value_type>::get_dec ()
          {
            assert (type == DEC);
            Eigen::Vector3 value;
            buffer.index(3) = 0; value[0] = buffer.value();
            buffer.index(3)++;   value[1] = buffer.value();
            buffer.index(3)++;   value[2] = buffer.value();
            return value;
          }

        template <typename value_type>
          void MapWriter<value_type>::set_dec (const Eigen::Vector3& value)
          {
            assert (type == DEC);
            buffer.index(3) = 0; buffer.value() = value[0];
            buffer.index(3)++;   buffer.value() = value[1];
            buffer.index(3)++;   buffer.value() = value[2];
          }





        template <typename value_type>
          void MapWriter<value_type>::get_tod (VoxelTOD::vector_type& sh_coefs)
          {
            assert (type == TOD);
            sh_coefs.resize (buffer.size(3));
            for (auto l = Loop (3) (buffer); l; ++l)
              sh_coefs[buffer.index(3)] = buffer.value();
          }

        template <typename value_type>
          void MapWriter<value_type>::set_tod (const VoxelTOD::vector_type& sh_coefs)
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



