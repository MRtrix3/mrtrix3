// Compute shader performing per-workgroup reduction on a 3D texture across multiple operations.
// Threads load voxel intensities into shared memory, then a parallel reduction is applied for
// each IReduceOp<float> in the generic pack.

import reduction_utils;
import texture_utils;

extern static const uint kWorkgroupSizeX = 8;
extern static const uint kWorkgroupSizeY = 8;
extern static const uint kWorkgroupSizeZ = 4;
static const uint kWorkgroupTotalSize = kWorkgroupSizeX * kWorkgroupSizeY * kWorkgroupSizeZ;

typealias ToFloat<T> = float;


// A "meta" reduction operation that applies a pack of operations
// to a tuple of values.
struct CompositeReduceOp<each Operation : IReduceOp<float>> : 
IReduceOp<Tuple<expand ToFloat<each Operation>>>
{
    typealias ValueTuple = Tuple<expand ToFloat<each Operation>>;
    static const int ValueTupleSize = countof(expand each Operation);

    static ValueTuple identityElement()
    {
        return makeTuple(expand(each Operation).identityElement());
    }

    static ValueTuple reduce(ValueTuple a, ValueTuple b)
    {
        return makeTuple(expand(each Operation).reduce(each a, each b));
    }
};

struct Uniforms
{
    uint32_t3 dispatchGrid;
};

struct UniformsWithTransform {
    uint32_t3 dispatchGrid;
    float4x4 imageTransform;
};

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void main_sum(
    uint32_t3 globalId: SV_DispatchThreadID,
    uint32_t3 localId: SV_GroupThreadID,
    uint32_t3 workgroupId: SV_GroupID,
    ConstantBuffer<Uniforms> uniforms,
    Texture3D<float> inputImage,
    RWStructuredBuffer<float> output,
    SamplerState sampler)
{
    reduce<SumFloatOp>(globalId, localId, workgroupId, uniforms.dispatchGrid, inputImage, output, none, sampler);
}

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void main_sum_with_transform(
    uint32_t3 globalId: SV_DispatchThreadID,
    uint32_t3 localId: SV_GroupThreadID,
    uint32_t3 workgroupId: SV_GroupID,
    ConstantBuffer<UniformsWithTransform> uniforms,
    Texture3D<float> inputImage,
    RWStructuredBuffer<float> output,
    SamplerState sampler)
{
    reduce<SumFloatOp>(globalId, localId, workgroupId, uniforms.dispatchGrid, inputImage, output, uniforms.imageTransform, sampler);
}

void reduce<each Operation : IReduceOp<float>>(
    uint32_t3 globalId,
    uint32_t3 localId,
    uint32_t3 workgroupId,
    uint32_t3 dispatchGrid,
    Texture3D<float> inputImage,
    RWStructuredBuffer<float> output,
    Optional<float4x4> transformation,
    SamplerState sampler
)
{
    typealias ValueTuple = CompositeReduceOp<expand each Operation>.ValueTuple;
    typealias CompositeOperation = CompositeReduceOp<expand each Operation>;
    static const int operationsCount = countof(expand each Operation);
    static groupshared Array<ValueTuple, kWorkgroupTotalSize> localData;

    uint3 texDim;
    inputImage.GetDimensions(texDim.x, texDim.y, texDim.z);
    let localIndex = localId.x + localId.y * kWorkgroupSizeX + localId.z * kWorkgroupSizeX * kWorkgroupSizeY;

    // Each thread stores its corresponding voxel intensity in the local data array.
    ValueTuple threadValue;
    if (all(globalId < texDim)) {
        if (transformation.hasValue) {
            let transformedVoxel4 : float4 = mul(transformation.value, float4(globalId, 1.0f));
            let transformedVoxel = transformedVoxel4.xyz / transformedVoxel4.w;
            float voxelIntensity = 0.0F;
            if (all(transformedVoxel >= 0.0F) && all(transformedVoxel <= float3(texDim - 1u))) {
                voxelIntensity = inputImage.SampleLevel(sampler, (transformedVoxel.xyz + 0.5F) / float3(texDim), 0.0).r;
            }
            threadValue = makeTuple(expand ToFloat<each Operation>(voxelIntensity));
        }
        else {
            let voxelIntensity = inputImage.Load(int4(globalId, 0));
            threadValue = makeTuple(expand ToFloat<each Operation>(voxelIntensity));
        }
    }
    else {
        threadValue = CompositeOperation.identityElement();
    }
    localData[localIndex] = threadValue;
    GroupMemoryBarrierWithGroupSync();

    let resultValue = workgroupReduce<ValueTuple, CompositeOperation, kWorkgroupTotalSize>(localData, localIndex);

    // The first thread in the workgroup writes the final result for the group.
    if (localIndex == 0)
    {
        let wgIndex = workgroupId.x
            + workgroupId.y * dispatchGrid[0]
            + workgroupId.z * dispatchGrid[0] * dispatchGrid[1];
        int i = 0;
        expand output[wgIndex * operationsCount + i++] = each resultValue;
    }
}


void reduceAtomic<each Operation : IAtomicReduceOp<float, uint32_t>>(
    uint32_t3 globalId,
    uint32_t3 localId,
    uint32_t3 workgroupId,
    uint32_t3 dispatchGrid,
    Texture3D<float> inputImage,
    RWStructuredBuffer<Atomic<uint32_t>> output, // in global memory
    Optional<float4x4> transformation,
    SamplerState sampler)
{
    static const int operationsCount = countof(expand each Operation);
    static_assert(operationsCount > 0, "At least one operation must be provided");

    static groupshared Array<Atomic<uint32_t>, operationsCount> localData;

    let localIndex = localId.x + localId.y * kWorkgroupSizeX + localId.z * kWorkgroupSizeX * kWorkgroupSizeY;
    if(localIndex == 0) {
        int i = 0;
        expand localData[i++].store((each Operation).identityElement());
    }

    GroupMemoryBarrierWithGroupSync();

    uint32_t3 textureDims = textureSize(inputImage);

    if(all(globalId < textureDims)) {
        float value;
        if (transformation.hasValue)
        {
            let samplingField = VoxelSamplingField(inputImage, sampler);
            let transformedVoxel = mul(transformation.value, float4(globalId, 1.0f)).xyz;
            float voxelIntensity = 0.0F;
            if (all(transformedVoxel >= 0.0F) && all(transformedVoxel <= float3(textureDims - 1u))) {
                voxelIntensity = samplingField.sample(transformedVoxel);
            }
            value = voxelIntensity;
        }
        else
        {
            value = inputImage.Load(int4(globalId, 0)).r;
        }

        int i = 0;
        expand (each Operation).atomicReduce(localData[i++], value);
    }

    GroupMemoryBarrierWithGroupSync();

    // Global reduction
    if(localIndex == 0) {
        int j = 0;
        expand (each Operation).atomicReduceEncoded(output[j], localData[j++].load());
    }
}

// Min/Max reduction with atomic operations using ordered-float encoding to handle negatives
[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void minMaxAtomic(
    uint32_t3 globalId: SV_DispatchThreadID,
    uint32_t3 localId: SV_GroupThreadID,
    uint32_t3 workgroupId: SV_GroupID,
    ConstantBuffer<Uniforms> uniforms,
    Texture3D<float> inputTexture,
    RWStructuredBuffer<Atomic<uint32_t>> outputBuffer,
    SamplerState sampler)
{
    reduceAtomic<AtomicMinFloatOp, AtomicMaxFloatOp>(globalId, localId, workgroupId, uniforms.dispatchGrid, inputTexture, outputBuffer, none, sampler);
}

// Min/Max reduction with atomic operations and a given transformation from the target coordinates
// to the source coordinates.
[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void minMaxAtomicWithTransform(
    uint32_t3 globalId: SV_DispatchThreadID,
    uint32_t3 localId: SV_GroupThreadID,
    uint32_t3 workgroupId: SV_GroupID,
    ConstantBuffer<UniformsWithTransform> uniforms,
    Texture3D<float> inputTexture,
    RWStructuredBuffer<Atomic<uint32_t>> outputBuffer,
    SamplerState sampler)
{
    reduceAtomic<AtomicMinFloatOp, AtomicMaxFloatOp>(globalId, localId, workgroupId, uniforms.dispatchGrid, inputTexture, outputBuffer, uniforms.imageTransform, sampler);
}
