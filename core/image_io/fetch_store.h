/* Copyright (c) 2008-2023 the MRtrix3 contributors.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Covered Software is provided under this License on an "as is"
 * basis, without warranty of any kind, either expressed, implied, or
 * statutory, including, without limitation, warranties that the
 * Covered Software is free of defects, merchantable, fit for a
 * particular purpose or non-infringing.
 * See the Mozilla Public License v. 2.0 for more details.
 *
 * For more details, see http://www.mrtrix.org/.
 */

#ifndef __image_io_fetch_store_h__
#define __image_io_fetch_store_h__

#include "raw.h"
#include "datatype.h"

namespace MR
{


  template <typename ValueType>
    typename std::enable_if<!is_data_type<ValueType>::value, void>::type __set_fetch_store_functions (
        std::function<ValueType(const void*,size_t,default_type,default_type)>& /*fetch_func*/,
        std::function<void(ValueType,void*,size_t,default_type,default_type)>& /*store_func*/,
        DataType /*datatype*/) { }



  template <typename ValueType>
    typename std::enable_if<is_data_type<ValueType>::value, void>::type __set_fetch_store_functions (
        std::function<ValueType(const void*,size_t,default_type,default_type)>& fetch_func,
        std::function<void(ValueType,void*,size_t,default_type,default_type)>& store_func,
        DataType datatype);


}

#endif



