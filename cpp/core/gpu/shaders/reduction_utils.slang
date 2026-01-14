module ReductionUtils;
import atomic_utils;

public interface IReduceOp<T>
{
    static T identityElement();
    static T reduce(T a, T b);
};

// Value: the logical value type (float | uint32_t | int32_t)
// Storage: the type used in the atomic variable (uint32_t for float; same as Value for ints)
public interface IAtomicReduceOp<Value, Storage : IAtomicable> 
{
    static Storage identityElement();
    static void atomicReduce(__ref Atomic<Storage> dest, Value value);
    static void atomicReduceEncoded(__ref Atomic<Storage> dest, Storage value);
}

public struct SumOp<T> : IReduceOp<T> where T : IArithmetic
{
    public static T identityElement() { return T(0); }
    public static T reduce(T a, T b) { return a + b; }
};

public struct SumArrayOp<T, let N : uint32_t> : IReduceOp<Array<T, N>> where T : IArithmetic
{
    public static Array<T, N> identityElement() { Array<T, N> v; for (uint i = 0; i < N; i++) v[i] = T(0); return v; }
    public static Array<T, N> reduce(Array<T, N> a, Array<T, N> b)
    {
        Array<T, N> r;
        for (uint i = 0; i < N; i++)
            r[i] = a[i] + b[i];
        return r;
    }
};

public struct SumFloatOp : IReduceOp<float>
{
    public static float identityElement() { return 0.0F; }
    public static float reduce(float a, float b) { return a + b; }
};

public struct MinFloatOp : IReduceOp<float>
{
    public static float identityElement() { return float.maxValue; }
    public static float reduce(float a, float b) { return min(a, b);
    }
};

public struct MaxFloatOp : IReduceOp<float>
{
    public static float identityElement() { return float.minValue; }
    public static float reduce(float a, float b) { return max(a, b); }
};

// NOTE: These operations only works for positive floats
// We exploit the fact given two positive float a,b with a > b, we have
// asuint(a) > asuint(b).
public struct AtomicMinPositiveFloatOp : IAtomicReduceOp<float, uint32_t>
{
    public static uint32_t identityElement() { return 0xFFFFFFFFu; } // +INF
    public static void atomicReduce(__ref Atomic<uint32_t> dst, float v)
    {
        dst.min(asuint(max(v, 0.0F)));
    }
    public static void atomicReduceEncoded(__ref Atomic<uint32_t> dst, uint32_t v)
    {
        dst.min(v);
    }
}

public struct AtomicMaxPositiveFloatOp : IAtomicReduceOp<float, uint32_t>
{
    public static uint32_t identityElement() { return 0; } // -INF
    public static void atomicReduce(__ref Atomic<uint> dst, float v)
    {
        dst.max(asuint(v));
    }
    public static void atomicReduceEncoded(__ref Atomic<uint32_t> dst, uint32_t v)
    {
        dst.max(v);
    }
}

// Encode floats into an order-preserving uint so unsigned comparisons match float ordering
// (including negatives). Mapping: for positive values flip the sign bit; for negative values
// bitwise-not the whole word.
// See https://stackoverflow.com/a/72461459
public uint32_t floatToOrderedUint(float v)
{
    let bits = asuint(v);
    let mask = (bits & 0x80000000u) != 0u ? 0xFFFFFFFFu : 0x80000000u;
    return bits ^ mask;
}

public float orderedUintToFloat(uint32_t v)
{
    let mask = (v & 0x80000000u) != 0u ? 0x80000000u : 0xFFFFFFFFu;
    return asfloat(v ^ mask);
}

public struct AtomicMinFloatOp : IAtomicReduceOp<float, uint32_t>
{
    public static uint32_t identityElement() { return floatToOrderedUint(float.maxValue); }
    public static void atomicReduce(__ref Atomic<uint32_t> dst, float v)
    {
        dst.min(floatToOrderedUint(v));
    }
    public static void atomicReduceEncoded(__ref Atomic<uint32_t> dst, uint32_t v)
    {
        dst.min(v);
    }
}

public struct AtomicMaxFloatOp : IAtomicReduceOp<float, uint32_t>
{
    public static uint32_t identityElement() { return floatToOrderedUint(-float.maxValue); }
    public static void atomicReduce(__ref Atomic<uint32_t> dst, float v)
    {
        dst.max(floatToOrderedUint(v));
    }
    public static void atomicReduceEncoded(__ref Atomic<uint32_t> dst, uint32_t v)
    {
        dst.max(v);
    }
}

// A function to perform a parallel reduction using the provided operation within a workgroup.
// Note: The size of the workgroup data must be a power of two.
public DataType workgroupReduce<DataType, Operation : IReduceOp<DataType>, uint32_t workgroupDataSize>
                               (__ref Array<DataType, workgroupDataSize> localData,uint32_t localIndex)
{
    static_assert(workgroupDataSize <= 1024, "workgroupDataSize must be <= 1024");
    static_assert((workgroupDataSize & (workgroupDataSize - 1)) == 0, "workgroupDataSize must be a power of two"); 
                                                                                                                        
    for (uint offset = workgroupDataSize / 2; offset > 0; offset /= 2)
    {
        if (localIndex < offset)
        {
            localData[localIndex] = Operation.reduce(localData[localIndex], localData[localIndex + offset]);
        }
        GroupMemoryBarrierWithGroupSync();
    }
    return localData[0];
}


interface IReduceWaveOp<T>
{
    inline static T identityElement();
    inline static T waveReduce(T value);
}

public struct ISumArrayWaveOp<T, let N : uint32_t> : IReduceWaveOp<Array<T, N>> where T : __BuiltinArithmeticType {
    inline static Array<T, N> identityElement() { return Array<T, N>(); }
    inline static Array<T, N> waveReduce(Array<T, N> array) {
        Array<T, N> reduced;
        for (var i = 0; i < N; ++i) { reduced[i] = WaveActiveSum(array[i]); }
        return reduced;
    }
    inline static Array<T, N> reduce(Array<T, N> a, Array<T, N> b) {
        Array<T, N> reduced;
        for (var i = 0; i < N; ++i) { reduced[i] = a[i] + b[i]; }
        return reduced;
    }
}

uint32_t wgslWorkgroupUniformLoad(uint32_t value)
{
    // See https://www.w3.org/TR/WGSL/#workgroupUniformLoad-builtin
    __intrinsic_asm "workgroupUniformLoad(&$0)";
}

[ForceInline]
public DataType workgroupReduceWithWaves<DataType, Operation : IReduceWaveOp<DataType>>
(__ref DataType threadValue, __ref DataType[] wavePartials, uint32_t numWaves, uint32_t waveIndex, uint32_t localIndex)
{
    // TODO: if this function is called multiple times in a kernel, then we are unnecessarily reading/writing from/to
    // shared memory. Should we take this parameters as function arguments instead?
    static groupshared uint32_t wgUniformNumWaves;
    static groupshared uint32_t wgUniformWaveSize;

    let waveReduced = Operation.waveReduce(threadValue);
    let laneIndex = WaveGetLaneIndex();
    let waveSize = WaveGetLaneCount();

    if (laneIndex == 0U) {
        wavePartials[waveIndex] = waveReduced;
    }

    if (localIndex == 0) {
        wgUniformNumWaves = numWaves;
        wgUniformWaveSize = waveSize;
    }

    GroupMemoryBarrierWithGroupSync();

    // We need this workaround because otherwise we break WGSL's subgroup uniformity analysis
    // See https://github.com/shader-slang/slang/issues/8774
    let uniformNumWaves = wgslWorkgroupUniformLoad(wgUniformNumWaves);
    let uniformWaveSize = wgslWorkgroupUniformLoad(wgUniformWaveSize);
    // We now each a single partial item for each wave, so we iterate in a loop
    // to combine them into one, each round shrinking the problem by waveSize.
    // For example, assume waveSize=4 and numWaves=10, then:
    // Indices:   0 1 2 3   4 5 6 7   8 9
    // Partials: [0 1 2 3] [4 5 6 7] [8 9]
    //           ^block 0  ^block 1  ^block 2
    // After the first iteration, we will have 3 remaining items.

    if (uniformNumWaves > uniformWaveSize) {
        var remaining = uniformNumWaves;
        while (remaining > 1U) {
            let baseIndex = waveIndex * uniformWaveSize;
            // Each wave will reduced up to waveSize items, but the last wave may have fewer and
            // some waves will have none.
            let blockCount = (baseIndex < remaining) ? min(uniformWaveSize, remaining - baseIndex) : 0U;
            let laneValue = laneIndex < blockCount ? wavePartials[baseIndex + laneIndex] : Operation.identityElement();
            GroupMemoryBarrierWithGroupSync();

            let blockReduced = Operation.waveReduce(laneValue);
            let newRemaining = (remaining + uniformWaveSize - 1) / uniformWaveSize;

            if (laneIndex == 0U && waveIndex < newRemaining) {
                wavePartials[waveIndex] = blockReduced;
            }

            GroupMemoryBarrierWithGroupSync();
            remaining = newRemaining;
        }
    }

    // If the number of remaning elements can fit in the subgroup size we can just need an extra wave reduction
    else {
        let laneValue = (waveIndex == 0  && laneIndex < uniformNumWaves) ? wavePartials[laneIndex] : Operation.identityElement();
        let finalValue = Operation.waveReduce(laneValue);
        if (waveIndex == 0) {
            wavePartials[0] = finalValue;
        }
        GroupMemoryBarrierWithGroupSync();
    }

    return wavePartials[0];
}
