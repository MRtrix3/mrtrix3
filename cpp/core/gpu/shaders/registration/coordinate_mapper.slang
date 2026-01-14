import global_transformation;
import voxelscannermatrices;

// A helper class to map coordinates between source and target spaces.
// It uses the voxel to scanner space and inverse matrices to perform the mapping.
struct CoordinateMapper {
    uint3 _sourceDim;
    uint3 _targetDim;
    VoxelScannerMatrices _vsm;
    float3 _sourceCentreVoxelCoord;
    float3 _sourceCentreScanner;

    __init(uint3 sourceDim, uint3 targetDim, in VoxelScannerMatrices vsm)
     {
        this._sourceDim = sourceDim;
        this._targetDim = targetDim;
        this._vsm = vsm;
        this._sourceCentreVoxelCoord = float3(sourceDim) * 0.5F;
        this._sourceCentreScanner = mul(vsm.sourceVoxelToScanner, float4(this._sourceCentreVoxelCoord, 1.0F)).xyz;
     }

    // Maps a voxel coordinate in the source space to target space
    __generic <Transformation : ITransformation>
    float3 mapSourceVoxelToTarget(float3 sourceVoxelCoord, Transformation transformation)
    {
        float4 sourceScannerCoord = mul(_vsm.sourceVoxelToScanner, float4(sourceVoxelCoord, 1.0F));
        float3 targetScannerCoord = transformation.apply(sourceScannerCoord.xyz);
        return mul(_vsm.targetScannerToVoxel, float4(targetScannerCoord, 1.0F)).xyz;
    }

    // Maps a voxel coordinate in the target space to source space
    __generic <Transformation : ITransformation>
    float3 mapTargetVoxelToSource(float3 targetVoxelCoord, Transformation transformation)
    {
        float4 targetScannerCoord = mul(_vsm.targetVoxelToScanner, float4(targetVoxelCoord, 1.0F));
        float3 sourceScannerCoord = transformation.apply(targetScannerCoord.xyz);
        return mul(_vsm.sourceScannerToVoxel, float4(sourceScannerCoord, 1.0F)).xyz;
    }


    // Maps a voxel coordinate in the target space to scanner space
    float3 mapTargetVoxelToScanner(float3 targetVoxelCoord)
    {
        return mul(_vsm.targetVoxelToScanner, float4(targetVoxelCoord, 1.0F)).xyz;
    }

    // Maps a voxel gradient in source space to scanner space
    // NOTE: the gradient vector is covariant. So if coordinates transform as v' = M * v,
    // the gradient transforms as g' = g * inverse(M). Since sourceScannerToVoxel is the inverse
    // of sourceVoxelToScanner, this function maps a voxel gradient in source space to scanner space.
    // Note that in Slang the notation mul(vec, matrix) is equivalent to matrix^T * vec.
    float3 mapVoxelGradientToScanner(float3 voxelGradient)
    {
        return mul(float4(voxelGradient, 0.0F), _vsm.sourceScannerToVoxel).xyz;
    }
    

    // Returns the scanner Jacobian at a given scanner coordinate
    float3 sourceCentreScanner()
    {
        return _sourceCentreScanner;
    }

    // Checks if a coordinate is within the source dimensions
    bool inSourceInt(int32_t3 coord)
    {
        return all(coord >= int32_t3(0, 0, 0)) && all(coord < int32_t3(_sourceDim));
    }

    // Checks if a coordinate is within the target dimensions
    bool inTargetInt(int32_t3 coord)
    {
        return all(coord >= int32_t3(0, 0, 0)) && all(coord < int32_t3(_targetDim));
    }

    // Checks if a coordinate is within the source dimensions
    bool inSource(float3 coord)
    {
        return all(coord >= float3(0.0F, 0.0F, 0.0F)) && all(coord < float3(_sourceDim));
    }

    // Checks if a coordinate is within the target dimensions
    bool inTarget(float3 coord)
    {
        return all(coord >= float3(0.0F, 0.0F, 0.0F)) && all(coord < float3(_targetDim));
    }
};
