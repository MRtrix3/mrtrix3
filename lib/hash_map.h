/*******************************************************************************
    Copyright (C) 2014 Brain Research Institute, Melbourne, Australia
    
    Permission is hereby granted under the Patent Licence Agreement between
    the BRI and Siemens AG from July 3rd, 2012, to Siemens AG obtaining a
    copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to possess, use, develop, manufacture,
    import, offer for sale, market, sell, lease or otherwise distribute
    Products, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*******************************************************************************/


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
