#include "clip_3d_closed.h"
#include "core/osg_cgal_converter/converter.h"
#include "core/geometry/mesh_utils.h"
#include "utils/error_handling.h"
#include "utils/logging.h"

#include "Math/Line3d.h"
#include "Math/Vector3d.h"

#include <osg/Plane>
#include <osg/Group>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/TriangleIndexFunctor>
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <osgUtil/SmoothingVisitor>

#include <vector>
#include <memory>
#include <string>
#include <cmath>

#include <CGAL/Polygon_mesh_processing/clip.h>
#include <CGAL/Polygon_mesh_processing/corefinement.h>

namespace CGAL_OSG_TOOL_NS {

/**
 * 完成处理并返回结果节点
 */
osg::Node* finalizeProcessing(osg::Geode* resultGeode, int successCount, int failureCount, const std::string& functionName) {
    if (!resultGeode || resultGeode->getNumDrawables() == 0) {
        Logger::error(functionName + ": no valid geometry after processing");
        return nullptr;
    }

    Logger::info(functionName + ": processing completed - " + 
                std::to_string(successCount) + " successful, " + 
                std::to_string(failureCount) + " failed");

    return resultGeode;
}

/**
 * 使用Math::Line3d创建一个立方体网格
 */
std::unique_ptr<Surface_mesh> createSurfaceMeshFromLine3d(const Math::Line3d& line3d, const Math::Vector3d& normal, double size, const PrecisionParams& precision) {
    if (line3d.IsZeroLine()) {
        Logger::error("createSurfaceMeshFromLine3d: line3d is zero line");
        return nullptr;
    }

    auto mesh = std::make_unique<Surface_mesh>();

    // 获取线段的起始点和结束点
    Math::Point3d startPoint = line3d.Start();
    Math::Point3d endPoint = line3d.End();

    // 计算法线normal向量
    double nx = normal.X();
    double ny = normal.Y();
    double nz = normal.Z();
    double len = sqrt(nx * nx + ny * ny + nz * nz);
    if (len < precision.vectorEpsilon) {
        Logger::error("createSurfaceMeshFromLine3d: normal is zero vector");
        return nullptr;
    }
    nx /= len;
    ny /= len;
    nz /= len;

    // 计算法线方向向量
    double dirX = endPoint.X() - startPoint.X();
    double dirY = endPoint.Y() - startPoint.Y();
    double dirZ = endPoint.Z() - startPoint.Z();
    double dirLen = sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ);
    if (dirLen < precision.vectorEpsilon) {
        Logger::error("createSurfaceMeshFromLine3d: line3d has zero length");
        return nullptr;
    }
    dirX /= dirLen; dirY /= dirLen; dirZ /= dirLen;

    // 计算法线方向向量是否与normal平面平行
    double dot = dirX * nx + dirY * ny + dirZ * nz;
    if (std::abs(dot) > 1 - precision.planeEpsilon) {
        Logger::error("createSurfaceMeshFromLine3d: line3d is parallel to normal");
        return nullptr;
    }

    // 计算与线段方向垂直的向量（沿u轴方向偏移）
    double ux = dirY * nz - dirZ * ny;
    double uy = dirZ * nx - dirX * nz;
    double uz = dirX * ny - dirY * nx;
    double ulen = sqrt(ux * ux + uy * uy + uz * uz);
    if (ulen < precision.vectorEpsilon) {
        Logger::error("createSurfaceMeshFromLine3d: failed to create orthogonal vector");
        return nullptr;
    }
    ux /= ulen; uy /= ulen; uz /= ulen;

    // 8个顶点：以line3d为棱，沿normal方向延伸size长度
    // v0, v1, v2, v3: 下底面（起始点、起始点+u、结束点+u、结束点）
    // v4, v5, v6, v7: 上底面（沿normal方向偏移）
    double sx = startPoint.X();
    double sy = startPoint.Y();
    double sz = startPoint.Z();
    double ex = endPoint.X();
    double ey = endPoint.Y();
    double ez = endPoint.Z();

    // 创建下面四个顶点（沿着u轴偏移）
    auto v0 = mesh->add_vertex(K::Point_3(sx, sy, sz));
    auto v1 = mesh->add_vertex(K::Point_3(sx + ux * size, sy + uy * size, sz + uz * size));
    auto v2 = mesh->add_vertex(K::Point_3(ex + ux * size, ey + uy * size, ez + uz * size));
    auto v3 = mesh->add_vertex(K::Point_3(ex, ey, ez));

    // 创建上面四个顶点（沿normal方向偏移）
    auto v4 = mesh->add_vertex(K::Point_3(sx + nx * size, sy + ny * size, sz + nz * size));
    auto v5 = mesh->add_vertex(K::Point_3(sx + ux * size + nx * size, sy + uy * size + ny * size, sz + uz * size + nz * size));
    auto v6 = mesh->add_vertex(K::Point_3(ex + ux * size + nx * size, ey + uy * size + ny * size, ez + uz * size + nz * size));
    auto v7 = mesh->add_vertex(K::Point_3(ex + nx * size, ey + ny * size, ez + nz * size));

    // 6个面（每个面有2个三角形，共12个三角形）
    // 下底面
    mesh->add_face(v0, v1, v2); mesh->add_face(v0, v2, v3);
    // 上底面
    mesh->add_face(v4, v7, v6); mesh->add_face(v4, v6, v5);
    // 侧面1 (v0-v1-v5-v4)
    mesh->add_face(v0, v4, v5); mesh->add_face(v0, v5, v1);
    // 侧面2 (v1-v2-v6-v5)
    mesh->add_face(v1, v5, v6); mesh->add_face(v1, v6, v2);
    // 侧面3 (v2-v3-v7-v6)
    mesh->add_face(v2, v6, v7); mesh->add_face(v2, v7, v3);
    // 侧面4 (v3-v0-v4-v7)
    mesh->add_face(v3, v7, v4); mesh->add_face(v3, v4, v0);

    // 验证并修复mesh
    if (!validateAndRepairMesh(mesh.get(), true, "createSurfaceMeshFromLine3d")) {
        return nullptr;
    }

    return mesh;
}

/**
 * 使用Math::Line3d创建一个立方体网格并转换为osg::Geometry
 */
osg::Geometry* createBoxGeometryFromLine3d(const Math::Line3d& line3d, const Math::Vector3d& normal, double size, const osg::Vec4& color, double vectorEpsilon, double planeEpsilon, double texCoordEpsilon, double bboxEps) {
    PrecisionParams precision(vectorEpsilon, planeEpsilon, texCoordEpsilon, bboxEps);
    auto mesh = createSurfaceMeshFromLine3d(line3d, normal, size, precision);
    if (!mesh) {
        return nullptr;
    }

    // 转换为OSG Geometry
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec2Array> texCoords = new osg::Vec2Array;
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;

    // 添加顶点
    for (auto v : mesh->vertices()) {
        const K::Point_3& p = mesh->point(v);
        vertices->push_back(osg::Vec3(CGAL::to_double(p.x()), CGAL::to_double(p.y()), CGAL::to_double(p.z())));
        normals->push_back(osg::Vec3(0.0, 0.0, 0.0)); // 稍后计算法线
        texCoords->push_back(osg::Vec2(0.0, 0.0)); // 默认纹理坐标
        colors->push_back(color);
    }

    // 添加面
    std::vector<unsigned int> indices;
    for (auto f : mesh->faces()) {
        std::vector<Surface_mesh::Vertex_index> faceVertices;
        for (auto v : CGAL::vertices_around_face(mesh->halfedge(f), *mesh)) {
            faceVertices.push_back(v);
        }
        if (faceVertices.size() >= 3) {
            for (size_t i = 1; i < faceVertices.size() - 1; ++i) {
                indices.push_back(faceVertices[0].idx());
                indices.push_back(faceVertices[i].idx());
                indices.push_back(faceVertices[i + 1].idx());
            }
        }
    }

    // 设置几何数据
    geometry->setVertexArray(vertices);
    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
    geometry->setTexCoordArray(0, texCoords);
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

    // 使用DrawElements进行索引绘制
    osg::ref_ptr<osg::DrawElementsUInt> drawElements = new osg::DrawElementsUInt(GL_TRIANGLES);
    for (auto i : indices) {
        drawElements->push_back(i);
    }
    geometry->addPrimitiveSet(drawElements);

    // 计算法线
    osgUtil::SmoothingVisitor smoother;
    smoother.apply(*geometry);

    return geometry.release();
}

/**
 * 使用Math::Line3d创建一个立方体网格并转换为osg::Geometry - osg::Vec3版本
 */
osg::Geometry* createBoxGeometryFromLine3d(const Math::Line3d& line3d, const osg::Vec3& normal, double size, const osg::Vec4& color, double vectorEpsilon, double planeEpsilon, double texCoordEpsilon, double bboxEps) {
    Math::Vector3d mathNormal(normal.x(), normal.y(), normal.z());
    return createBoxGeometryFromLine3d(line3d, mathNormal, size, color, vectorEpsilon, planeEpsilon, texCoordEpsilon, bboxEps);
}

/**
 * 平面裁剪函数 (3D裁剪)
 */
osg::Node* clipOSGNodeWithPlane(osg::Node* node, const osg::Plane& plane, double vectorEpsilon, double planeEpsilon, double texCoordEpsilon, double bboxEpsilon) {
    PrecisionParams precision(vectorEpsilon, planeEpsilon, texCoordEpsilon, bboxEpsilon);
    
    // 验证输入节点
    NodeValidationResult validationResult = validateOSGNode(node, "clipOSGNodeWithPlane");
    if (!validationResult.valid) {
        Logger::error(validationResult.errorMessage);
        return nullptr;
    }
    osg::Geode* geode = validationResult.geode;

    osg::ref_ptr<osg::Geode> resultGeode = new osg::Geode;
    int successCount = 0, failureCount = 0;

    // 遍历Geode中的所有Geometry
    for (unsigned int i = 0; i < geode->getNumDrawables(); ++i) {
        osg::Geometry* geom = geode->getDrawable(i)->asGeometry();
        if (!geom || !validateGeometry(geom, "clipOSGNodeWithPlane", i)) {
            failureCount++;
            continue;
        }

        // 尝试处理Geometry，捕获异常
        tryProcessGeometryWithExceptionHandling([&]() {
            bool hasTexCoords = false;
            Surface_mesh* subjectMesh = geometryToSurfaceMesh(geom, hasTexCoords, true, precision);
            if (!subjectMesh) {
                failureCount++;
                return;
            }

            // 将 osg::Plane 转换为 CGAL 平面 (ax + by + cz + d = 0)
            osg::Vec3 normal = plane.getNormal();
            double distance = plane.distance(osg::Vec3(0, 0, 0));
            K::Plane_3 cgalPlane(normal.x(), normal.y(), normal.z(), -distance);

            // 使用 CGAL 的平面裁剪功能
            bool success = CGAL::Polygon_mesh_processing::clip(*subjectMesh, cgalPlane);

            if (success) {
                osg::Geometry* resultGeom = createOSGGeometryFromSurfaceMesh(*subjectMesh, geom, hasTexCoords, precision);
                if (resultGeom) {
                    resultGeode->addDrawable(resultGeom);
                    successCount++;
                } else {
                    failureCount++;
                }
            } else {
                failureCount++;
            }

            delete subjectMesh;
        }, "clipOSGNodeWithPlane", i, failureCount);
    }

    return finalizeProcessing(resultGeode.release(), successCount, failureCount, "clipOSGNodeWithPlane");
}

/**
 * 3D几何体裁剪（支持差集/交集/并集）
 */
osg::Node* clipOSGGeometryWithGeometry(osg::Node* subjectNode, osg::Node* clipperNode, int clipType, double vectorEpsilon, double planeEpsilon, double texCoordEpsilon, double bboxEpsilon) {
    PrecisionParams precision(vectorEpsilon, planeEpsilon, texCoordEpsilon, bboxEpsilon);
    
    // 验证输入节点
    NodeValidationResult subjectValidation = validateOSGNode(subjectNode, "clipOSGGeometryWithGeometry", "subjectNode");
    if (!subjectValidation.valid) {
        Logger::error(subjectValidation.errorMessage);
        return nullptr;
    }
    osg::Geode* subjectGeode = subjectValidation.geode;

    NodeValidationResult clipperValidation = validateOSGNode(clipperNode, "clipOSGGeometryWithGeometry", "clipperNode");
    if (!clipperValidation.valid) {
        Logger::error(clipperValidation.errorMessage);
        return nullptr;
    }
    osg::Geode* clipperGeode = clipperValidation.geode;

    osg::ref_ptr<osg::Geode> resultGeode = new osg::Geode;
    int successCount = 0, failureCount = 0;

    // 遍历subjectGeode中的所有Geometry
    for (unsigned int i = 0; i < subjectGeode->getNumDrawables(); ++i) {
        osg::Geometry* subjectGeom = subjectGeode->getDrawable(i)->asGeometry();
        if (!subjectGeom || !validateGeometry(subjectGeom, "clipOSGGeometryWithGeometry", i)) {
            failureCount++;
            continue;
        }

        // 尝试处理Geometry，捕获异常
        tryProcessGeometryWithExceptionHandling([&]() {
            // 转换subject为CGAL Surface_mesh
            bool subjectHasTexCoords = false;
            Surface_mesh* subjectMesh = geometryToSurfaceMesh(subjectGeom, subjectHasTexCoords, true, precision);
            if (!subjectMesh) {
                failureCount++;
                return;
            }

            // 转换clipper为CGAL Surface_mesh
            bool clipperHasTexCoords = false;
            Surface_mesh* clipperMesh = new Surface_mesh;
            if (!geodeToSurfaceMesh(clipperGeode, *clipperMesh, clipperHasTexCoords, true, "clipper", precision)) {
                delete subjectMesh;
                delete clipperMesh;
                failureCount++;
                return;
            }

            // 执行布尔运算
            Surface_mesh* resultMesh = new Surface_mesh;
            bool success = false;

            try {
                switch (clipType) {
                case 0: // 差集 (subject - clipper)
                    success = CGAL::Polygon_mesh_processing::corefine_and_compute_difference(
                        *subjectMesh, *clipperMesh, *resultMesh);
                    break;
                case 1: // 交集 (subject ∩ clipper)
                    success = CGAL::Polygon_mesh_processing::corefine_and_compute_intersection(
                        *subjectMesh, *clipperMesh, *resultMesh);
                    break;
                case 2: // 并集 (subject ∪ clipper)
                    success = CGAL::Polygon_mesh_processing::corefine_and_compute_union(
                        *subjectMesh, *clipperMesh, *resultMesh);
                    break;
                default:
                    Logger::error("clipOSGGeometryWithGeometry: invalid clip type");
                    success = false;
                    break;
                }
            } catch (const std::exception& e) {
                Logger::error("clipOSGGeometryWithGeometry: boolean operation failed: " + std::string(e.what()));
                success = false;
            }

            if (success) {
                osg::Geometry* resultGeom = createOSGGeometryFromSurfaceMesh(*resultMesh, subjectGeom, subjectHasTexCoords, precision);
                if (resultGeom) {
                    resultGeode->addDrawable(resultGeom);
                    successCount++;
                } else {
                    failureCount++;
                }
            } else {
                failureCount++;
            }

            delete subjectMesh;
            delete clipperMesh;
            delete resultMesh;
        }, "clipOSGGeometryWithGeometry", i, failureCount);
    }

    return finalizeProcessing(resultGeode.release(), successCount, failureCount, "clipOSGGeometryWithGeometry");
}

/**
 * 3D直线布尔运算裁剪
 */
osg::Node* clipNodeWithLine3dBoolean(osg::Node* node, const Math::Line3d& line3d, const Math::Vector3d& normal, double size, int clipType, double vectorEpsilon, double planeEpsilon, double texCoordEpsilon, double bboxEpsilon) {
    PrecisionParams precision(vectorEpsilon, planeEpsilon, texCoordEpsilon, bboxEpsilon);
    
    // 验证输入节点
    NodeValidationResult validationResult = validateOSGNode(node, "clipNodeWithLine3dBoolean");
    if (!validationResult.valid) {
        Logger::error(validationResult.errorMessage);
        return nullptr;
    }
    osg::Geode* geode = validationResult.geode;

    // 创建裁剪器网格
    auto clipperMesh = createSurfaceMeshFromLine3d(line3d, normal, size, precision);
    if (!clipperMesh) {
        Logger::error("clipNodeWithLine3dBoolean: failed to create clipper mesh");
        return nullptr;
    }

    osg::ref_ptr<osg::Geode> resultGeode = new osg::Geode;
    int successCount = 0, failureCount = 0;

    // 遍历Geode中的所有Geometry
    for (unsigned int i = 0; i < geode->getNumDrawables(); ++i) {
        osg::Geometry* geom = geode->getDrawable(i)->asGeometry();
        if (!geom || !validateGeometry(geom, "clipNodeWithLine3dBoolean", i)) {
            failureCount++;
            continue;
        }

        // 尝试处理Geometry，捕获异常
        tryProcessGeometryWithExceptionHandling([&]() {
            bool hasTexCoords = false;
            Surface_mesh* subjectMesh = geometryToSurfaceMesh(geom, hasTexCoords, true, precision);
            if (!subjectMesh) {
                failureCount++;
                return;
            }

            // 执行布尔运算
            Surface_mesh* resultMesh = new Surface_mesh;
            bool success = false;

            try {
                switch (clipType) {
                case 0: // 差集 (subject - clipper)
                    success = CGAL::Polygon_mesh_processing::corefine_and_compute_difference(
                        *subjectMesh, *clipperMesh, *resultMesh);
                    break;
                case 1: // 交集 (subject ∩ clipper)
                    success = CGAL::Polygon_mesh_processing::corefine_and_compute_intersection(
                        *subjectMesh, *clipperMesh, *resultMesh);
                    break;
                case 2: // 并集 (subject ∪ clipper)
                    success = CGAL::Polygon_mesh_processing::corefine_and_compute_union(
                        *subjectMesh, *clipperMesh, *resultMesh);
                    break;
                default:
                    Logger::error("clipNodeWithLine3dBoolean: invalid clip type");
                    success = false;
                    break;
                }
            } catch (const std::exception& e) {
                Logger::error("clipNodeWithLine3dBoolean: boolean operation failed: " + std::string(e.what()));
                success = false;
            }

            if (success) {
                osg::Geometry* resultGeom = createOSGGeometryFromSurfaceMesh(*resultMesh, geom, hasTexCoords, precision);
                if (resultGeom) {
                    resultGeode->addDrawable(resultGeom);
                    successCount++;
                } else {
                    failureCount++;
                }
            } else {
                failureCount++;
            }

            delete subjectMesh;
            delete resultMesh;
        }, "clipNodeWithLine3dBoolean", i, failureCount);
    }

    return finalizeProcessing(resultGeode.release(), successCount, failureCount, "clipNodeWithLine3dBoolean");
}

} // namespace CGAL_OSG_TOOL_NS