// Cubic B–spline kernel with compact support = 2
float cubicBSpline(float x)
{
    let ax = abs(x);
    let u = max(0.0, 2.0 - ax);
    let v = max(0.0, 1.0 - ax);
    let u3 = u * u * u;
    let v3 = v * v * v;
    return (u3 - 4.0 * v3) * (1.0 / 6.0);
}

float cubicBSplineDerivative(float x)
{
    let ax = abs(x);
    let u = max(0.0, 2.0 - ax);
    let v = max(0.0, 1.0 - ax);
    let du_dx = (ax < 2.0) ? -sign(x) : 0.0;
    let dv_dx = (ax < 1.0) ? -sign(x) : 0.0;
    let u2 = u * u;
    let v2 = v * v;
    return (3.0 * u2 * du_dx - 12.0 * v2 * dv_dx) * (1.0 / 6.0);
}
