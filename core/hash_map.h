/* Copyright (c) 2008-2017 the MRtrix3 contributors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, you can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * MRtrix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * For more details, see http://www.mrtrix.org/.
 */


#ifndef __hash_map_h__
#define __hash_map_h__

#ifdef MRTRIX_USE_TR1
#  include <tr1/unordered_map>
#  define MRTRIX_HASH_MAP_TYPE std::tr1::unordered_map
#elif MRTRIX_USE_HASH_MAP
#  ifdef __GNUC__
#    if __GNUC__ < 3
#      include <hash_map.h>
#      define MRTRIX_HASH_MAP_TYPE ::hash_map
#    else
#      include <ext/hash_map>
#      define MRTRIX_HASH_MAP_TYPE ::__gnu_cxx::hash_map
#    endif
#  endif
#else
#  include <unordered_map>
#  define MRTRIX_HASH_MAP_TYPE std::unordered_map
#endif


namespace MR
{
  template <class K, class V> struct UnorderedMap {
    typedef
    MRTRIX_HASH_MAP_TYPE <K,V> Type;
  };
}

#undef MRTRIX_HASH_MAP_TYPE

#endif
