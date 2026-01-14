import cubic_bspline;

// Parzen binner helper as a templated struct.
// Provides intensity -> bin mapping, neighbourhood computation and
// precomputation of cubic B-spline weights and derivatives.

struct ParzenBinner<let NumBins : uint32_t, let Padding : int32_t>
{
    // Small epsilon for numeric stability
    static const float epsilon = 1e-9F;

    // Effective number of bins excluding padding on both ends
    static float effectiveRangeBins()
    {
        return float((NumBins - 1) - 2 * Padding);
    }

    // Map an intensity into bin-space (clamped)
    static float mapIntensityToBin(float intensity, float minVal, float maxVal)
    {
        let range = max(maxVal - minVal, epsilon);
        let eff = effectiveRangeBins();
        var bin = (intensity - minVal) / range * eff + float(Padding);
        return clamp(bin, float(Padding), float(NumBins - 1 - Padding));
    }

    // Compute start/end bin indices (4-wide neighbourhood for cubic B-spline)
    static void computeBinNeighbourhood(float bin, out uint32_t start, out uint32_t end)
    {
        let centre = int32_t(floor(bin));
        start = uint32_t(max(centre - 1, 0));
        end   = uint32_t(min(centre + 2, int32_t(NumBins - 1)));
    }

    // Precompute B-spline weights for bins in [start..end]
    static void computeWeights(float bin, uint32_t start, uint32_t end,
                                  out float4 weights, out uint32_t count)
    {
        count = 0U;
        for (uint32_t i = start; i <= end; ++i) {
            weights[count] = cubicBSpline(bin - float(i));
            ++count;
        }

        // Initialize remaining weights to zero
        for (uint32_t i = count; i < 4; ++i) {
            weights[i] = 0.0F;
        }
    }

    // Precompute weights and their derivatives for bins in [start..end]
    static void computeWeightsAndDerivatives(float bin, uint32_t start, uint32_t end,
                                           out float4 weights,
                                           out float4 derivatives,
                                           out uint32_t count)
    {
        count = 0U;
        for (uint32_t j = start; j <= end; ++j) {
            let rel = bin - float(j);
            weights[count] = cubicBSpline(rel);
            derivatives[count]  = cubicBSplineDerivative(rel);
            ++count;
        }

        // Initialize remaining weights and derivatives to zero
        for (uint32_t i = count; i < 4; ++i) {
            weights[i] = 0.0F;
            derivatives[i] = 0.0F;
        }
    }

    static void computeDerivatives(float bin, uint32_t start, uint32_t end,
                                   out float4 derivatives,
                                   out uint32_t count)
    {
        count = 0U;
        for (uint32_t j = start; j <= end; ++j) {
            let rel = bin - float(j);
            derivatives[count]  = cubicBSplineDerivative(rel);
            ++count;
        }

        // Initialize remaining weights and derivatives to zero
        for (uint32_t i = count; i < 4; ++i) {
            derivatives[i] = 0.0F;
        }
    }
};