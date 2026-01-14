// This shader computes a joint histogram of intensities from a given source and target volume.
import atomic_utils;
import cubic_bspline;
import texture_utils;
import reduction_utils;

extern static const uint32_t kWorkgroupSizeX = 8;
extern static const uint32_t kWorkgroupSizeY = 8;
extern static const uint32_t kWorkgroupSizeZ = 4;


extern static const int32_t kNumBins;
// TODO: Use extern static const bool once the Slang compiler bug is fixed.
extern static const uint32_t kUseFixedMask;
extern static const uint32_t kUseMovingMask;
static const int32_t kPadding = 2; // reserve bins at each end so center never touches histogram edge
static const int32_t kWorkgroupInvocations = kWorkgroupSizeX * kWorkgroupSizeY * kWorkgroupSizeZ;

// A small epsilon to avoid division by zero errors.
static const float epsilon = 1e-9;

struct Intensities
{
    float sourceMin;
    float sourceMax;
    float targetMin;
    float targetMax;
}

struct Uniforms
{
    uint32_t3 dispatchGrid;
    Intensities intensities;
    // Optional linear transformation matrix from target image coordinates to source image coordinates.
    float4x4 transformationMatrix;
}

// Map intensity to nearest bin while preserving padding at edges
uint32_t mapIntensityToNearestBin(float intensity, float inMin, float inMax)
{
    let range = inMax - inMin;
    let denom = (range <= epsilon) ? epsilon : range;
    let normalized = clamp((intensity - inMin) / denom, 0.0F, 1.0F);
    let usableBins = float(kNumBins - 2 * kPadding);
    let idxF = normalized * (usableBins - 1.0F) + 0.5F;
    let idxClamped = clamp(idxF, 0.0F, usableBins - 1.0F);
    return uint32_t(kPadding) + uint32_t(idxClamped);
}


[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void rawHistogram(
    uint32_t3 globalId: SV_DispatchThreadID,
    uint32_t3 localId: SV_GroupThreadID,
    uint32_t3 workgroupId: SV_GroupID,
    ConstantBuffer<Uniforms> uniforms,
    Texture3D<float> movingTexture,
    Texture3D<float> fixedTexture,
    Texture3D<float> movingMaskTexture,
    Texture3D<float> fixedMaskTexture,
    RWStructuredBuffer<Atomic<uint32_t>> jointHistogram,
    SamplerState sampler)
{
    static groupshared Array<Atomic<uint32_t>, kNumBins * kNumBins> localHistogram;
    static_assert(kNumBins > 0, "kNumBins must be greater than 0");

    let sourceDimensions = textureSize(movingTexture);
    let targetDimensions = textureSize(fixedTexture);
    let sourceDimensionsF = float3(sourceDimensions);

    let localIndex = localId.x + localId.y * kWorkgroupSizeX + localId.z * kWorkgroupSizeX * kWorkgroupSizeY;
    let totalBins = kNumBins * kNumBins;

    // Initialise workgroup-shared histogram and total mass to zero
    for (var i = localIndex; i < totalBins; i += kWorkgroupInvocations) {
        localHistogram[i].store(0U);
    }
    GroupMemoryBarrierWithGroupSync();
    let movingSamplingField = VoxelSamplingField(movingTexture, sampler);
    let movingMaskField = VoxelSamplingField(movingMaskTexture, sampler);
    let fixedMaskField = VoxelSamplingField(fixedMaskTexture, sampler);
    let useFixedMask = kUseFixedMask != 0U;
    let useMovingMask = kUseMovingMask != 0U;

    // Nearest-bin deposition: one atomic add per voxel into the workgroup-local histogram
    bool includeVoxel = all(globalId < targetDimensions);
    float3 movingVoxelCoord = float3(0.0F, 0.0F, 0.0F);
    if (includeVoxel) {
        let targetVoxelCoord = float3(globalId);
        if (useFixedMask) {
            let fixedMaskValue = fixedMaskField.sample(targetVoxelCoord);
            if (fixedMaskValue < 0.5F) {
                includeVoxel = false;
            }
        }
        if (includeVoxel) {
            movingVoxelCoord = mul(uniforms.transformationMatrix, float4(targetVoxelCoord, 1.0F)).xyz;
            let isMovingVoxelInSource = all(movingVoxelCoord >= 0.0F) && all(movingVoxelCoord < sourceDimensionsF);
            if (!isMovingVoxelInSource) {
                includeVoxel = false;
            }
        }
        if (includeVoxel && useMovingMask) {
            let movingMaskValue = movingMaskField.sample(movingVoxelCoord);
            if (movingMaskValue < 0.5F) {
                includeVoxel = false;
            }
        }
    }

    if (includeVoxel) {
        let sourceIntensity = movingSamplingField.sample(movingVoxelCoord);
        let targetIntensity = fixedTexture.Load(int4(globalId, 0)).r;

        let targetBin = mapIntensityToNearestBin(targetIntensity, uniforms.intensities.targetMin, uniforms.intensities.targetMax);
        let sourceBin = mapIntensityToNearestBin(sourceIntensity, uniforms.intensities.sourceMin, uniforms.intensities.sourceMax);

        let binIndex = targetBin * kNumBins + sourceBin;
        localHistogram[binIndex].increment();
    }

    GroupMemoryBarrierWithGroupSync();

    let dispatchGrid = uniforms.dispatchGrid;
    let wgIndex = workgroupId.x + workgroupId.y * dispatchGrid.x + workgroupId.z * dispatchGrid.x * dispatchGrid.y;
    let numHistograms = dispatchGrid.x * dispatchGrid.y * dispatchGrid.z;

    // Merge workgroup-local histograms into global histogram
    for (var i = localIndex; i < totalBins; i += kWorkgroupInvocations) {
        let localCount = localHistogram[i].load();
        if (localCount > 0U) {
            jointHistogram[i].add(localCount);
        }
    }
}


// Smooth the merged histogram by convolving with a cubic B-spline kernel
// NOTE: This only yields an approximation of the Parzen-windowed histogram,
// but it's a good trade-off between accuracy and computational efficiency.
[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, 1)]
void smoothHistogram(
    uint32_t3 globalId: SV_DispatchThreadID,
    ConstantBuffer<Uniforms> uniforms,
    StructuredBuffer<uint32_t> jointHistogram,
    RWStructuredBuffer<float> jointHistogramSmoothed
)
{
    let targetBin = globalId.x;
    let sourceBin = globalId.y;
    if (targetBin >= uint32_t(kNumBins) || sourceBin >= uint32_t(kNumBins)) {
        return;
    }

    var accumulated = 0.0F;
    // cubic B-spline has compact support [-2, 2]
    for (int dTarget = -2; dTarget <= 2; ++dTarget) {
        for (int dSource = -2; dSource <= 2; ++dSource) {
            let neighbourTargetBinInt = int(targetBin) + dTarget;
            let neighbourSourceBinInt = int(sourceBin) + dSource;
            let neighbourTargetBin = uint32_t(clamp(neighbourTargetBinInt, 0, kNumBins - 1));
            let neighbourSourceBin = uint32_t(clamp(neighbourSourceBinInt, 0, kNumBins - 1));
            let weight = cubicBSpline(float(dTarget)) * cubicBSpline(float(dSource));
            let histIndex = neighbourTargetBin * kNumBins + neighbourSourceBin;
            accumulated += weight * float(jointHistogram[histIndex]);
        }
    }

    let outIndex = targetBin * kNumBins + sourceBin;
    jointHistogramSmoothed[outIndex] = accumulated;
}


// This entry point is dispatched with a single workgroup
// to compute the total mass of the joint histogram
static const uint32_t kTotalMassWorkgroupSize = 1024;
[shader("compute")]
[numthreads(kTotalMassWorkgroupSize, 1, 1)]
void computeTotalMass(
    uint32_t3 localId: SV_GroupThreadID,
    StructuredBuffer<float> jointHistogramSmoothed,
    RWStructuredBuffer<float> jointHistogramMass
)
{
    static groupshared Array<float, kTotalMassWorkgroupSize> sharedSums; // one slot per thread
    static const uint32_t totalBins = kNumBins * kNumBins;

    let localIndex = localId.x;

    // Each thread sums a strided range of bins
    var threadSum = 0.0F;
    for (var i = localIndex; i < totalBins; i += kTotalMassWorkgroupSize) {
        threadSum += jointHistogramSmoothed[i];
    }

    sharedSums[localIndex] = threadSum;
    GroupMemoryBarrierWithGroupSync();

    let totalMass = workgroupReduce<float, SumFloatOp, kTotalMassWorkgroupSize>(sharedSums, localIndex);

    if (localIndex == 0u) {
        jointHistogramMass[0] = totalMass;
    }
}
