#ifndef ROADBED_CGAL_TOOLS_H
#define ROADBED_CGAL_TOOLS_H

#include <osg/Node>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Vec3>
#include <osg/Vec2>
#include <vector>
#include <string>

#include "core/osg_cgal_converter/converter.h"
#include "core/geometry/mesh_utils.h"
#include "core/common/precision.h"
#include "utils/error_handling.h"
#include "utils/logging.h"
namespace Math
{
	class Line3d;
	class Vector3d;
}

namespace CGAL_OSG_TOOL_NS {

/**
 * 3D裁剪类型
 */
enum class ClipType {
    DIFFERENCE = 0,    // 差集 (node - clipper)
    INTERSECTION = 1,  // 交集 (node ∩ clipper)
    UNION = 2          // 并集 (node ∪ clipper)
};

/**
 * 3D平面裁剪
 * @param node 要裁剪的OSG节点
 * @param plane 裁剪平面
 * @param precision 精度参数
 * @return 裁剪后的OSG节点，失败返回nullptr
 */
osg::Node* clipOSGNodeWithPlane(
    osg::Node* node,
    const osg::Plane& plane,
    const PrecisionParams& precision = PrecisionParams::defaultPrecision()
);

/**
 * 3D几何体裁剪
 * @param subjectNode 被裁剪的OSG节点
 * @param clipperNode 裁剪器OSG节点
 * @param clipType 裁剪类型
 * @param precision 精度参数
 * @return 裁剪后的OSG节点，失败返回nullptr
 */
osg::Node* clipOSGNodeWithNode(
    osg::Node* subjectNode,
    osg::Node* clipperNode,
    ClipType clipType,
    const PrecisionParams& precision = PrecisionParams::defaultPrecision()
);

/**
 * 2D多边形裁剪
 * @param node 要裁剪的OSG节点
 * @param clipper 裁剪多边形
 * @param precision 精度参数
 * @return 裁剪后的OSG节点，失败返回nullptr
 */
osg::Node* clipOSGNodeWithPolygon(
    osg::Node* node,
    const std::vector<osg::Vec2>& clipper,
    const PrecisionParams& precision = PrecisionParams::defaultPrecision()
);


/**
 * 计算网格信息
 * @param mesh 输入网格
 * @param meshName 网格名称
 */
void computeMeshInfo(
    const Surface_mesh& mesh,
    const std::string& meshName = "Mesh"
);

} // namespace CGAL_OSG_TOOL_NS

#endif // ROADBED_CGAL_TOOLS_H