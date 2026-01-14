module GlobalTransformation;

public struct RigidTransformationParameters
{
    public float tx, ty, tz; // Translation
    public float rx, ry, rz; // Axis angle parameterisation
}

public struct AffineTransformationParameters
{
    public float tx, ty, tz;    // Translation
    public float rx, ry, rz;    // Axis angle parameterisation
    public float sx, sy, sz;    // Scaling
    public float shx, shy, shz; // Shearing
}

public interface ITransformation
{
    associatedtype TParams;
    associatedtype Jacobian : IArray<float3>;
    static const uint32_t kParamCount;

    __init(TParams params, float3 pivot);
    float3 apply(float3 coord);
    float4x4 matrix();
    Jacobian jacobian(float3 coord);
    // Returns the i-th column of the Jacobian matrix at the given coordinate
    // Using this function can avoid constructing the full Jacobian array which 
    // sometimes lead to register spilling in a thread hurting performance.
    float3 jacobianVector(uint32_t index, float3 coord);
}

static const float kEps = 1e-5F;

// Computes the rotation matrix from the exponential map using Rodrigues’ formula.
// See https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula
// Let v = (rx,ry,rz), θ = |v|. Then
//   R = I + A * K + B * K^2,
// where
//   A = sin(θ)/θ,    B = (1-cos(θ))/(θ^2),
// and K is the skew-symmetric matrix of v.
// Note that the original Rodrigues’ formula is given as R = I + sin(θ)*K + (1-cos(θ))*K^2.
// but given v = (rx, ry, rz), θ = |v|, the unit axis is u = v/θ and K(u) = K(v/θ) = K(v)/θ.
public float3x3 rotationMatrix(float3 rotationVector)
{
    let theta = length(rotationVector);
    float A, B;
    if (theta < kEps)
    {
        A = 1.0f - theta * theta / 6.0f;
        B = 0.5f - theta * theta / 24.0f;
    }
    else
    {
        A = sin(theta) / theta;
        B = (1.0f - cos(theta)) / (theta * theta);
    }

    let K = float3x3(0.0F, -rotationVector.z, rotationVector.y,
                     rotationVector.z, 0.0F, -rotationVector.x,
                     -rotationVector.y, rotationVector.x, 0.0F);

    let K2 = mul(K, K);
    let I = float3x3(1.0F, 0.0F, 0.0F,
                     0.0F, 1.0F, 0.0F,
                     0.0F, 0.0F, 1.0F);
    return I + A * K + B * K2;
}

// Computes the derivative of the rotation matrix with respect to one of the three
// exponential map parameters.  That is, for a given component (0 for rx, 1 for ry, 2 for rz),
// it returns dR/dv_i where R = I + A*K + B*K^2 and
//   A = sin(θ)/θ,  B = (1-cos(θ))/(θ^2),
// and K is the skew-symmetric matrix of v = (rx,ry,rz) with θ = |v|.
// dR/dv_i = dA/dv_i * K + A * dK/dv_i + dB/dv_i * K^2 + B * d(K^2)/dv_i.
public float3x3 rotationMatrixDerivative(float3 rotationVector, uint32_t component)
{
    let theta = length(rotationVector);
    float A, B, dA_dtheta, dB_dtheta;
    if (theta < kEps)
    {
        A = 1.0F - theta * theta / 6.0F;
        B = 0.5F - theta * theta / 24.0F;
        dA_dtheta = -theta / 3.0F;
        dB_dtheta = -theta / 12.0F;
    }
    else
    {
        A = sin(theta) / theta;
        B = (1.0f - cos(theta)) / (theta * theta);
        dA_dtheta = (theta * cos(theta) - sin(theta)) / (theta * theta);
        dB_dtheta = (theta * sin(theta) - 2.0f * (1.0f - cos(theta))) / (theta * theta * theta);
    }
    let K = float3x3(0.0F, -rotationVector.z, rotationVector.y,
                     rotationVector.z, 0.0F, -rotationVector.x,
                     -rotationVector.y, rotationVector.x, 0.0F);
    let K2 = mul(K, K);
    float3x3 dK = float3x3(0);
    if (component == 0)
        dK = float3x3(0.0F, 0.0F, 0.0F, 0.0F, 0.0F, -1.0F, 0.0F, 1.0F, 0.0f);
    else if (component == 1)
        dK = float3x3(0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, -1.0F, 0.0F, 0.0f);
    else if (component == 2)
        dK = float3x3(0.0F, -1.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0f);
    let dK2 = mul(dK, K) + mul(K, dK);
    let vi = rotationVector[component];
    let dA_dvi = (theta < kEps) ? -vi / 3.0f : dA_dtheta * (vi / theta);
    let dB_dvi = (theta < kEps) ? -vi / 12.0f : dB_dtheta * (vi / theta);
    return dA_dvi * K + A * dK + dB_dvi * K2 + B * dK2;
}

// Matrix layout:
// R R R Tx
// R R R Ty
// R R R Tz
// 0 0 0 1
// The matrix is constructed assuming rotation is applied before translation
public float4x4 buildRigidMatrix(RigidTransformationParameters params)
{
    let R = rotationMatrix(float3(params.rx, params.ry, params.rz));
    return float4x4(
        R[0][0], R[0][1], R[0][2], params.tx,
        R[1][0], R[1][1], R[1][2], params.ty,
        R[2][0], R[2][1], R[2][2], params.tz,
        0.0F, 0.0F, 0.0F, 1.0F);
}

// Matrix layout:
// M M M Tx
// M M M Ty
// M M M Tz
// 0 0 0 1
// M = R * S * Sh is the 3x3 linear transformation matrix (Rotation * Scale * Shear)
public float4x4 buildAffineMatrix(AffineTransformationParameters params, Optional<float3x3> precomputedRotation = none)
{
    let rotationVector = float3(params.rx, params.ry, params.rz);
    let R = precomputedRotation.hasValue ? precomputedRotation.value : rotationMatrix(rotationVector);
    let S = float3x3(
        params.sx, 0.0F, 0.0F,
        0.0F, params.sy, 0.0F,
        0.0F, 0.0F, params.sz);

    let Sh = float3x3(
        1.0F, params.shx, params.shy,
        0.0F, 1.0F, params.shz,
        0.0F, 0.0F, 1.0F);

    // NOTE: in HLSL(and Slang), the * operator is for component-wise multiplication,
    // so we need to use mul() to multiply matrices.
    let M = mul(R, mul(S, Sh));
    return float4x4(
        M[0][0], M[0][1], M[0][2], params.tx,
        M[1][0], M[1][1], M[1][2], params.ty,
        M[2][0], M[2][1], M[2][2], params.tz,
        0.0F, 0.0F, 0.0F, 1.0F);
}

// T(v;α) = Rotation matrix + Translation
public float3 applyRigidTransform3D(float3 coord, RigidTransformationParameters params)
{
    let coord4 = float4(coord, 1.0F);
    let transformed = mul(buildRigidMatrix(params), coord4);
    return transformed.xyz;
}

public float3 applyRigidTransform3DAboutPivot(float3 coord,
                                              float3 pivot,
                                              RigidTransformationParameters params)
{
    let local = coord - pivot;
    let transformed4 = mul(buildRigidMatrix(params), float4(local, 1.0F));
    return transformed4.xyz + pivot;
}

// T(v;α) = Rotation Matrix * Scale Matrix * Shear Matrix + Translation
public float3 applyAffineTransform3D(float3 coord, AffineTransformationParameters params)
{
    let coord4 = float4(coord, 1.0f);
    let transformed = mul(buildAffineMatrix(params), coord4);
    return transformed.xyz;
}

public float3 applyAffineTransform3DAboutPivot(float3 coord, float3 pivot, AffineTransformationParameters params)
{
    let local = coord - pivot;
    let transformed4 = mul(buildAffineMatrix(params), float4(local, 1.0f));
    return transformed4.xyz + pivot;
}

public float3 rigidJacobianVector(uint32_t index, float3 coord, RigidTransformationParameters params)
{
    if(index == 0) return float3(1.0F, 0.0F, 0.0F);
    if(index == 1) return float3(0.0F, 1.0F, 0.0F);
    if(index == 2) return float3(0.0F, 0.0F, 1.0F);

    let rotationVector = float3(params.rx, params.ry, params.rz);
    if(index == 3) return mul(rotationMatrixDerivative(rotationVector, 0), coord);
    if(index == 4) return mul(rotationMatrixDerivative(rotationVector, 1), coord);
    if(index == 5) return mul(rotationMatrixDerivative(rotationVector, 2), coord);

    return float3(0.0F, 0.0F, 0.0F); // Invalid index
}

public float3[6] rigidJacobian(float3 coord, RigidTransformationParameters params)
{
    let dTdtx = rigidJacobianVector(0U, coord, params);
    let dTdty = rigidJacobianVector(1U, coord, params);
    let dTdtz = rigidJacobianVector(2U, coord, params);
    let dRdVx = rigidJacobianVector(3U, coord, params);
    let dRdVy = rigidJacobianVector(4U, coord, params);
    let dRdVz = rigidJacobianVector(5U, coord, params);

    return float3[6](
        dTdtx, dTdty, dTdtz,
        dRdVx, dRdVy, dRdVz);
}



public float3 affineJacobianVector(uint32_t index, float3 coord, AffineTransformationParameters params)
{
}

public float3[12] affineJacobian(float3 coord, AffineTransformationParameters params)
{
    let dTdtx = affineJacobianVector(0U, coord, params);
    let dTdty = affineJacobianVector(1U, coord, params);
    let dTdtz = affineJacobianVector(2U, coord, params);
    let dTdRx = affineJacobianVector(3U, coord, params);
    let dTdRy = affineJacobianVector(4U, coord, params);
    let dTdRz = affineJacobianVector(5U, coord, params);
    let dTdSx = affineJacobianVector(6U, coord, params);
    let dTdSy = affineJacobianVector(7U, coord, params);
    let dTdSz = affineJacobianVector(8U, coord, params);
    let dTdShx = affineJacobianVector(9U, coord, params);
    let dTdShy = affineJacobianVector(10U, coord, params);
    let dTdShz = affineJacobianVector(11U, coord, params);

    return float3[12](
        dTdtx, dTdty, dTdtz,
        dTdRx, dTdRy, dTdRz,
        dTdSx, dTdSy, dTdSz,
        dTdShx, dTdShy, dTdShz);
}

public struct RigidTransformation : ITransformation
{
    public typedef RigidTransformationParameters TParams;
    public typedef float3[6] Jacobian;
    public static const uint kParamCount = 6;
    public TParams params;
    public float3 pivot;

    __init(TParams params, float3 pivot)
    {
        this.params = params;
        this.pivot = pivot;
    }
    public float3 apply(float3 coord)
    {
        return applyRigidTransform3DAboutPivot(coord, pivot, params);
    }

    public float4x4 matrix() 
    {
        return buildRigidMatrix(params);
    }
    
    public Jacobian jacobian(float3 coord)
    {
        let evalCoord = coord - pivot;
        return rigidJacobian(evalCoord, params);
    }

    public float3 jacobianVector(uint32_t index, float3 coord)
    {
        let evalCoord = coord - pivot;
        return rigidJacobianVector(index, evalCoord, params);
    }
}

public struct AffineTransformation : ITransformation
{
    public typedef AffineTransformationParameters TParams;
    public typedef float3[12] Jacobian;
    public static const uint kParamCount = 12;
    public TParams params;
    public float3 pivot;
    float3x3 rotationMat;

    __init(TParams params, float3 pivot)
    {
        this.params = params;
        this.pivot = pivot;
        this.rotationMat = rotationMatrix(float3(params.rx, params.ry, params.rz));
    }

    public float3 apply(float3 coord)
    {
        let local = coord - pivot;
        let transformed4 = mul(buildAffineMatrix(params, rotationMat), float4(local, 1.0f));
        return transformed4.xyz + pivot;
    }

    float4x4 matrix() 
    {
        return buildAffineMatrix(params, rotationMat);
    }

    public Jacobian jacobian(float3 coord)
    {
        let evalCoord = coord - pivot;
        return affineJacobian(evalCoord, params);
    }

    public float3 jacobianVector(uint32_t index, float3 coord)
    {
        let evalCoord = coord - pivot;
        if(index == 0U) return float3(1.0F, 0.0F, 0.0F);
        if(index == 1U) return float3(0.0F, 1.0F, 0.0F);
        if(index == 2U) return float3(0.0F, 0.0F, 1.0F);

        let scaleMatrix = float3x3(
            params.sx, 0.0F, 0.0F,
            0.0F, params.sy, 0.0F,
            0.0F, 0.0F, params.sz);

        let shearMatrix = float3x3(
            1.0F, params.shx, params.shy,
            0.0F, 1.0F, params.shz,
            0.0F, 0.0F, 1.0F);


        // Rotation derivatives : (∂T/∂v_i) = ∂R/∂v_i * S * Sh * x
        if(index >= 3U && index <= 5U) {
            let rotationVector = float3(params.rx, params.ry, params.rz);
            let shearedThenScaledCoord = mul(scaleMatrix, mul(shearMatrix, evalCoord));
            return mul(rotationMatrixDerivative(rotationVector, index - 3U), shearedThenScaledCoord);
        }

        // Scale derivatives : (∂T/∂scale_i) = (R * ∂S/∂s_i * Sh) * x
        if(index >= 6U && index <= 8U) {
            let coordSheared = mul(shearMatrix, evalCoord);
            if(index == 6U) return mul(rotationMat, float3(coordSheared.x, 0.0, 0.0));
            if(index == 7U) return mul(rotationMat, float3(0.0, coordSheared.y, 0.0));
            return mul(rotationMat, float3(0.0, 0.0, coordSheared.z));
        }

        // Shear derivatives : (∂T/∂shear_i) = (R * S * ∂Sh/∂sh_i) * x
        if(index >= 9U && index <= 11U) {
            if(index == 9U) return mul(rotationMat, mul(scaleMatrix, float3(evalCoord.y, 0.0, 0.0)));
            if(index == 10U) return mul(rotationMat, mul(scaleMatrix, float3(evalCoord.z, 0.0, 0.0)));
            return mul(rotationMat, mul(scaleMatrix, float3(0.0, evalCoord.z, 0.0)));
        }

        return float3(0.0F, 0.0F, 0.0F); // Invalid index

    }
}