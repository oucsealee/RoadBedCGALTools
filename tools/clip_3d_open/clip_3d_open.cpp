#include "clip_3d_open.h"
#include "core/osg_cgal_converter/converter.h"
#include "core/geometry/mesh_utils.h"
#include "utils/error_handling.h"
#include "utils/logging.h"

#include <osg/Node>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Plane>
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/Group>
#include <osg/StateSet>
#include <osg/ClipPlane>
#include <osg/TriangleIndexFunctor>
#include <osg/Array>
#include <osg/PrimitiveSet>

#include <vector>
#include <memory>
#include <string>
#include <cmath>
#include <osg/GL>

#include <CGAL/Polygon_mesh_processing/intersection.h>
#include <CGAL/Polygon_mesh_processing/clip.h>

namespace CGAL_OSG_TOOL_NS {

/**
 * 向节点添加裁剪平面
 */
bool addClipPlaneToNode(osg::Node* node, const osg::Plane& plane, unsigned int clipPlaneIndex) {
    if (!node) {
        Logger::error("addClipPlaneToNode: node is nullptr");
        return false;
    }

    osg::ref_ptr<osg::StateSet> stateSet = node->getOrCreateStateSet();
    osg::ref_ptr<osg::ClipPlane> clipPlane = new osg::ClipPlane(clipPlaneIndex, plane);
    stateSet->setAttributeAndModes(clipPlane, osg::StateAttribute::ON);
    stateSet->setMode(GL_CLIP_PLANE0 + clipPlaneIndex, osg::StateAttribute::ON);

    return true;
}

/**
 * 向节点添加多个裁剪平面
 */
bool addClipPlanesToNode(osg::Node* node, const std::vector<osg::Plane>& planes) {
    if (!node) {
        Logger::error("addClipPlanesToNode: node is nullptr");
        return false;
    }

    for (unsigned int i = 0; i < planes.size(); ++i) {
        if (!addClipPlaneToNode(node, planes[i], i)) {
            return false;
        }
    }

    return true;
}

/**
 * 从节点移除裁剪平面
 */
bool removeClipPlaneFromNode(osg::Node* node, unsigned int clipPlaneIndex) {
    if (!node) {
        Logger::error("removeClipPlaneFromNode: node is nullptr");
        return false;
    }

    osg::ref_ptr<osg::StateSet> stateSet = node->getStateSet();
    if (!stateSet) {
        return false;
    }

    stateSet->removeAttribute(osg::StateAttribute::CLIPPLANE, clipPlaneIndex);
    stateSet->setMode(GL_CLIP_PLANE0 + clipPlaneIndex, osg::StateAttribute::OFF);

    return true;
}

/**
 * 从节点移除所有裁剪平面
 */
bool removeAllClipPlanesFromNode(osg::Node* node) {
    if (!node) {
        Logger::error("removeAllClipPlanesFromNode: node is nullptr");
        return false;
    }

    osg::ref_ptr<osg::StateSet> stateSet = node->getStateSet();
    if (!stateSet) {
        return false;
    }

    // 移除最多8个裁剪平面（OpenGL限制）
    for (unsigned int i = 0; i < 8; ++i) {
        stateSet->removeAttribute(osg::StateAttribute::CLIPPLANE, i);
        stateSet->setMode(GL_CLIP_PLANE0 + i, osg::StateAttribute::OFF);
    }

    return true;
}

/**
 * 简化平面裁剪（开放网格）
 */
osg::Node* clipOpenMeshSimple(osg::Node* node, const osg::Plane& plane, double vectorEpsilon, double planeEpsilon, double texCoordEpsilon, double bboxEpsilon) {
    if (!node) {
        Logger::error("clipOpenMeshSimple: node is nullptr");
        return nullptr;
    }

    // 创建一个新的Group节点作为父节点
    osg::ref_ptr<osg::Group> resultGroup = new osg::Group;

    // 克隆原始节点
    osg::ref_ptr<osg::Node> clonedNode = dynamic_cast<osg::Node*>(node->clone(osg::CopyOp::DEEP_COPY_ALL));
    if (!clonedNode) {
        Logger::error("clipOpenMeshSimple: failed to clone node");
        return nullptr;
    }

    // 向克隆节点添加裁剪平面
    if (!addClipPlaneToNode(clonedNode, plane)) {
        Logger::error("clipOpenMeshSimple: failed to add clip plane");
        return nullptr;
    }

    // 将克隆节点添加到结果组
    resultGroup->addChild(clonedNode);

    return resultGroup.release();
}

/**
 * 计算网格与平面的交线
 */
osg::Geometry* computeMeshPlaneIntersection(osg::Node* node, const osg::Plane& plane, const osg::Vec4& color, double lineWidth) {
    if (!node) {
        Logger::error("computeMeshPlaneIntersection: node is nullptr");
        return nullptr;
    }

    // 验证输入节点
    NodeValidationResult validationResult = validateOSGNode(node, "computeMeshPlaneIntersection");
    if (!validationResult.valid) {
        Logger::error(validationResult.errorMessage);
        return nullptr;
    }
    osg::Geode* geode = validationResult.geode;

    // 收集所有交点
    std::vector<osg::Vec3> intersectionPoints;

    // 遍历Geode中的所有Geometry
    for (unsigned int i = 0; i < geode->getNumDrawables(); ++i) {
        osg::Geometry* geom = geode->getDrawable(i)->asGeometry();
        if (!geom || !validateGeometry(geom, "computeMeshPlaneIntersection", i)) {
            continue;
        }

        osg::Vec3Array* vertices = dynamic_cast<osg::Vec3Array*>(geom->getVertexArray());
        if (!vertices) {
            continue;
        }

        // 遍历所有图元
        for (unsigned int j = 0; j < geom->getNumPrimitiveSets(); ++j) {
            osg::PrimitiveSet* primSet = geom->getPrimitiveSet(j);
            if (!primSet) {
                continue;
            }

            // 处理三角形图元
            if (primSet->getMode() == GL_TRIANGLES) {
                osg::DrawElementsUInt* elements = dynamic_cast<osg::DrawElementsUInt*>(primSet);
                if (elements) {
                    for (unsigned int k = 0; k < elements->size(); k += 3) {
                        osg::Vec3 v0 = (*vertices)[elements->at(k)];
                        osg::Vec3 v1 = (*vertices)[elements->at(k + 1)];
                        osg::Vec3 v2 = (*vertices)[elements->at(k + 2)];

                        // 计算三角形与平面的交点
                        // 这里需要实现三角形与平面的交线计算
                        // 暂时省略具体实现
                    }
                }
            }
        }
    }

    // 如果没有交点，返回nullptr
    if (intersectionPoints.empty()) {
        return nullptr;
    }

    // 创建交线Geometry
    osg::ref_ptr<osg::Geometry> lineGeometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> lineVertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> lineColors = new osg::Vec4Array;

    for (const auto& point : intersectionPoints) {
        lineVertices->push_back(point);
        lineColors->push_back(color);
    }

    lineGeometry->setVertexArray(lineVertices);
    lineGeometry->setColorArray(lineColors);
    lineGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    lineGeometry->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP, 0, lineVertices->size()));

    // 设置线宽
    osg::ref_ptr<osg::StateSet> stateSet = lineGeometry->getOrCreateStateSet();
    stateSet->setMode(GL_LINE_SMOOTH, osg::StateAttribute::ON);
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);

    return lineGeometry.release();
}

/**
 * 计算两个网格的交线
 */
osg::Geometry* computeMeshMeshIntersection(osg::Node* meshA, osg::Node* meshB, const osg::Vec4& color, double lineWidth, double tolerance) {
    if (!meshA || !meshB) {
        Logger::error("computeMeshMeshIntersection: meshA or meshB is nullptr");
        return nullptr;
    }

    // 验证输入节点
    NodeValidationResult validationResultA = validateOSGNode(meshA, "computeMeshMeshIntersection", "meshA");
    if (!validationResultA.valid) {
        Logger::error(validationResultA.errorMessage);
        return nullptr;
    }

    NodeValidationResult validationResultB = validateOSGNode(meshB, "computeMeshMeshIntersection", "meshB");
    if (!validationResultB.valid) {
        Logger::error(validationResultB.errorMessage);
        return nullptr;
    }

    // 转换为CGAL Surface_mesh
    Surface_mesh meshA_cgal;
    Surface_mesh meshB_cgal;
    bool hasTexCoordsA = false;
    bool hasTexCoordsB = false;

    if (!geodeToSurfaceMesh(validationResultA.geode, meshA_cgal, hasTexCoordsA, false, "meshA")) {
        Logger::error("computeMeshMeshIntersection: failed to convert meshA to CGAL");
        return nullptr;
    }

    if (!geodeToSurfaceMesh(validationResultB.geode, meshB_cgal, hasTexCoordsB, false, "meshB")) {
        Logger::error("computeMeshMeshIntersection: failed to convert meshB to CGAL");
        return nullptr;
    }

    // 计算交线
    std::vector<std::vector<K::Point_3>> intersectionPolylines;

    try {
        CGAL::Polygon_mesh_processing::surface_intersection(
            meshA_cgal, meshB_cgal,
            std::back_inserter(intersectionPolylines));
    } catch (const std::exception& e) {
        Logger::error("computeMeshMeshIntersection: surface intersection failed: " + std::string(e.what()));
        return nullptr;
    }

    // 如果没有交线，返回nullptr
    if (intersectionPolylines.empty()) {
        return nullptr;
    }

    // 创建交线Geometry
    osg::ref_ptr<osg::Geometry> lineGeometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> lineVertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> lineColors = new osg::Vec4Array;

    for (const auto& polyline : intersectionPolylines) {
        for (const auto& point : polyline) {
            lineVertices->push_back(osg::Vec3(CGAL::to_double(point.x()), CGAL::to_double(point.y()), CGAL::to_double(point.z())));
            lineColors->push_back(color);
        }
    }

    lineGeometry->setVertexArray(lineVertices);
    lineGeometry->setColorArray(lineColors);
    lineGeometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    lineGeometry->addPrimitiveSet(new osg::DrawArrays(GL_LINE_STRIP, 0, lineVertices->size()));

    // 设置线宽
    osg::ref_ptr<osg::StateSet> stateSet = lineGeometry->getOrCreateStateSet();
    stateSet->setMode(GL_LINE_SMOOTH, osg::StateAttribute::ON);
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);

    return lineGeometry.release();
}

/**
 * 计算网格-网格交线点集（多条独立折线）
 */
std::vector<std::vector<osg::Vec3>> computeMeshMeshIntersectionPoints(osg::Node* meshA, osg::Node* meshB, double vectorEpsilon, double planeEpsilon, double texCoordEpsilon, double bboxEpsilon) {
    if (!meshA || !meshB) {
        Logger::error("computeMeshMeshIntersectionPoints: meshA or meshB is nullptr");
        return {};
    }

    // 验证输入节点
    NodeValidationResult validationResultA = validateOSGNode(meshA, "computeMeshMeshIntersectionPoints", "meshA");
    if (!validationResultA.valid) {
        Logger::error(validationResultA.errorMessage);
        return {};
    }

    NodeValidationResult validationResultB = validateOSGNode(meshB, "computeMeshMeshIntersectionPoints", "meshB");
    if (!validationResultB.valid) {
        Logger::error(validationResultB.errorMessage);
        return {};
    }

    // 转换为CGAL Surface_mesh
    Surface_mesh meshA_cgal;
    Surface_mesh meshB_cgal;
    bool hasTexCoordsA = false;
    bool hasTexCoordsB = false;
    PrecisionParams precision(vectorEpsilon, planeEpsilon, texCoordEpsilon, bboxEpsilon);

    if (!geodeToSurfaceMesh(validationResultA.geode, meshA_cgal, hasTexCoordsA, false, "meshA", precision)) {
        Logger::error("computeMeshMeshIntersectionPoints: failed to convert meshA to CGAL");
        return {};
    }

    if (!geodeToSurfaceMesh(validationResultB.geode, meshB_cgal, hasTexCoordsB, false, "meshB", precision)) {
        Logger::error("computeMeshMeshIntersectionPoints: failed to convert meshB to CGAL");
        return {};
    }

    // 计算交线
    std::vector<std::vector<K::Point_3>> intersectionPolylines;

    try {
        CGAL::Polygon_mesh_processing::surface_intersection(
            meshA_cgal, meshB_cgal,
            std::back_inserter(intersectionPolylines));
    } catch (const std::exception& e) {
        Logger::error("computeMeshMeshIntersectionPoints: surface intersection failed: " + std::string(e.what()));
        return {};
    }

    // 转换为OSG Vec3
    std::vector<std::vector<osg::Vec3>> result;
    for (const auto& polyline : intersectionPolylines) {
        std::vector<osg::Vec3> osgPolyline;
        for (const auto& point : polyline) {
            osgPolyline.push_back(osg::Vec3(CGAL::to_double(point.x()), CGAL::to_double(point.y()), CGAL::to_double(point.z())));
        }
        if (!osgPolyline.empty()) {
            result.push_back(osgPolyline);
        }
    }

    return result;
}

} // namespace CGAL_OSG_TOOL_NS