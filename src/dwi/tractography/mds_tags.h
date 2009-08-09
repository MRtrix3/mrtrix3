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

#ifndef __dwi_tractography_mds_tags_h__
#define __dwi_tractography_mds_tags_h__

#include "data_type.h"

namespace MR {
  namespace DWI {
    namespace Tractography {

      class Tag {
        protected:
          uint32_t  id;
        public:
          Tag ()                      { id = 0; }
          Tag (uint32_t n)             { id = n; }
          Tag (const Tag& t)          { id = t.id; }
          Tag (uint32_t l1, uint32_t l2, uint32_t l3, DataType dt) { id = (l1 << 24) | (l2 << 16) | (l3 << 8) | dt(); }

          const Tag&   operator= (const Tag& t)      { id = t.id; return (*this); }
          uint32_t      operator() () const           { return (id); }
          uint32_t      operator[] (int n) const     { return ((id >> 8*(3-n)) & 0x000000FFU); }
          bool         operator== (const Tag& t) const  { return (id == t.id); }
          bool         operator!= (const Tag& t) const  { return (id != t.id); }

          void         set_type (DataType dt)           { id = ( id & 0xFFFFFF00U ) | dt(); }
          DataType     type () const                { return (id & 0x000000FFU); }
          void         set_BO (bool BE);
          void         unset_BO ();

          static const uint8_t     Text          = 0xFFU;
          static const uint8_t     GroupStart    = 0xFEU;
          static const uint8_t     GroupEnd      = 0xFDU;
      };

      std::ostream& operator<< (std::ostream& stream, const Tag& t);












      inline void Tag::set_BO (bool BE) { 
        DataType dt (type()); 
        if (dt != DataType::Bit && dt != DataType::Int8 && dt != DataType::UInt8 &&
            dt != DataType::Undefined && dt != DataType::Text && dt != DataType::GroupStart &&
            dt != DataType::GroupEnd) {
          if (BE) dt.set_flag (DataType::BigEndian);
          else dt.set_flag (DataType::LittleEndian);
        }
        id = ( id & 0xFFFFFF00U ) | dt();
      }
      inline void  Tag::unset_BO () { 
        DataType dt (type()); 
        if (dt != DataType::Text && dt != DataType::GroupStart && dt != DataType::GroupEnd) 
          dt.unset_flag (DataType::LittleEndian | DataType::BigEndian);
        id = ( id & 0xFFFFFF00U ) | dt();
      }

      inline std::ostream& operator<< (std::ostream& stream, const Tag& t)
      {
        stream << "(" << t[0] << "." << t[1] << "." << t[2] << ":" << t.type().specifier() << ")";
        return (stream);
      }




      namespace Tags {
        static const Tag End     (0, 0, 255, DataType::Undefined);
        static const Tag Skip    (0, 0,   1, DataType::Undefined);
        static const Tag Cmd     (0, 0,   2, DataType::Text);
        static const Tag Comment (0, 0,   3, DataType::Text);

        static const Tag Track         (2, 0, 0, DataType::Float32);

        namespace ROI {
          static const Tag Start       (2, 1, 0, DataType::GroupStart);
          static const Tag Type        (2, 1, 1, DataType::UInt8);
          static const Tag Shape       (2, 1, 2, DataType::UInt8);
          static const Tag SphereParam (2, 1, 3, DataType::Float32);
          static const Tag MaskParam   (2, 1, 4, DataType::Text);
          static const Tag End         (2, 1, 0, DataType::GroupEnd);
        }   

        static const Tag Method         (2, 0,  1, DataType::Text);
        static const Tag Source         (2, 0,  2, DataType::Text);
        static const Tag StepSize       (2, 0,  3, DataType::Float32);
        static const Tag MaxNumTracks   (2, 0,  4, DataType::UInt32);
        static const Tag MaxDist        (2, 0,  5, DataType::Float32);
        static const Tag Threshold      (2, 0,  6, DataType::Float32);
        static const Tag InitThreshold  (2, 0,  7, DataType::Float32);
        static const Tag MinCurv        (2, 0,  8, DataType::Float32);
        static const Tag Mask           (2, 0,  9, DataType::Text);
        static const Tag UniDirectional (2, 0, 10, DataType::UInt8);
        static const Tag InitDirection  (2, 0, 11, DataType::Float32);
        static const Tag NumTracksGenerated  (2, 0, 12, DataType::UInt32);
        static const Tag MaxNumTracksGenerated  (2, 0, 13, DataType::UInt32);
        static const Tag MaskThreshold  (2, 0, 14, DataType::Float32);

        namespace SD {
          static const Tag LMax         (2, 0, 16, DataType::UInt32);
          static const Tag Precomputed  (2, 0, 17, DataType::UInt8);
          static const Tag MaxTrials    (2, 0, 18, DataType::UInt32);
        }   
      }   

    }
  }
}


#endif

