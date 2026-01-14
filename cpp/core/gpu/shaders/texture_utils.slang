module texture_utils;

public uint3 textureSize<T>(Texture3D<T> tex) where T : ITexelElement {
    uint3 size;
    tex.GetDimensions(size.x, size.y, size.z);
    return size;
}

public uint3 textureSize<T>(RWTexture3D<T> tex) where T : ITexelElement {
    uint3 size;
    tex.GetDimensions(size.x, size.y, size.z);
    return size;
}

public uint3 textureSize<T>(WTexture3D<T> tex) where T : ITexelElement {
    uint3 size;
    tex.GetDimensions(size.x, size.y, size.z);
    return size;
}


// Defines different coordinate systems for 3D volume sampling.
// The underlying texture sampler always expects normalized coordinates [0,1],
// so each coordinate system applies the appropriate transformation before sampling.
public enum SamplingCoordinateSystem {
    // Voxel-centered coordinate system.
    // In this system, voxel centers are located at integer positions (i, j, k).
    // When sampling, coordinates are shifted by +0.5 before normalization.
    // GPU samplers expect coordinates where the center of a texel
    // is at (i+0.5, j+0.5), so adding 0.5 aligns it correctly.
    Voxel,
    // Texel-centered coordinate system.
    // In this system, voxel corners are at integer positions, meaning voxel centers
    // are at half-integer positions (i+0.5, j+0.5, k+0.5).
    // Coordinates are used directly without shifting during sampling.
    Texel
}

public typealias TexelSamplingField = VolumeSamplingField<SamplingCoordinateSystem.Texel>;
public typealias VoxelSamplingField = VolumeSamplingField<SamplingCoordinateSystem.Voxel>;


// A convenient class for sampling and gradient computation on 3D images.
public struct VolumeSamplingField<let coordSys: SamplingCoordinateSystem> {
    Texture3D<float> image;
    uint3 dimensions;
    SamplerState sampler;

    public __init(Texture3D<float> img, SamplerState smpl) {
        image = img;
        sampler = smpl;
        image.GetDimensions(dimensions.x, dimensions.y, dimensions.z);
    }

    public float sample(float3 coord) {
        if (coordSys == SamplingCoordinateSystem.Voxel) {
            return image.SampleLevel(sampler, (coord + 0.5F) / float3(dimensions), 0.0F);
        } else if (coordSys == SamplingCoordinateSystem.Texel) {
            return image.SampleLevel(sampler, coord / float3(dimensions), 0.0F);
        }
        return 0.0F;
    }

    public bool maskAccepts(float3 coord, bool useMask) {
        if (!useMask) { return true; }
        return sample(coord) >= 0.5F;
    }

    public float3 spatialGradient(float3 voxelCoord) {
        let offsetX = float3(1.0F, 0.0F, 0.0F);
        let offsetY = float3(0.0F, 1.0F, 0.0F);
        let offsetZ = float3(0.0F, 0.0F, 1.0F);

        let gradX = (sample(voxelCoord + offsetX) - sample(voxelCoord - offsetX)) * 0.5f;
        let gradY = (sample(voxelCoord + offsetY) - sample(voxelCoord - offsetY)) * 0.5f;
        let gradZ = (sample(voxelCoord + offsetZ) - sample(voxelCoord - offsetZ)) * 0.5f;

        return float3(gradX, gradY, gradZ);
    }

    public float3 spatialGradientTrilinearAnalytic(float3 coord) {
        float3 localCoord = coord;
        if (coordSys == SamplingCoordinateSystem.Texel) {
            localCoord = coord - 0.5F;
        }

        // Clamp localCoord so i0 >= 0 and i0+1 stays in-bounds
        int3 i0 = int3(floor(localCoord));
        i0 = max(i0, int3(0, 0, 0));
        let i1 : int3 = min(i0 + 1, int3(dimensions) - 1);
        let t : float3 = localCoord - float3(i0);

        // Fetch the 8 cell corners
        let v000 = image.Load(int4(i0, 0)).r;
        let v100 = image.Load(int4(int3(i1.x, i0.y, i0.z),0)).r;
        let v010 = image.Load(int4(int3(i0.x, i1.y, i0.z),0)).r;
        let v110 = image.Load(int4(int3(i1.x, i1.y, i0.z),0)).r;
        let v001 = image.Load(int4(int3(i0.x, i0.y, i1.z),0)).r;
        let v101 = image.Load(int4(int3(i1.x, i0.y, i1.z),0)).r;
        let v011 = image.Load(int4(int3(i0.x, i1.y, i1.z),0)).r;
        let v111 = image.Load(int4(int3(i1.x, i1.y, i1.z),0)).r;

        // Analytic partials of the trilinear form
        let ddx0 = lerp(v100 - v000, v110 - v010, t.y);
        let ddx1 = lerp(v101 - v001, v111 - v011, t.y);
        let ddx  = lerp(ddx0, ddx1, t.z);

        let ddy0 = lerp(v010 - v000, v110 - v100, t.x);
        let ddy1 = lerp(v011 - v001, v111 - v101, t.x);
        let ddy  = lerp(ddy0, ddy1, t.z);

        let ddz0 = lerp(v001 - v000, v101 - v100, t.x);
        let ddz1 = lerp(v011 - v010, v111 - v110, t.x);
        let ddz  = lerp(ddz0, ddz1, t.y);

        return float3(ddx, ddy, ddz);
    }
}


public struct GradientSamplingField {
    Texture3D<float4> gradientImage;
    uint3 dimensions;
    SamplerState sampler;

    public __init(Texture3D<float4> img, SamplerState smpl) {
        gradientImage = img;
        sampler = smpl;
        gradientImage.GetDimensions(dimensions.x, dimensions.y, dimensions.z);
    }

    public float3 sample(float3 coord) {
        return gradientImage.SampleLevel(sampler, coord / float3(dimensions), 0.0F).rgb;
    }
}
