// TODO: Write math and derivation of gradients
import global_transformation;
import reduction_utils;
import texture_utils;
import coordinate_mapper;
import voxelscannermatrices;

struct SSDParameters<let N : int>
{
    float ssd;
    Array<float, N> gradients;
};

struct SSDReductionOP<let N : int> : IReduceOp<SSDParameters<N>>
{
    static SSDParameters<N> identityElement() {
        Array<float, N> gradients;
        for (uint32_t i = 0U; i < N; ++i)
        {
            gradients[i] = 0.0F;
        }
        return SSDParameters<N>(0.0F, gradients);
    }

    static SSDParameters<N> reduce(SSDParameters<N> a, SSDParameters<N> b) {
        let ssd = a.ssd + b.ssd;
        Array<float, N> gradients;
        for (uint32_t i = 0U; i < N; ++i)
        {
            gradients[i] = a.gradients[i] + b.gradients[i];
        }
        return SSDParameters<N>(ssd, gradients);
    }
};


extern static const uint32_t kWorkgroupSizeX = 8;
extern static const uint32_t kWorkgroupSizeY = 8;
extern static const uint32_t kWorkgroupSizeZ = 4;
// TODO: Use extern static const bool once the Slang compiler bug is fixed.
extern static const uint32_t kUseSourceMask;
extern static const uint32_t kUseTargetMask;

extern static const uint32_t kComputeGradients;
static const uint32_t workgroupInvocations = kWorkgroupSizeX * kWorkgroupSizeY * kWorkgroupSizeZ;


struct Uniforms<Transformation> where Transformation : ITransformation{
    uint3 dispatchGrid;
    float3 transformationPivot;
    Transformation.TParams currentTransform;
    VoxelScannerMatrices voxelScannerMatrices;
};

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void main<Transformation>(
    uint3 globalId: SV_DispatchThreadID,
    uint3 localId: SV_GroupThreadID,
    uint3 workgroupId: SV_GroupID,
    ConstantBuffer<Uniforms<Transformation>> uniforms,
    Texture3D<float> sourceImage,
    Texture3D<float> targetImage,
    Texture3D<float> sourceMask,
    Texture3D<float> targetMask,
    SamplerState linearSampler,
    RWStructuredBuffer<SSDParameters<Transformation.kParamCount>> ssdAndGradientsPartials,
    RWStructuredBuffer<Atomic<uint32_t>> numContributingVoxels
) where Transformation : ITransformation
{
    static const uint32_t paramsCount = Transformation.kParamCount;
    typedef SSDParameters<paramsCount> CurrentSSDParameters;
    static groupshared Array<CurrentSSDParameters, workgroupInvocations> localParameters;
    static groupshared Atomic<uint32_t> localNumContributingVoxels;

    var ssdParams = CurrentSSDParameters(0.0F, Array<float, paramsCount>());
    let localIndex = localId.x + localId.y * kWorkgroupSizeX + localId.z * kWorkgroupSizeX * kWorkgroupSizeY;
    let sourceDim = textureSize(sourceImage);
    let targetDim = textureSize(targetImage);

    let coordMapper = CoordinateMapper(
        sourceDim, targetDim, uniforms.voxelScannerMatrices
    );

    let transformation = Transformation(uniforms.currentTransform, uniforms.transformationPivot);
    let sourceTextureField = VoxelSamplingField(sourceImage, linearSampler);
    let sourceMaskField = VoxelSamplingField(sourceMask, linearSampler);
    let targetMaskField = VoxelSamplingField(targetMask, linearSampler);

    if (localIndex == 0U) {
        localNumContributingVoxels.store(0U);
    }
    GroupMemoryBarrierWithGroupSync();

    bool includeVoxel = coordMapper.inTargetInt(int3(globalId));
    let targetVoxelCoord = float3(globalId);

    if (includeVoxel && targetMaskField.maskAccepts(targetVoxelCoord, kUseTargetMask != 0U)) {
        let movingVoxelCoord = coordMapper.mapTargetVoxelToSource(targetVoxelCoord, transformation);
        if (coordMapper.inSource(movingVoxelCoord) && sourceMaskField.maskAccepts(movingVoxelCoord, kUseSourceMask != 0U)) {
            let targetIntensity = targetImage.Load(int4(globalId, 0)).r;
            let movingIntensity = sourceTextureField.sample(movingVoxelCoord);
            let error = movingIntensity - targetIntensity;
            let gradMovingVoxel = sourceTextureField.spatialGradient(movingVoxelCoord);
            let gradMovingScanner = coordMapper.mapVoxelGradientToScanner(gradMovingVoxel);
            let targetScannerCoord = coordMapper.mapTargetVoxelToScanner(targetVoxelCoord);

            var voxelGradients : Array<float, paramsCount>;
            if (kComputeGradients != 0U) {
                for (uint32_t i = 0U; i < paramsCount; ++i) {
                    voxelGradients[i] = error * dot(gradMovingScanner, transformation.jacobianVector(i, targetScannerCoord));
                }
            }
            else {
                for (uint32_t i = 0U; i < paramsCount; ++i) {
                    voxelGradients[i] = 0.0F;
                }
            }
            ssdParams = CurrentSSDParameters(0.5F * error * error, voxelGradients);
            localNumContributingVoxels.increment();
        }
    }
    localParameters[localIndex] = ssdParams;

    GroupMemoryBarrierWithGroupSync();

    let finalValue = workgroupReduce< CurrentSSDParameters, SSDReductionOP<paramsCount>, workgroupInvocations>(
        localParameters, localIndex);

    if (localIndex == 0U) {
        let wgIndex = workgroupId.x
                    + workgroupId.y * uniforms.dispatchGrid[0]
                    + workgroupId.z * uniforms.dispatchGrid[0] * uniforms.dispatchGrid[1];
        ssdAndGradientsPartials[wgIndex] = finalValue;
        numContributingVoxels[0].add(localNumContributingVoxels.load());
    }
}
