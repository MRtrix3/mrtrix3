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
