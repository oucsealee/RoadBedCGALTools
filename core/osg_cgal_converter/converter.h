#ifndef CONVERTER_H
#define CONVERTER_H

#include <osg/Geometry>
#include <osg/Node>
#include <osg/Geode>
#include <osg/Array>
#include <osg/Vec2>
#include <osg/Vec3>
#include <osg/Vec4>

#include <vector>
#include <map>
#include <string>

#include <CGAL/basic.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/General_polygon_set_2.h>
#include <CGAL/Gps_segment_traits_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Surface_mesh.h>

#include "texture_sampler.h"
#include "core/common/precision.h"

namespace CGAL_OSG_TOOL_NS {

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Surface_mesh<K::Point_3> Surface_mesh;
typedef CGAL::Gps_segment_traits_2<K> Traits_2;
typedef Traits_2::Polygon_2 Polygon_2;
typedef Traits_2::Polygon_with_holes_2 Polygon_with_holes_2;
typedef CGAL::General_polygon_set_2<Traits_2> Polygon_set;

/**
 * osg::Vec3 比较器（用于std::map）
 */
struct Vec3Comparator {
    const PrecisionParams* precision;
    Vec3Comparator(const PrecisionParams* p = nullptr) : precision(p) {}

    bool operator()(const osg::Vec3& a, const osg::Vec3& b) const {
        double eps = precision ? precision->vectorEpsilon : 1e-10;
        if (std::abs(a.x() - b.x()) > eps) return a.x() < b.x();
        if (std::abs(a.y() - b.y()) > eps) return a.y() < b.y();
        return a.z() < b.z();
    }
};

// 节点验证结果
struct NodeValidationResult {
    osg::Geode* geode;
    bool valid;
    std::string errorMessage;
};

/**
 * 验证OSG节点是否为有效的Geode并包含drawables
 */
NodeValidationResult validateOSGNode(osg::Node* node, const std::string& functionName, const std::string& nodeName = "node");

/**
 * 验证Geometry是否有足够的顶点
 */
bool validateGeometry(osg::Geometry* geom, const std::string& functionName, unsigned int geomIndex, int minVertices = 3);

/**
 * 验证和修复Surface_mesh
 *
 * @param mesh 要验证的mesh
 * @param bclose_model 是否封闭模型
 * @param functionName 调用该函数的函数名，用于日志输出
 * @return 成功返回true，失败返回false
 */
bool validateAndRepairMesh(Surface_mesh* mesh, bool bclose_model, const std::string& functionName);

/**
 * 统一处理Geometry处理过程中的异常
 * 封装了四种标准异常类型的捕获逻辑
 */
void tryProcessGeometryWithExceptionHandling(const std::function<void()>& processFunc, const std::string& functionName, unsigned int geomIndex, int& failureCount);

/**
 * 从原始Geometry采样纹理坐标（用于裁剪后纹理恢复）
 *
 * @param origGeom 原始OSG Geometry
 * @param point 要采样的3D点
 * @param hasTexCoords 原始几何体是否有纹理坐标
 * @param tolerance 容差
 * @return 采样得到的纹理坐标
 *
 * @note 使用AABB树查找最近三角形，然后线性插值纹理坐标
 */
osg::Vec2 sampleTextureFromGeometry(osg::Geometry* origGeom, const osg::Vec3& point, bool hasTexCoords, double tolerance = 1e-6);

// ============================================================================
// 3D 转换函数 (OSG Geometry ? CGAL Surface_mesh)
// ============================================================================

/**
 * 将OSG Geometry转换为CGAL Surface_mesh
 *
 * @param geom OSG Geometry对象
 * @param hasTexCoords 输出是否有纹理坐标
 * @param bclose_model 是否封闭模型
 * @param precision 精度参数
 * @return CGAL Surface_mesh指针，失败返回nullptr
 *
 * @note 不保留纹理坐标和颜色信息，但会检查并输出是否有纹理坐标
 * @note 使用位置作为顶点键（不考虑纹理坐标）
 */
Surface_mesh* geometryToSurfaceMesh(osg::Geometry* geom,
                                     bool& hasTexCoords, 
                                     bool bclose_model,
                                     const PrecisionParams& precision);

/**
 * 将OSG Geode转换为CGAL Surface_mesh（合并所有Geometry）
 *
 * @param geode OSG Geode对象
 * @param cgalMesh 输出的CGAL Surface_mesh
 * @param hasTexCoords 输出是否有纹理坐标
 * @param bclose_model 是否封闭模型
 * @param meshName mesh名称（用于日志）
 * @return 成功返回true，失败返回false
 *
 * @note 将Geode中的所有Geometry合并到一个Surface_mesh中
 * @note 检查并输出是否有纹理坐标
 */
bool geodeToSurfaceMesh(osg::Geode* geode, Surface_mesh& cgalMesh, bool& hasTexCoords, bool bclose_model, 
                       const std::string& meshName = "", const PrecisionParams& precision_param = PrecisionParams());

/**
 * 将Surface_mesh转换回OSG Geometry（通过空间插值采样纹理坐标）
 *
 * @param mesh CGAL Surface_mesh对象
 * @param originalGeom 原始OSG Geometry（用于采样纹理坐标）
 * @param hasTexCoords 是否有纹理坐标
 * @param precision 精度参数
 * @return OSG Geometry指针，失败返回nullptr
 *
 * @note 纹理坐标通过AABB树查找最近三角形并线性插值
 * @note 计算顶点法线
 */
osg::Geometry* createOSGGeometryFromSurfaceMesh(Surface_mesh& mesh,
                                                 osg::Geometry* originalGeom,
                                                 bool hasTexCoords,
                                                 const PrecisionParams& precision = PrecisionParams());

// ============================================================================
// 2D 转换函数 (OSG Geometry ? CGAL Polygon_2)
// ============================================================================

/**
 * 从三角面片提取CGAL多边形 (2D)
 *
 * @param geom OSG Geometry对象
 * @return CGAL多边形数组
 */
std::vector<Polygon_2> extractPolygonsFromGeometry(osg::Geometry* geom);

/**
 * 使用CGAL进行多边形裁剪 (2D)
 *
 * @param subjects 要裁剪的多边形数组
 * @param clipper 裁剪多边形
 * @return 裁剪后的多边形数组
 */
std::vector<Polygon_2> clipPolygons(const std::vector<Polygon_2>& subjects, const Polygon_2& clipper);

/**
 * 将CGAL多边形转换为OSG Geometry (2D)
 *
 * @param polygons CGAL多边形数组
 * @param hasOriginalTexCoords 是否有原始纹理坐标
 * @return OSG Geometry指针，失败返回nullptr
 *
 * @note 裁剪后的纹理坐标会根据包围盒重新生成
 */
osg::Geometry* createOSGGeometryFromPolygons(const std::vector<Polygon_2>& polygons,
                                               bool hasOriginalTexCoords);

}  // namespace CGAL_OSG_TOOL_NS

#endif // CONVERTER_H