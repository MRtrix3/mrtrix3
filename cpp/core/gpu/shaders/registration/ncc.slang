// NCC = Normalized Cross Correlation
// LNCC = Local Normalized Cross Correlation

// Definition (for a neighborhood N around voxel x):
//   LNCC(x) = Cov(I_t, I_m) / sqrt[ Var_t * Var_m ]
//
// where:
//   - I_t are target intensities in neighborhood N
//   - I_m are moving image intensities in neighborhood N (after transformation)
//   - mu_t, mu_m are the means of I_t and I_m in N
//   - Cov(I_t, I_m) = (1/|N|) * Σ_p (I_t(p) - mu_t)(I_m(p) - mu_m)
//   - Var_t = (1/|N|) * Σ_p (I_t(p) - mu_t)^2
//   - Var_m = (1/|N|) * Σ_p (I_m(p) - mu_m)^2
// dLNCC/dalpha = (C' * Var_m - 0.5 * C * Var_m') / (Var_m * sqrt(Var_t * Var_m))
// Global NCC: same equations, but the sums are accumulated over the whole valid domain.
// TODO: write derivation of gradients.

import global_transformation;
import reduction_utils;
import texture_utils;
import coordinate_mapper;
import voxelscannermatrices;

extern static const uint32_t kWorkgroupSizeX = 8;
extern static const uint32_t kWorkgroupSizeY = 4;
extern static const uint32_t kWorkgroupSizeZ = 4;
// TODO: Use extern static const bool once the Slang compiler bug is fixed.
extern static const uint32_t kUseSourceMask;
extern static const uint32_t kUseTargetMask;
extern static const uint32_t kComputeGradients;
extern static const uint32_t kWindowRadius = 0;
static const uint32_t kWorkgroupInvocations = kWorkgroupSizeX * kWorkgroupSizeY * kWorkgroupSizeZ;
// For numerical stability in variance and denominator calculations
static const float kEpsVar = 1e-6F;
static const float kEpsDenom = 1e-6F;

struct LNCCParameters<let N : int> {
    float cost;
    Array<float, N> gradients;
};

struct LNCCReductionOP<let N : int> : IReduceOp<LNCCParameters<N>> {
    static LNCCParameters<N> identityElement() {
        var params : LNCCParameters<N>;
        params.cost = 0.0F;
        for (uint32_t i = 0U; i < N; ++i) {
            params.gradients[i] = 0.0F;
        }
        return params;
    }

    static LNCCParameters<N> reduce(LNCCParameters<N> a, LNCCParameters<N> b) {
        var params : LNCCParameters<N>;
        params.cost = a.cost + b.cost;
        for (uint32_t i = 0U; i < N; ++i) {
            params.gradients[i] = a.gradients[i] + b.gradients[i];
        }
        return params;
    }
};

struct GlobalNCCParameters<let N : int> {
    float sumTarget;
    float sumMoving;
    float sumTargetSquared;
    float sumMovingSquared;
    float sumTargetMoving;
    Array<float, N> sumTargetMovingPrime;
    Array<float, N> sumMovingPrime;
    Array<float, N> sumMovingSquaredPrime;
};

struct GlobalNCCReductionOP<let N : int> : IReduceOp<GlobalNCCParameters<N>> {
    static GlobalNCCParameters<N> identityElement() {
        var zeros : Array<float, N>;
        for (uint32_t i = 0U; i < N; ++i) { zeros[i] = 0.0F; }
        return GlobalNCCParameters<N>(0.0F, 0.0F, 0.0F, 0.0F, 0.0F, zeros, zeros, zeros);
    }

    static GlobalNCCParameters<N> reduce(GlobalNCCParameters<N> a, GlobalNCCParameters<N> b) {
        var tmPrime : Array<float, N>;
        var mPrime : Array<float, N>;
        var mmPrime : Array<float, N>;
        for (uint32_t i = 0U; i < N; ++i) {
            tmPrime[i] = a.sumTargetMovingPrime[i] + b.sumTargetMovingPrime[i];
            mPrime[i] = a.sumMovingPrime[i] + b.sumMovingPrime[i];
            mmPrime[i] = a.sumMovingSquaredPrime[i] + b.sumMovingSquaredPrime[i];
        }
        return GlobalNCCParameters<N>(a.sumTarget + b.sumTarget,
                                      a.sumMoving + b.sumMoving,
                                      a.sumTargetSquared + b.sumTargetSquared,
                                      a.sumMovingSquared + b.sumMovingSquared,
                                      a.sumTargetMoving + b.sumTargetMoving,
                                      tmPrime,
                                      mPrime,
                                      mmPrime);
    }
};

struct NCCUniforms<Transformation> where Transformation : ITransformation {
    uint32_t3 dispatchGrid;
    float3 transformationPivot;
    Transformation.TParams currentTransform;
    VoxelScannerMatrices voxelScannerMatrices;
};

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void lncc_main<Transformation>(
    uint32_t3 globalId : SV_DispatchThreadID,
    uint32_t3 localId : SV_GroupThreadID,
    uint32_t3 workgroupId : SV_GroupID,
    ConstantBuffer<NCCUniforms<Transformation>> uniforms,
    Texture3D<float> sourceImage,
    Texture3D<float> targetImage,
    Texture3D<float> sourceMask,
    Texture3D<float> targetMask,
    SamplerState linearSampler,
    RWStructuredBuffer<float> lnccPartials,
    RWStructuredBuffer<Atomic<uint32_t>> numContributingVoxels)
    where Transformation : ITransformation {
    static const uint32_t paramsCount = Transformation.kParamCount;
    typedef LNCCParameters<paramsCount> CurrentParams;
    static groupshared Array<CurrentParams, kWorkgroupInvocations> workgroupPartials;
    static groupshared Atomic<uint32_t> localValidCount;

    let localIndex = localId.x + localId.y * kWorkgroupSizeX + localId.z * kWorkgroupSizeX * kWorkgroupSizeY;
    if (localIndex == 0U) { localValidCount.store(0U); }
    GroupMemoryBarrierWithGroupSync();

    let radius = int32_t(kWindowRadius);
    var threadParams = LNCCReductionOP<paramsCount>::identityElement();
    if (radius > 0) {
        let sourceDim = textureSize(sourceImage);
        let targetDim = textureSize(targetImage);
        let coordMapper = CoordinateMapper(sourceDim, targetDim, uniforms.voxelScannerMatrices);
        let transformation = Transformation(uniforms.currentTransform, uniforms.transformationPivot);
        let sourceField = VoxelSamplingField(sourceImage, linearSampler);
        let targetField = VoxelSamplingField(targetImage, linearSampler);
        let sourceMaskField = VoxelSamplingField(sourceMask, linearSampler);
        let targetMaskField = VoxelSamplingField(targetMask, linearSampler);

        let center = int3(globalId);
        if (coordMapper.inTargetInt(center) && targetMaskField.maskAccepts(float3(center), kUseTargetMask != 0U)) {
            var sumTarget : float = 0.0F;
            var sumMoving : float = 0.0F;
            var sumTargetSquared : float = 0.0F;
            var sumMovingSquared : float = 0.0F;
            var sumTargetMoving : float = 0.0F;
            // NOTE: We treat valid samples as fixed, but the true derivative should include
            // boundary/mask discontinuities.
            var validSamples : float = 0.0F;
            var sumTargetMovingPrime : Array<float, paramsCount>;
            var sumMovingPrime : Array<float, paramsCount>;
            var sumMovingSquaredPrime : Array<float, paramsCount>;
            for (uint32_t i = 0U; i < paramsCount; ++i) {
                sumTargetMovingPrime[i] = 0.0F;
                sumMovingPrime[i] = 0.0F;
                sumMovingSquaredPrime[i] = 0.0F;
            }

            for (int32_t dz = -radius; dz <= radius; ++dz) {
                for (int32_t dy = -radius; dy <= radius; ++dy) {
                    for (int32_t dx = -radius; dx <= radius; ++dx) {
                        let neighbor = center + int3(dx, dy, dz);
                        if (!coordMapper.inTargetInt(neighbor)) {
                            continue;
                        }
                        let neighborFloat = float3(neighbor);
                        if (!targetMaskField.maskAccepts(neighborFloat, kUseTargetMask != 0U)) {
                            continue;
                        }

                        let sourceCoord = coordMapper.mapTargetVoxelToSource(neighborFloat, transformation);
                        if (!coordMapper.inSource(sourceCoord)) {
                            continue;
                        }
                        if (!sourceMaskField.maskAccepts(sourceCoord, kUseSourceMask != 0U)) {
                            continue;
                        }
                        let targetVal = targetField.sample(neighborFloat);
                        let movingVal = sourceField.sample(sourceCoord);

                        validSamples += 1.0F;
                        sumTarget += targetVal;
                        sumMoving += movingVal;
                        sumTargetSquared += targetVal * targetVal;
                        sumMovingSquared += movingVal * movingVal;
                        sumTargetMoving += targetVal * movingVal;

                        if (kComputeGradients != 0U) {
                            let gradVoxel = sourceField.spatialGradient(sourceCoord);
                            let gradScanner = coordMapper.mapVoxelGradientToScanner(gradVoxel);
                            let scannerCoord = coordMapper.mapTargetVoxelToScanner(neighborFloat);
                            for (uint32_t p = 0U; p < paramsCount; ++p) {
                                let imPrime = dot(gradScanner, transformation.jacobianVector(p, scannerCoord));
                                sumTargetMovingPrime[p] += targetVal * imPrime;
                                sumMovingPrime[p] += imPrime;
                                sumMovingSquaredPrime[p] += movingVal * imPrime;
                            }
                        }
                    }
                }
            }

            if (validSamples > 0.0F) {
                let invN = 1.0F / validSamples;
                let meanTarget = sumTarget * invN;
                let meanMoving = sumMoving * invN;
                let varianceTarget = max(0.0F, sumTargetSquared * invN - meanTarget * meanTarget);
                let varianceMoving = max(0.0F, sumMovingSquared * invN - meanMoving * meanMoving);
                if (varianceTarget > kEpsVar && varianceMoving > kEpsVar) {
                    let covariance = sumTargetMoving * invN - meanTarget * meanMoving;
                    let costDenom = sqrt(max(varianceTarget * varianceMoving, kEpsVar));
                    threadParams.cost = -covariance / costDenom;

                    if (kComputeGradients != 0U) {
                        let denomGradBase = max(varianceMoving * sqrt(max(varianceTarget * varianceMoving, kEpsVar)), kEpsDenom);
                        for (uint32_t p = 0U; p < paramsCount; ++p) {
                            let cPrime = (sumTargetMovingPrime[p] * invN) - (meanTarget * sumMovingPrime[p] * invN);
                            let varMovingPrime = 2.0F * (sumMovingSquaredPrime[p] * invN - meanMoving * sumMovingPrime[p] * invN);
                            let gradValue = (cPrime * varianceMoving - 0.5F * covariance * varMovingPrime) / denomGradBase;
                            threadParams.gradients[p] = -gradValue;
                        }
                    }

                    localValidCount.increment();
                }
            }
        }
    }

    workgroupPartials[localIndex] = threadParams;
    GroupMemoryBarrierWithGroupSync();
    let reduced = workgroupReduce<CurrentParams, LNCCReductionOP<paramsCount>, kWorkgroupInvocations>(workgroupPartials, localIndex);

    if (localIndex == 0U) {
        let wgIndex = workgroupId.x
                    + workgroupId.y * uniforms.dispatchGrid[0]
                    + workgroupId.z * uniforms.dispatchGrid[0] * uniforms.dispatchGrid[1];
        let base = wgIndex * (1 + paramsCount);
        lnccPartials[base] = reduced.cost;
        for (uint32_t i = 0U; i < paramsCount; ++i) {
            lnccPartials[base + 1 + i] = reduced.gradients[i];
        }
        numContributingVoxels[0].add(localValidCount.load());
    }
}

[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void global_ncc_main<Transformation>(
    uint32_t3 globalId : SV_DispatchThreadID,
    uint32_t3 localId : SV_GroupThreadID,
    uint32_t3 workgroupId : SV_GroupID,
    ConstantBuffer<NCCUniforms<Transformation>> uniforms,
    Texture3D<float> sourceImage,
    Texture3D<float> targetImage,
    Texture3D<float> sourceMask,
    Texture3D<float> targetMask,
    SamplerState linearSampler,
    RWStructuredBuffer<float> globalPartials,
    RWStructuredBuffer<Atomic<uint32_t>> numContributingVoxels)
    where Transformation : ITransformation {
    static const uint32_t paramsCount = Transformation.kParamCount;
    typedef GlobalNCCParameters<paramsCount> CurrentParams;
    static groupshared Array<CurrentParams, kWorkgroupInvocations> workgroupPartials;
    static groupshared Atomic<uint32_t> localValidCount;

    let localIndex = localId.x + localId.y * kWorkgroupSizeX + localId.z * kWorkgroupSizeX * kWorkgroupSizeY;
    if (localIndex == 0U) { localValidCount.store(0U); }
    GroupMemoryBarrierWithGroupSync();

    let sourceDim = textureSize(sourceImage);
    let targetDim = textureSize(targetImage);
    let coordMapper = CoordinateMapper(sourceDim, targetDim, uniforms.voxelScannerMatrices);
    let transformation = Transformation(uniforms.currentTransform, uniforms.transformationPivot);
    let sourceField = VoxelSamplingField(sourceImage, linearSampler);
    let targetField = VoxelSamplingField(targetImage, linearSampler);
    let sourceMaskField = VoxelSamplingField(sourceMask, linearSampler);
    let targetMaskField = VoxelSamplingField(targetMask, linearSampler);

    CurrentParams threadParams = GlobalNCCReductionOP<paramsCount>::identityElement();

    if (coordMapper.inTargetInt(int3(globalId)) && targetMaskField.maskAccepts(float3(globalId), kUseTargetMask != 0U)) {
        let sourceCoord = coordMapper.mapTargetVoxelToSource(float3(globalId), transformation);
        if (coordMapper.inSource(sourceCoord) && sourceMaskField.maskAccepts(sourceCoord, kUseSourceMask != 0U)) {
            let targetVal = targetField.sample(float3(globalId));
            let movingVal = sourceField.sample(sourceCoord);
            threadParams.sumTarget = targetVal;
            threadParams.sumMoving = movingVal;
            threadParams.sumTargetSquared = targetVal * targetVal;
            threadParams.sumMovingSquared = movingVal * movingVal;
            threadParams.sumTargetMoving = targetVal * movingVal;

            if (kComputeGradients != 0U) {
                let gradVoxel = sourceField.spatialGradient(sourceCoord);
                let gradScanner = coordMapper.mapVoxelGradientToScanner(gradVoxel);
                let scannerCoord = coordMapper.mapTargetVoxelToScanner(float3(globalId));
                for (uint32_t p = 0U; p < paramsCount; ++p) {
                    let imPrime = dot(gradScanner, transformation.jacobianVector(p, scannerCoord));
                    threadParams.sumTargetMovingPrime[p] = targetVal * imPrime;
                    threadParams.sumMovingPrime[p] = imPrime;
                    threadParams.sumMovingSquaredPrime[p] = movingVal * imPrime;
                }
            }

            localValidCount.increment();
        }
    }

    workgroupPartials[localIndex] = threadParams;
    GroupMemoryBarrierWithGroupSync();
    let reduced = workgroupReduce<CurrentParams, GlobalNCCReductionOP<paramsCount>, kWorkgroupInvocations>(workgroupPartials, localIndex);

    if (localIndex == 0U) {
        let wgIndex = workgroupId.x
                    + workgroupId.y * uniforms.dispatchGrid[0]
                    + workgroupId.z * uniforms.dispatchGrid[0] * uniforms.dispatchGrid[1];
        let base = wgIndex * (5 + 3 * paramsCount);
        globalPartials[base + 0] = reduced.sumTarget;
        globalPartials[base + 1] = reduced.sumMoving;
        globalPartials[base + 2] = reduced.sumTargetSquared;
        globalPartials[base + 3] = reduced.sumMovingSquared;
        globalPartials[base + 4] = reduced.sumTargetMoving;

        var offset : uint32_t = 5U;
        for (uint32_t i = 0U; i < paramsCount; ++i) { globalPartials[base + offset + i] = reduced.sumTargetMovingPrime[i]; }
        offset += paramsCount;
        for (uint32_t i = 0U; i < paramsCount; ++i) { globalPartials[base + offset + i] = reduced.sumMovingPrime[i]; }
        offset += paramsCount;
        for (uint32_t i = 0U; i < paramsCount; ++i) { globalPartials[base + offset + i] = reduced.sumMovingSquaredPrime[i]; }

        numContributingVoxels[0].add(localValidCount.load());
    }
}
