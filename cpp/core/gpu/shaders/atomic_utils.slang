/* Copyright (c) 2008-2026 the MRtrix3 contributors.
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
module atomic_utils;

// The use of __ref is necessary to pass a reference to an Atomic<T> object.
// This is undocumented behaviour. 
// See // https://github.com/shader-slang/slang/issues/5941#issuecomment-2564397693
public float atomicAddF32InMemory(__ref Atomic<uint> sum, float value)
{
    for(uint oldBits = sum.load(); ; )
    {
        uint newBits = asuint(asfloat(oldBits) + value);
        uint prevBits = sum.compareExchange(oldBits, newBits);
        if (prevBits == oldBits) {
            return asfloat(newBits);
        }
        oldBits = prevBits;
    }
}
