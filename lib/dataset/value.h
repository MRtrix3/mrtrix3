/*
    Copyright 2009 Brain Research Institute, Melbourne, Australia

    Written by J-Donald Tournier, 07/01/10.

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

#ifndef __dataset_value_h__
#define __dataset_value_h__

namespace MR {
  namespace DataSet {

    //! \addtogroup DataSet
    // @{

    template <class Set> class Value {
      public:
        typedef typename Set::value_type value_type;

        Value (Set& parent) : S (parent) { }
        operator value_type () const { return (S.get_value()); }
        value_type operator= (value_type value) { S.set_value (value); return (value); }
        value_type operator= (const Value& V) { value_type value = V.S.get_value(); S.set_value (value); return (value); }
        value_type operator+= (value_type value) { value += S.get_value(); S.set_value (value); return (value); }
        value_type operator-= (value_type value) { value = S.get_value() - value; S.set_value (value); return (value); }
        value_type operator*= (value_type value) { value *= S.get_value(); S.set_value (value); return (value); }
        value_type operator/= (value_type value) { value = S.get_value() / value; S.set_value (value); return (value); }
      private:
        Set& S;
    };

    //! @}
  }
}

#endif

