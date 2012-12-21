/*
    Copyright 2008 Brain Research Institute, Melbourne, Australia

    Written by Robert Smith, 2010.

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


#include "dwi/directions/mask.h"



namespace MR {
  namespace DWI {
    namespace Directions {



      const uint8_t Mask::masks[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};


      bool Mask::full() const
      {

        if (excess_bits()) {

          size_t i;
          for (i = 0; i != bytes() - 1; ++i) {
            if (data[i] != 0xFF)
              return false;
          }
          if (((data[i] | ~excess_mask()) & 0x000000FF) != 0xFF)
            return false;
          return true;

        } else {

          for (size_t i = 0; i != bytes(); ++i) {
            if (data[i] != 0xFF)
              return false;
          }
          return true;

        }

      }




      bool Mask::empty() const
      {

        if (excess_bits()) {

          size_t i;
          for (i = 0; i != bytes() - 1; ++i) {
            if (data[i])
              return false;
          }
          if (data[i] & excess_mask())
            return false;
          return true;

        } else {

          for (size_t i = 0; i != bytes(); ++i) {
            if (data[i])
              return false;
          }
          return true;

        }

      }





      void Mask::erode (const size_t iterations)
      {
        for (size_t iter = 0; iter != iterations; ++iter) {
          Mask temp (*this);
          for (size_t d = 0; d != size(); ++d) {
            if (!temp[d]) {
              for (std::vector<size_t>::const_iterator i = dirs.get_adj_dirs(d).begin(); i != dirs.get_adj_dirs(d).end(); ++i)
                reset (*i);
            }
          }
        }
      }




      void Mask::dilate (const size_t iterations)
      {
        for (size_t iter = 0; iter != iterations; ++iter) {
          Mask temp (*this);
          for (size_t d = 0; d != size(); ++d) {
            if (temp[d]) {
              for (std::vector<size_t>::const_iterator i = dirs.get_adj_dirs(d).begin(); i != dirs.get_adj_dirs(d).end(); ++i)
                set (*i);
            }
          }
        }
      }




      size_t Mask::get_min_linkage (const Mask& that)
      {
        size_t iterations = 0;
        for (Mask temp (that); !(temp & *this).empty(); ++iterations)
          temp.dilate();
        return iterations;
      }





      bool Mask::is_adjacent (const size_t d) const
      {
        for (std::vector<size_t>::const_iterator i = dirs.get_adj_dirs (d).begin(); i != dirs.get_adj_dirs (d).end(); ++i) {
          if (test (*i))
            return true;
        }
        return false;
      }





      size_t Mask::count (const bool value) const
      {
        size_t count = 0;
        for (size_t i = 0; i != size(); ++i) {
          if (value == test (i))
            ++count;
        }
        return count;
      }



      bool Mask::operator== (const Mask& that) const
      {

        assert (that.size() == size());

        if (!excess_bits())
          return (!memcmp (data, that.data, bytes()));

        if (memcmp (data, that.data, bytes() - 1))
          return false;
        if ((data[bytes()-1] & excess_mask()) != (that.data[bytes()-1] & excess_mask()))
          return false;
        return true;

      }




      Mask& Mask::operator|= (const Mask& that)
      {
        assert (that.size() == size());
        for (size_t i = 0; i != bytes(); ++i)
          data[i] |= that.data[i];
        return *this;
      }


      Mask& Mask::operator&= (const Mask& that)
      {
        assert (that.size() == size());
        for (size_t i = 0; i != bytes(); ++i)
          data[i] &= that.data[i];
        return *this;
      }


      Mask& Mask::operator^= (const Mask& that)
      {
        assert (that.size() == size());
        for (size_t i = 0; i != bytes(); ++i)
          data[i] ^= that.data[i];
        return *this;
      }



      Mask Mask::operator~() const
      {
        Mask result (dirs);
        for (size_t i = 0; i != bytes(); ++i)
          result.data[i] = ~data[i];
        return result;
      }


      std::ofstream& operator<< (std::ofstream& stream, Mask& d)
      {
        stream << "0x";
        if (d.excess_bits()) {
          stream << d.byte_to_hex (d.data[d.bytes() - 1] & d.excess_mask());
          for (size_t i = d.bytes() - 2; i--;)
            stream << d.byte_to_hex (d.data[i]);
        } else {
          for (size_t i = d.bytes() - 1; i--;)
            stream << d.byte_to_hex (d.data[i]);
        }
        return stream;
      }



      std::string Mask::byte_to_hex (const uint8_t d) const
      {
        std::string out;
        for (size_t i = 0; i != 2; ++i) {
          const uint8_t dm = i ? (d & 0x0F) : ((d & 0xF0) >> 4);
          switch (dm) {
            case 0:  out.push_back ('0'); break;
            case 1:  out.push_back ('1'); break;
            case 2:  out.push_back ('2'); break;
            case 3:  out.push_back ('3'); break;
            case 4:  out.push_back ('4'); break;
            case 5:  out.push_back ('5'); break;
            case 6:  out.push_back ('6'); break;
            case 7:  out.push_back ('7'); break;
            case 8:  out.push_back ('8'); break;
            case 9:  out.push_back ('9'); break;
            case 10: out.push_back ('A'); break;
            case 11: out.push_back ('B'); break;
            case 12: out.push_back ('C'); break;
            case 13: out.push_back ('D'); break;
            case 14: out.push_back ('E'); break;
            case 15: out.push_back ('F'); break;
          }
        }
        return out;
      }



    }
  }
}
