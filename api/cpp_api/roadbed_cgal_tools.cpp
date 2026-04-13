#include "roadbed_cgal_tools.h"
#include "tools/clip_3d_closed/clip_3d_closed.h"
#include "tools/clip_2d/clip_2d.h"
#include "core/geometry/mesh_utils.h"

namespace CGAL_OSG_TOOL_NS {

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
    const PrecisionParams& precision
) {
    return clipOSGNodeWithPlane(node, plane, precision.vectorEpsilon, precision.planeEpsilon, precision.texCoordEpsilon, precision.bboxEpsilon);
}

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
    const PrecisionParams& precision
) {
    return clipOSGGeometryWithGeometry(subjectNode, clipperNode, static_cast<int>(clipType), precision.vectorEpsilon, precision.planeEpsilon, precision.texCoordEpsilon, precision.bboxEpsilon);
}

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
    const PrecisionParams& precision
) {
    return clipOSGNodeWithPolygon(node, clipper, precision.vectorEpsilon, precision.planeEpsilon, precision.texCoordEpsilon, precision.bboxEpsilon);
}

/**
 * 计算网格信息
 * @param mesh 输入网格
 * @param meshName 网格名称
 */
void computeMeshInfo(
    const Surface_mesh& mesh,
    const std::string& meshName
) {
    printMeshInfo(mesh, meshName);
}

} // namespace CGAL_OSG_TOOL_NS
