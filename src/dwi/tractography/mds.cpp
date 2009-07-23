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

#include "dwi/tractography/mds.h"
#include "dwi/tractography/mds_tags.h"

namespace MR {
  namespace DWI {
    namespace Tractography {

      void MDS::read (const std::string& filename, Properties& properties)
      {
        tracks.clear();
        current_offset = 0;
        prop = &properties;

        if (filename.size()) mmap.init (filename);
        else {
          if (mmap.name().size()) mmap.init (mmap.name());
          else throw Exception ("no filename set for MDS file");
        }

        mmap.map();

        if (memcmp (mmap.address(), "MDS#", 4)) 
          throw Exception ("file \"" + mmap.name() + "\" is not in MDS format (unrecognised magic number)");

        is_BE = false;
        if (get<uint16_t> ((uint8_t*) mmap.address() + sizeof (int32_t), is_BE) == 0x0100U) is_BE = true;
        else if (get<uint16_t> ((uint8_t*) mmap.address() + sizeof (uint32_t), is_BE) != 0x0001U) 
          throw Exception ("MDS file \"" + mmap.name() + "\" is badly formed (invalid byte order specifier)");

        current_offset = sizeof (uint32_t) + sizeof (uint16_t);

        while (1) {
          if (size_t(current_offset + 2*sizeof (uint32_t)) >= mmap.size()) 
            throw Exception ("end of file reached before last MDS element in file \"" + mmap.name() + "\"");

          if (size_t(current_offset + 2*sizeof (uint32_t) + size()) >= mmap.size()) 
            throw Exception ("end of file reached before end of data for last MDS element in file \"" + mmap.name() + "\"");

          if (tag().type() == DataType::Undefined && size() != 0) 
            throw Exception ("MDS tag with undefined type and non-zero size encountered in file \"" + mmap.name() + "\"");

          if (tag() == Tags::End) break;

          if (tag().type() == DataType::GroupStart) stack.push_back (tag());
          else if (tag().type() == DataType::GroupEnd) {
            if (stack.size() == 0) 
              throw Exception("MDS file \"" + mmap.name() + "\" is badly formed (unmatched GroupEnd tag)");

            stack.pop_back ();
          }

          if (tag() != Tags::Skip) {
            try { interpret (); }
            catch (...) { throw Exception ("error interpreting MDS tag file \"" + mmap.name() + "\""); }
          }

          current_offset += 2*sizeof (uint32_t) + size();
        };

      }





      void MDS::create (const std::string& filename, const Properties& properties)
      {
        current_offset = 0;
#ifdef BYTE_ORDER_BIG_ENDIAN
        is_BE = true;
#else
        is_BE = false;
#endif

        mmap.init (filename, MDS_SIZE_INC, "tck");
        mmap.set_read_only (false);
        mmap.map();

        memcpy (mmap.address(), "MDS#", 4);
        put<uint16_t> (0x01U, (uint8_t*) mmap.address() + sizeof (uint32_t), is_BE);

        current_offset = next = sizeof (uint32_t) + sizeof (uint16_t);

        tracks.clear();


        std::map<std::string, std::string>::const_iterator i;
        if ((i = properties.find ("method")) != properties.end()) append_string (Tags::Method, i->second);
        if ((i = properties.find ("cmd")) != properties.end())    append_string (Tags::Cmd, i->second);
        if ((i = properties.find ("source")) != properties.end()) append_string (Tags::Source, i->second);
        if ((i = properties.find ("mask")) != properties.end())   append_string (Tags::Mask, i->second);

        if ((i = properties.find ("step_size")) != properties.end())       append_float32 (Tags::StepSize,      to<float32> (i->second));
        if ((i = properties.find ("max_dist")) != properties.end())        append_float32 (Tags::MaxDist,       to<float32> (i->second));
        if ((i = properties.find ("threshold")) != properties.end())       append_float32 (Tags::Threshold,     to<float32> (i->second));
        if ((i = properties.find ("init_threshold")) != properties.end())  append_float32 (Tags::InitThreshold, to<float32> (i->second));
        if ((i = properties.find ("min_curv")) != properties.end())        append_float32 (Tags::MinCurv,       to<float32> (i->second));

        if ((i = properties.find ("max_num_tracks")) != properties.end())  append_uint32 (Tags::MaxNumTracks,   to<uint32_t> (i->second));
        if ((i = properties.find ("unidirectional")) != properties.end())  append_uint8  (Tags::UniDirectional, to<uint8_t> (i->second));

        if ((i = properties.find ("lmax")) != properties.end())            append_int32 (Tags::SD::LMax,        to<int32_t> (i->second));
        if ((i = properties.find ("sh_precomputed")) != properties.end())  append_int8  (Tags::SD::Precomputed, to<int8_t> (i->second));
        if ((i = properties.find ("sd_max_trials")) != properties.end())   append_int32 (Tags::SD::MaxTrials,   to<int32_t> (i->second));


        for (std::vector<RefPtr<ROI> >::const_iterator r = properties.roi.begin(); r != properties.roi.end(); ++r) {
          const ROI& roi (**r);
          append (Tags::ROI::Start);

          append_uint8 (Tags::ROI::Type, roi.type);
          append_uint8 (Tags::ROI::Shape, roi.mask.size() ? 2 : 1);

          if (roi.mask.size()) append_string (Tags::ROI::MaskParam, roi.mask);
          else {
            float32 v[] = { roi.position[0], roi.position[1], roi.position[2], roi.radius };
            append_float32 (Tags::ROI::SphereParam, v, 4);
          }
          append (Tags::ROI::End);
        }

        for (std::vector<std::string>::const_iterator c = properties.comments.begin(); c != properties.comments.end(); ++c)
          append_string (Tags::Comment, *c);

      }




      void MDS::finalize ()
      {
        if (!mmap.is_mapped()) throw Exception ("attempt to write to currently unmapped file \"" + mmap.name() + "\"");
        mmap.set_read_only (true);
        mmap.map();
      }






      void MDS::interpret ()
      {
        Properties& properties (*prop);

        if (tag() == Tags::Track) {
          Track T;
          T.parent = this;
          T.offset = offset();
          T.count = size() / (3*sizeof (float32));
          T.is_BE = BE();
          tracks.push_back (T);
        }
        else if (tag()[0] == 2 && tag()[1] == 1) {

          if ( (tag() == Tags::ROI::End && containers().size() != 0) ||
              (tag() != Tags::ROI::End && containers().size() != 1) ) 
            error ("unexpected hierarchy in track file \"" + name() + "\"!");

          if (tag() == Tags::ROI::Start) { type = ROI::Undefined; shape = 100; sphere_pos.invalidate(); sphere_rad = NAN; mask_name.clear(); }
          else {
            if (tag() == Tags::ROI::Type) type = (ROI::Type) get_uint8();
            else if (tag() == Tags::ROI::Shape) shape = get_uint8();
            else if (tag() == Tags::ROI::SphereParam) {
              if (shape != 1) throw Exception ("invalid parameters supplied for roi in track file \"" + name() + "\"");
              sphere_pos[0] = get_float32 (0);
              sphere_pos[1] = get_float32 (1);
              sphere_pos[2] = get_float32 (2);
              sphere_rad = get_float32 (3);
            }
            else if (tag() == Tags::ROI::MaskParam) {
              if (shape != 2) throw Exception ("invalid parameters supplied for roi in track file \"" + name() + "\"");
              mask_name = get_string();
            }
            else if (tag() == Tags::ROI::End) {
              if (shape == 1) properties.roi.push_back (RefPtr<ROI> (new ROI (type, sphere_pos, sphere_rad)));
              else if (shape == 2) properties.roi.push_back (RefPtr<ROI> (new ROI (type, mask_name)));
            }
            else goto unknown;
          }
        }
        else if (tag() == Tags::StepSize)               properties["step_size"] = str(get_float32());
        else if (tag() == Tags::MaxDist)                properties["max_dist"] = str(get_float32());
        else if (tag() == Tags::Threshold)              properties["threshold"] = str(get_float32());
        else if (tag() == Tags::InitThreshold)          properties["init_threshold"] = str(get_float32());
        else if (tag() == Tags::MinCurv)                properties["min_curv"] = str(get_float32());
        else if (tag() == Tags::MaxNumTracks)           properties["max_num_tracks"] = str(get_uint32());
        else if (tag() == Tags::NumTracksGenerated)     properties["num_tracks_generated"] = str(get_uint32());
        else if (tag() == Tags::MaxNumTracksGenerated)  properties["max_num_tracks_generated"] = str(get_uint32());
        else if (tag() == Tags::MaskThreshold)          properties["mask_threshold"] = str(get_float32());
        else if (tag() == Tags::UniDirectional)         properties["unidirectional"] = ( get_uint8() ? "1" : "0" );
        else if (tag() == Tags::Cmd)                    properties["cmd"] = get_string();
        else if (tag() == Tags::Source)                 properties["source"] = get_string();
        else if (tag() == Tags::Comment)                properties.comments.push_back (get_string());
        else if (tag() == Tags::InitDirection)          properties["init_direction"] = str(get_float32(0)) + "," + str(get_float32(1)) + "," + str(get_float32(2));
        else if (tag() == Tags::Method)                 properties["method"] = uppercase (get_string());
        else if (tag() == Tags::SD::LMax)               properties["lmax"] = str(get_uint32()); 
        else if (tag() == Tags::SD::Precomputed)        properties["sh_precomputed"] = ( get_uint8() ? "1" : "0" );
        else if (tag() == Tags::SD::MaxTrials)          properties["max_trials"] = str(get_uint32()); 
        else if (tag() == Tags::Mask)                   properties.roi.push_back (RefPtr<ROI> (new ROI (ROI::Mask, get_string())));
        else goto unknown;

        return;

unknown:
        VAR (tag());
        error (printf ("unknown tag \"%c.%c.%c:%s\" in track file \"%s\" - ignored", 
              tag()[0], tag()[1], tag()[2], tag().type().specifier(), name().c_str()));
      }









      void MDS::append (const std::vector<Point>& points)
      {
        uint len = 3*sizeof(float32)*points.size();
        append (Tags::Track, len);

        float32* p = (float32*) data();
        for (uint n = 0; n < points.size(); n++) {
          put<float32> (points[n][0], p++, BE());
          put<float32> (points[n][1], p++, BE());
          put<float32> (points[n][2], p++, BE());
        }

        Track tck;
        tck.parent = this;
        tck.offset = offset();
        tck.count = points.size();
        tck.is_BE = BE();
        tracks.push_back (tck);
      }










      void MDS::append (Tag tag_id, uint32_t nbytes)
      {
        if (!mmap.is_mapped()) throw Exception ("attempt to write to currently unmapped file \"" + mmap.name() + "\"");

        tag_id.unset_BO();

        current_offset = next;

        debug (MR::printf ("writing: tag %u.%u.%u (%s), %u bytes at offset %u", 
              tag_id[0], tag_id[1], tag_id[2], tag_id.type().specifier(), nbytes, current_offset));

        size_t new_size = mmap.size();
        while (size_t(current_offset + 4*sizeof (uint32_t) + nbytes) > new_size) 
          new_size += MDS_SIZE_INC;

        if (new_size != mmap.size()) {
          mmap.resize (new_size);
          mmap.map();
        }

        next = current_offset + 2*sizeof (uint32_t) + nbytes;

        put<uint32_t> (0, (uint8_t*) mmap.address() + next + sizeof (uint32_t), is_BE);
        put<uint32_t> (Tags::End(), (uint8_t*) mmap.address() + next, is_BE);

        put<uint32_t> (nbytes, (uint8_t*) mmap.address() + current_offset + sizeof (uint32_t), is_BE);
        put<uint32_t> (tag_id(), (uint8_t*) mmap.address() + current_offset, is_BE);
      }

    }
  }
}

