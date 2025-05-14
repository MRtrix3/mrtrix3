const workgroupSize = vec3<u32>({{workgroup_size}});
const workgroupInvocations = workgroupSize.x * workgroupSize.y * workgroupSize.z;

// Linear transformation matrix from output to input
@group(0) @binding(0) var<storage, read> transformationMat: mat4x4<f32>;
@group(0) @binding(1) var inputImage: texture_3d<f32>;
@group(0) @binding(2) var outputImage: texture_storage_3d<r32float, write>;
@group(0) @binding(3) var linearSampler: sampler;

@compute @workgroup_size(workgroupSize.x, workgroupSize.y, workgroupSize.z)
fn main(@builtin(global_invocation_id) globalId: vec3<u32>)
{
    let inputDims = textureDimensions(inputImage);
    let outputDims = textureDimensions(outputImage);

    if (globalId.x >= outputDims.x || globalId.y >= outputDims.y || globalId.z >= outputDims.z) {
        return;
    }

    let dstVoxel = vec3<f32>(globalId);
    let transformedVoxel4 = transformationMat * vec4<f32>(dstVoxel, 1.0);
    let transformedVoxel  = transformedVoxel4.xyz / transformedVoxel4.w;

    let inside = all(transformedVoxel >= vec3<f32>(0.0)) &&
                 all(transformedVoxel <  vec3<f32>(inputDims));

    var outputValue = vec4<f32>();
    if(inside) {
        let sampleCoord = (transformedVoxel + 0.5)/ vec3<f32>(inputDims);
        outputValue = textureSampleLevel(inputImage, linearSampler, sampleCoord, 0.0);
    }
    textureStore(outputImage, globalId, outputValue);
}
