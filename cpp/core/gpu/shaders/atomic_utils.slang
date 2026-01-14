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
