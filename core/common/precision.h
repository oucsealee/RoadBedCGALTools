#ifndef PRECISION_H
#define PRECISION_H

#pragma warning(disable: 4828)

namespace CGAL_OSG_TOOL_NS {

/**
 * 精度控制参数
 */
struct PrecisionParams
{
public:
    double vectorEpsilon = 1e-10;    // 向量长度判断阈值 默认1e-10
    double planeEpsilon = 1e-6;     // 平面法向量判断阈值 默认1e-6
    double texCoordEpsilon = 1e-10;  // 纹理坐标比较阈值 默认1e-10
    double bboxEpsilon = 1e-10;      // 边界框尺寸判断阈值 默认1e-10

    PrecisionParams() {}

    PrecisionParams(double vecEps, double planeEps, double texEps, double bboxEps)
        : vectorEpsilon(vecEps)
        , planeEpsilon(planeEps)
        , texCoordEpsilon(texEps)
        , bboxEpsilon(bboxEps)
    {}

    /**
     * 设置默认精度参数
     */
    static PrecisionParams defaultPrecision() {
        return PrecisionParams();
    }

    /**
     * 设置高精度参数
     */
    static PrecisionParams highPrecision() {
        return PrecisionParams(1e-12, 1e-8, 1e-12, 1e-12);
    }

    /**
     * 设置低精度参数（更快的计算）
     */
    static PrecisionParams lowPrecision() {
        return PrecisionParams(1e-8, 1e-4, 1e-8, 1e-8);
    }
};

} // namespace CGAL_OSG_TOOL_NS

#endif // PRECISION_H