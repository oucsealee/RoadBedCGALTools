/**
 * @file rb_tool_clip_3d_open_capi.cpp
 * @brief C API 导出接口实现
 *
 * 所有参数直接通过 void* 传递，按约定内存布局解释
 */
#include "clip_3d_open_capi.h"
#include "tools/clip_3d_open/clip_3d_open.h"
#include <osg/Plane>
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/Group>
#include <osg/Geometry>
#include <vector>
#include <cstring>
#include "Math/Line3d.h"

extern "C" {

// ============================================================================
// 裁剪平面操作
// ============================================================================

bool rb_add_clip_plane(void* node, const void* plane, unsigned int clipPlaneIndex) {
    if (!node || !plane) return false;
    return CGAL_OSG_TOOL_NS::addClipPlaneToNode(
        static_cast<osg::Node*>(node),
        *static_cast<const osg::Plane*>(plane),
        clipPlaneIndex);
}

bool rb_add_clip_planes(void* node, const void* planes) {
    if (!node || !planes) return false;
    auto* arr = static_cast<std::vector<osg::Plane>*>(const_cast<void*>(planes));
    return CGAL_OSG_TOOL_NS::addClipPlanesToNode(
        static_cast<osg::Node*>(node), *arr);
}

bool rb_remove_clip_plane(void* node, unsigned int clipPlaneIndex) {
    if (!node) return false;
    return CGAL_OSG_TOOL_NS::removeClipPlaneFromNode(
        static_cast<osg::Node*>(node), clipPlaneIndex);
}

bool rb_remove_all_clip_planes(void* node) {
    if (!node) return false;
    return CGAL_OSG_TOOL_NS::removeAllClipPlanesFromNode(static_cast<osg::Node*>(node));
}

void* rb_clip_open_mesh_simple(
    void* node,
    const void* plane,
    double vectorEps,
    double planeEps,
    double texCoordEps,
    double bboxEps) {

    if (!node || !plane) {
        return nullptr;
    }

    return CGAL_OSG_TOOL_NS::clipOpenMeshSimple(
        static_cast<osg::Node*>(node),
        *static_cast<const osg::Plane*>(plane),
        vectorEps, planeEps, texCoordEps, bboxEps);
}

// ============================================================================
// 网格-平面交线
// ============================================================================

void* rb_compute_mesh_plane_intersection(
    void* node,
    const void* plane,
    const void* color,
    double lineWidth) {

    if (!node || !plane || !color) {
        return nullptr;
    }

    return CGAL_OSG_TOOL_NS::computeMeshPlaneIntersection(
        static_cast<osg::Node*>(node),
        *static_cast<const osg::Plane*>(plane),
        *static_cast<const osg::Vec4*>(color),
        lineWidth);
}

// ============================================================================
// 网格-网格交线
// ============================================================================

void* rb_compute_mesh_mesh_intersection(
    void* meshA,
    void* meshB,
    const void* color,
    double lineWidth,
    double tolerance) {

    if (!meshA || !meshB || !color) {
        return nullptr;
    }

    return CGAL_OSG_TOOL_NS::computeMeshMeshIntersection(
        static_cast<osg::Node*>(meshA),
        static_cast<osg::Node*>(meshB),
        *static_cast<const osg::Vec4*>(color),
        lineWidth,
        tolerance);
}

void* rb_compute_mesh_mesh_intersection_points(
    void* meshA,
    void* meshB,
    double vectorEps,
    double planeEps,
    double texCoordEps,
    double bboxEps) {

    if (!meshA || !meshB) {
        return nullptr;
    }

    std::vector<std::vector<osg::Vec3>> polylines =
        CGAL_OSG_TOOL_NS::computeMeshMeshIntersectionPoints(
            static_cast<osg::Node*>(meshA),
            static_cast<osg::Node*>(meshB),
            vectorEps, planeEps, texCoordEps, bboxEps);

    return new std::vector<std::vector<osg::Vec3>>(std::move(polylines));
}

} // extern "C"
