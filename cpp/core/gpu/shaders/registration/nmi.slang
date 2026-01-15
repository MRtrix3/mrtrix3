// Normalised Mutual Information (NMI) = (H_t + H_m) / H_j
// where
//   - H_j = -Σ_ij p_ij * log(p_ij) is the joint entropy.
//   - H_t = -Σ_i p_i * log(p_i) is the target marginal entropy.
//   - H_m = -Σ_j p_j * log(p_j) is the moving marginal entropy.
//
// Joint / marginal probabilities and normalisation:
//   - H_ij = Σ_v h_ij(v) is joint histogram counts
//   - Z = Σ_ab H_{ab} is total mass
//   - p_ij = H_ij / Z
//   - p_i = Σ_j p_ij,   p_j = Σ_i p_ij
// where v is the voxel index.
//
// Using the quotient rule for NMI = (H_t + H_m) / H_j, after some tedious algebra (using dp_i/dq = Σ_ij dp_ij/dq),
// you can show that:
//   dNMI/dq = 1/(H_j*Z) Σ_ij (dH_ij/dq * [NMI * log p_ij - log p_i - log p_j])
// The fact that the term in square brackets is independent of the voxel index v is used in this
// shader to precompute it once per-bin to avoid redundant calculations.
//
// To calculate dH_ij/dq, we first note that the parzen-window per-voxel contribution:
//   h_ij(v) = B(b_t(v) - i) * B(b_m(v) - j)
// where
//   - b_t(v) maps the target intensity at voxel v into bin-space (target bins indexed by i)
//   - b_m(v) maps the (transformed) moving intensity at voxel v into bin-space (moving/source bins indexed by j)
//   - B(x) is the cubic B-spline kernel and B'(x) = dB/dx.
//
// Only the moving image depends on the transform parameters q. Therefore:
//   dH_ij/dq = Σ_v d h_ij(v)/dq
// and
//   d h_ij(v)/dq = B( b_t(v) - i ) * B'( b_m(v) - j ) * d b_m(v)/dq
// Since b_m depends linearly on the moving image intensity I_m(x'):
//   d b_m / dq = (d b_m / d I_m) * d I_m(x')/dq
// and by the chain rule
//   d I_m(x')/dq = grad I_m(x') * d x'/dq
// where grad I_m is the image gradient in scanner coordinates at x'.

import coordinate_mapper;
import cubic_bspline;
import global_transformation;
import reduction_utils;
import texture_utils;
import parzen_binner;
import voxelscannermatrices;

extern static const int32_t kNumBins;
// TODO: Use extern static const bool once the Slang compiler bug is fixed.
extern static const uint32_t kUseTargetMask;
extern static const uint32_t kUseSourceMask;

static const float divisionEpsilon = 1e-7F; // To avoid divisions by zero
static const float logEpsilon = 1e-9F; // To avoid log(0)

static const uint32_t kPrecomputeWorkgroupSizeX = 16;
static const uint32_t kPrecomputeWorkgroupSizeY = 16;
static const uint32_t kPrecomputeWorkgroupInvocations = kPrecomputeWorkgroupSizeX * kPrecomputeWorkgroupSizeY;

// Computes marginals p_i, p_j, entropies H_t, H_m, H_j, the scalar NMI
// value, and a per-bin coefficient table coefficientsTable[i*kNumBins + j] = dNMI/dp_ij.
// NOTE: this entry point MUST be dispatched with a single workgroup.
[shader("compute")]
[numthreads(kPrecomputeWorkgroupSizeX, kPrecomputeWorkgroupSizeY, 1)]
void precompute(
    uint32_t3 globalId: SV_DispatchThreadID,
    uint32_t3 localId: SV_GroupThreadID,
    // Raw (unnormalised) joint histogram H and scalar total mass Z
    StructuredBuffer<float> jointHistogram,
    StructuredBuffer<float> jointHistogramMass,
    // coefficientsTable[i*kNumBins + j] = ( nmi*log p_ij - log p_i - log p_j ) / (H_j * Z)
    RWStructuredBuffer<float> coefficientsTable,
    RWStructuredBuffer<float> mutualInformation)
{
    static groupshared Array<float, kNumBins> pTarget;
    static groupshared Array<float, kNumBins> pMoving;
    static groupshared Array<float, kNumBins> logP_row; // log p_i (row / target marginal)
    static groupshared Array<float, kNumBins> logP_col; // log p_j (column / moving marginal)
    static groupshared Array<float3, kPrecomputeWorkgroupInvocations> workgroupEntropies; // H_j, H_t, H_m

    let localIndex = localId.y * kPrecomputeWorkgroupSizeX + localId.x;
    // We iterate over the histogram in a strided manner so that each thread
    // is responsible for accumulating a single row/column of the joint histogram.
    // Each thread will process kNumBins/kPrecomputeWorkgroupSizeY (for p_i) rows or
    // kNumBins/kPrecomputeWorkgroupSizeX (for p_j) columns.
    // p_i = Σ_j p_ij
    // Each thread sums its own row of the joint histogram.
    if (localId.x == 0) {
        for (uint32_t i = localId.y; i < kNumBins; i += kPrecomputeWorkgroupSizeY) {
            // guard in case kNumBins < workgroup size or not divisible by stride
            if (i >= kNumBins) break;
            var rowSum = 0.0F;
            for (uint32_t j = 0; j < kNumBins; ++j) {
                rowSum += jointHistogram[i * kNumBins + j];
            }
            pTarget[i] = rowSum; // H_i
        }
    }

    // p_j = Σ_i p_ij
    // Each thread sums its own column of the joint histogram.
    if(localId.y == 0) {
        for(uint32_t j = localId.x; j < kNumBins; j += kPrecomputeWorkgroupSizeX) {
            // guard in case kNumBins < workgroup size or not divisible by stride
            if (j >= kNumBins) break;
            var colSum = 0.0F;
            for(uint32_t i = 0; i < kNumBins; ++i) {
                colSum += jointHistogram[i*kNumBins + j];
            }
            pMoving[j] = colSum; // H_j
        }
    }

    GroupMemoryBarrierWithGroupSync();


    var entropiesVec = float3(0.0F, 0.0F, 0.0F); // H_j, H_t, H_m
    var localHJ = 0.0F;
    var localHT = 0.0F;
    var localHM = 0.0F;
    let Z = max(jointHistogramMass[0], 0.0F);

    // Accumulate joint entropy H_j and H_t contributions
    for (uint32_t i = localId.y; i < kNumBins; i += kPrecomputeWorkgroupSizeY) {
        if (i >= kNumBins) break;
        if (localId.x == 0) {
            let Hi = max(pTarget[i], 0.0F);
            let pi = Hi / max(Z, divisionEpsilon);
            entropiesVec.y += -pi * log(max(pi, logEpsilon));
        }
        for(uint32_t j = localId.x; j < kNumBins; j += kPrecomputeWorkgroupSizeX) {
            if (j >= kNumBins) break;
            let index = i * kNumBins + j;
            let Hij = max(jointHistogram[index], 0.0F);
            let p_ij = Hij / max(Z, divisionEpsilon);
            entropiesVec.x += -p_ij * log(max(p_ij, logEpsilon));
        }
    }

    // Accumulate H_m contributions (strided over columns)
    if (localId.y == 0) {
        for (uint32_t j = localId.x; j < kNumBins; j += kPrecomputeWorkgroupSizeX) {
            if (j >= kNumBins) break;
            let Hj = max(pMoving[j], 0.0F);
            let pj = Hj / max(Z, divisionEpsilon);
            entropiesVec.z += -pj * log(max(pj, logEpsilon));
        }
    }

    workgroupEntropies[localIndex] = entropiesVec;
    // Reduce H_j, H_t, H_m across the workgroup
    GroupMemoryBarrierWithGroupSync();

    let entropies = workgroupReduce<float3, SumOp<float3>, kPrecomputeWorkgroupInvocations>(workgroupEntropies, localIndex);
    let H_j = entropies.x;
    let H_t = entropies.y;
    let H_m = entropies.z;

    if(localId.x == 0 && localId.y == 0) {
        let denom = max(H_j, divisionEpsilon);
        mutualInformation[0] = (H_t + H_m) / denom;
    }

    GroupMemoryBarrierWithGroupSync();

    // Compute per-bin coefficient table coefficientsTable[index] = (nmi*log p_ij - log p_i - log p_j) / (H_j * Z)
    let nmi = (H_t + H_m) / max(H_j, divisionEpsilon);
    let Zsafe = max(Z, divisionEpsilon);
    let denom = H_j * Zsafe;

    var invNorm = denom > divisionEpsilon ? 1.0F / denom : 0.0F;

    // Precompute log p_i and log p_j into shared memory to avoid recomputing inside inner loop
    // Each thread along the x-axis of the workgroup computes log p_j
    // Each thread along the y-axis of the workgroup computes log p_i

    if(localId.x == 0) {
        for (uint32_t i = localId.y; i < kNumBins; i += kPrecomputeWorkgroupSizeY) {
            if (i >= kNumBins) break;
            let pi = max(pTarget[i] / Zsafe, 0.0F);
            logP_row[i] = log(max(pi, logEpsilon));
        }
    }

    if(localId.y == 0) {
        for (uint32_t j = localId.x; j < kNumBins; j += kPrecomputeWorkgroupSizeX) {
            if (j >= kNumBins) break;
            let pj = max(pMoving[j] / Zsafe, 0.0F);
            logP_col[j] = log(max(pj, logEpsilon));
        }
    }

    GroupMemoryBarrierWithGroupSync();

    for(uint32_t i = localId.y; i < kNumBins; i += kPrecomputeWorkgroupSizeY) {
        if (i >= kNumBins) break;
        for(uint32_t j = localId.x; j < kNumBins; j += kPrecomputeWorkgroupSizeX) {
            if (j >= kNumBins) break;
            let index = i * kNumBins + j;
            let Hij = max(jointHistogram[index], 0.0F);
            let p_ij = max(Hij / Zsafe, 0.0F);
            let logHij = log(max(p_ij, logEpsilon));
            let val = nmi * logHij - logP_row[i] - logP_col[j];
            coefficientsTable[index] = invNorm * val;
        }
    }
}

extern static const uint32_t kWorkgroupSizeX = 8;
extern static const uint32_t kWorkgroupSizeY = 8;
extern static const uint32_t kWorkgroupSizeZ = 8;
static const uint32_t kWorkgroupInvocations = kWorkgroupSizeX * kWorkgroupSizeY * kWorkgroupSizeZ;
static const uint32_t kMinSubgroupSize = 4;

typealias MIGradients<let N : uint32_t> = Array<float, N>;

struct Intensities
{
    float sourceMin;
    float sourceMax;
    float targetMin;
    float targetMax;
}

struct Uniforms<Transformation> where Transformation : ITransformation{
    uint32_t3 dispatchGrid;
    float3 transformationPivot;
    Intensities intensities;
    Transformation.TParams currentTransform;
    VoxelScannerMatrices voxelScannerMatrices;
};


[shader("compute")]
[numthreads(kWorkgroupSizeX, kWorkgroupSizeY, kWorkgroupSizeZ)]
void main<Transformation>(
    uint32_t3 globalId: SV_DispatchThreadID,
    uint32_t3 localId: SV_GroupThreadID,
    uint32_t3 workgroupId: SV_GroupID,
    ConstantBuffer<Uniforms<Transformation>> uniforms,
    Texture3D<float> sourceTexture,
    Texture3D<float> targetTexture,
    Texture3D<float> sourceMaskTexture,
    Texture3D<float> targetMaskTexture,
    SamplerState sampler,
    StructuredBuffer<float> coefficientsTable,
    RWStructuredBuffer<MIGradients<Transformation.kParamCount>> partialSumsGradients)
    where Transformation : ITransformation
{
    typealias Gradients = MIGradients<Transformation.kParamCount>;
    // Wave-partial gradient sums: one Gradients vector per possible wave in the WG.
    static const uint32_t kWaveMax = kWorkgroupInvocations / kMinSubgroupSize;
    static groupshared Array<Gradients, kWaveMax> wavePartials;
    typealias Binner = ParzenBinner<kNumBins, 2>;

    let sourceDimensions = textureSize(sourceTexture);
    let targetDimensions = textureSize(targetTexture);
    let coordMapper = CoordinateMapper(
        uint3(sourceDimensions), uint3(targetDimensions), uniforms.voxelScannerMatrices
    );

    let transformation = Transformation(uniforms.currentTransform, uniforms.transformationPivot);
    let sourceTextureField = VoxelSamplingField(sourceTexture, sampler);
    let sourceMaskField = VoxelSamplingField(sourceMaskTexture, sampler);
    let targetMaskField = VoxelSamplingField(targetMaskTexture, sampler);
    let effectiveRangeBins = Binner.effectiveRangeBins();

    let localIndex = localId.x + localId.y * kWorkgroupSizeX + localId.z * kWorkgroupSizeX * kWorkgroupSizeY;

    // Thread-local gradient accumulator kept in registers; must be defined for all threads
    var gradients: Gradients;
    for (uint32_t q = 0U; q < Transformation.kParamCount; ++q) { gradients[q] = 0.0F; }

    bool includeVoxel = coordMapper.inTargetInt(int3(globalId));
    float3 targetVoxelCoord = float3(globalId);
    float3 movingVoxelCoord = 0.0F;

    if (includeVoxel && targetMaskField.maskAccepts(targetVoxelCoord, kUseTargetMask != 0U)) {
        movingVoxelCoord = coordMapper.mapTargetVoxelToSource(targetVoxelCoord, transformation);
        if (!coordMapper.inSource(movingVoxelCoord) || !sourceMaskField.maskAccepts(movingVoxelCoord, kUseSourceMask != 0U)) {
            includeVoxel = false;
        }
    } else {
        includeVoxel = false;
    }

    if (includeVoxel) {
        let targetIntensity = targetTexture.Load(int4(globalId, 0)).r;
        let gradMovingVoxel = sourceTextureField.spatialGradient(movingVoxelCoord);
        let gradMovingScanner = coordMapper.mapVoxelGradientToScanner(gradMovingVoxel);
        let targetScannerCoord = coordMapper.mapTargetVoxelToScanner(targetVoxelCoord);

        let targetMin = uniforms.intensities.targetMin;
        let targetMax = uniforms.intensities.targetMax;
        let sourceMin = uniforms.intensities.sourceMin;
        let sourceMax = uniforms.intensities.sourceMax;

        let movingIntensity = sourceTextureField.sample(movingVoxelCoord);

        let targetBin = Binner.mapIntensityToBin(targetIntensity, targetMin, targetMax);
        let sourceBin = Binner.mapIntensityToBin(movingIntensity, sourceMin, sourceMax);

        uint32_t targetBinStart; uint32_t targetBinEnd;
        Binner.computeBinNeighbourhood(targetBin, targetBinStart, targetBinEnd);
        uint32_t sourceBinStart; uint32_t sourceBinEnd;
        Binner.computeBinNeighbourhood(sourceBin, sourceBinStart, sourceBinEnd);

        // Precompute B-spline weights and derivatives for available bins
        var targetWeights: float4;
        var sourceDerivWeights: float4;
        var targetCount: uint32_t;
        var sourceCount: uint32_t;

        Binner.computeWeights(targetBin, targetBinStart, targetBinEnd, targetWeights, targetCount);
        Binner.computeDerivatives(sourceBin, sourceBinStart, sourceBinEnd, sourceDerivWeights, sourceCount);

        // Derivative: dNMI/dq = Σ_ij C_ij * B_t(i) * B'_s(j) * (d b_m / d I_m) * (d I_m / d q)
        // Since b_m(I_m) = padding + effectiveRangeBins * (I_m - sourceMin) / (sourceMax - sourceMin)
        // d b_m / d I_m = effectiveRangeBins / (sourceMax - sourceMin)
        let dBin_dIm = effectiveRangeBins / max(sourceMax - sourceMin, divisionEpsilon);
        var accumulatedContribution = 0.0F; // C_ij * B_t(i) * B'_s(j)

        for (uint32_t tIdx = 0U; tIdx < targetCount; ++tIdx) {
            let rowIndex = (targetBinStart + tIdx) * kNumBins;
            let targetWeight = targetWeights[tIdx];
            let baseIndex = rowIndex + sourceBinStart;

            var coefficientsVector : float4 = 0.0F;
            [unroll]
            for (uint32_t off = 0U; off < 4U; ++off) {
                uint32_t col = sourceBinStart + off;
                if (col < kNumBins) {
                    coefficientsVector[off] = coefficientsTable[rowIndex + col];
                }
            }

            accumulatedContribution += targetWeight * dot(coefficientsVector, sourceDerivWeights);
        }

        let factor : float = accumulatedContribution * dBin_dIm;
        // Early-out if no contribution
        if (abs(factor) > 0.0F) {
            // Compute per-parameter image intensity Jacobian dIm/dq: reuse jacobian vectors
            // to avoid recomputing inside both gradient and Hessian paths.
            var dImdq: Gradients;
            [ForceUnroll]
            for (uint32_t q = 0U; q < Transformation.kParamCount; ++q) {
                let jvec = transformation.jacobianVector(q, targetScannerCoord);
                dImdq[q] = dot(gradMovingScanner, jvec);
            }

            for (uint32_t q = 0U; q < Transformation.kParamCount; ++q) { gradients[q] = factor * dImdq[q]; }
        }
    }

    // Wave-level reduction for gradients
    let waveSize = WaveGetLaneCount();
    let lane = WaveGetLaneIndex();
    [unroll]
    for (uint32_t q = 0U; q < Transformation.kParamCount; ++q) {
        let sum = WaveActiveSum(gradients[q]);
        if (lane == 0) {
            let waveIndex = localIndex / waveSize;
            if (waveIndex < kWaveMax) { wavePartials[waveIndex][q] += sum; }
        }
    }

    GroupMemoryBarrierWithGroupSync();

    let workgroupGradients = workgroupReduce<Gradients, SumArrayOp<float, Transformation.kParamCount>, kWaveMax>(wavePartials, localIndex);

    if (localIndex == 0U) {
        let wgIndex = workgroupId.x
                    + workgroupId.y * uniforms.dispatchGrid[0]
                    + workgroupId.z * uniforms.dispatchGrid[0] * uniforms.dispatchGrid[1];
        partialSumsGradients[wgIndex] = workgroupGradients;
    }
}
