/**
 * @file rb_tool_clip_3d_closed_capi.cpp
 * @brief C API 导出接口实现
 *
 * 所有参数直接通过 void* 传递，按约定内存布局解释
 */
#include "clip_3d_closed_capi.h"
#include "tools/clip_3d_closed/clip_3d_closed.h"
#include <osg/Plane>
#include <osg/Vec3>
#include <osg/Node>
#include "Math/Line3d.h"
#include "Math/Vector3d.h"

extern "C" {

// ============================================================================
// 平面裁剪
// ============================================================================

void* rb_clip_closed_node_with_plane(
    void* node,
    const void* plane,
    double vectorEps,
    double planeEps,
    double texCoordEps,
    double bboxEps) {

    if (!node || !plane) {
        return nullptr;
    }

    return CGAL_OSG_TOOL_NS::clipOSGNodeWithPlane(
        static_cast<osg::Node*>(node),
        *static_cast<const osg::Plane*>(plane),
        vectorEps, planeEps, texCoordEps, bboxEps);
}

// ============================================================================
// 几何体裁剪
// ============================================================================

void* rb_clip_closed_node_with_geometry(
    void* subjectNode,
    void* clipperNode,
    int clipType,
    double vectorEps,
    double planeEps,
    double texCoordEps,
    double bboxEps) {

    if (!subjectNode || !clipperNode) {
        return nullptr;
    }

    return CGAL_OSG_TOOL_NS::clipOSGGeometryWithGeometry(
        static_cast<osg::Node*>(subjectNode),
        static_cast<osg::Node*>(clipperNode),
        clipType,
        vectorEps, planeEps, texCoordEps, bboxEps);
}

// ============================================================================
// 3D 直线裁剪
// ============================================================================

void* rb_clip_closed_node_with_line3d(
    void* node,
    const void* line,
    const void* normal,
    double thickness,
    int clipType,
    double vectorEps,
    double planeEps,
    double texCoordEps,
    double bboxEps) {

    if (!node || !line || !normal) {
        return nullptr;
    }

    return CGAL_OSG_TOOL_NS::clipNodeWithLine3dBoolean(
        static_cast<osg::Node*>(node),
        *static_cast<const Math::Line3d*>(line),
        *static_cast<const Math::Vector3d*>(normal),
        thickness,
        clipType,
        vectorEps, planeEps, texCoordEps, bboxEps);
}

} // extern "C"
