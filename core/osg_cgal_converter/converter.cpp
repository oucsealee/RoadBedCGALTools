#include "converter.h"
#include "utils/logging.h"

#include <osg/Node>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/PrimitiveSet>
#include <osg/TriangleIndexFunctor>
#include <osg/Array>
#include <osg/PrimitiveSet>
#include <osg/CullFace>
#include <osg/StateSet>

#include <vector>
#include <map>
#include <limits>
#include <cmath>
#include <memory>
#include <list>

#include <CGAL/Polygon_mesh_processing/corefinement.h>
#include <CGAL/Polygon_mesh_processing/clip.h>
#include <CGAL/Polygon_mesh_processing/self_intersections.h>
#include <CGAL/Polygon_mesh_processing/orientation.h>
#include <CGAL/Polygon_mesh_processing/repair.h>
#include <CGAL/Polygon_mesh_processing/intersection.h>
#include <CGAL/AABB_tree.h>
#include <CGAL/AABB_traits.h>
#include <CGAL/AABB_triangle_primitive.h>

namespace CGAL_OSG_TOOL_NS {

/**
 * 验证OSG节点是否为有效的Geode并包含drawables
 */
NodeValidationResult validateOSGNode(osg::Node* node, const std::string& functionName, const std::string& nodeName) {
    NodeValidationResult result;
    result.geode = nullptr;
    result.valid = false;

    if (!node) {
        result.errorMessage = functionName + ": " + nodeName + " is nullptr";
        return result;
    }

    osg::Geode* geode = dynamic_cast<osg::Geode*>(node);
    if (!geode) {
        result.errorMessage = functionName + ": " + nodeName + " is not a Geode";
        return result;
    }

    if (geode->getNumDrawables() == 0) {
        result.errorMessage = functionName + ": " + nodeName + " has no drawables";
        return result;
    }

    result.geode = geode;
    result.valid = true;
    return result;
}

/**
 * 验证Geometry是否有足够的顶点
 */
bool validateGeometry(osg::Geometry* geom, const std::string& functionName, unsigned int geomIndex, int minVertices) {
    if (!geom) {
        return false;
    }

    osg::Vec3Array* vertices = dynamic_cast<osg::Vec3Array*>(geom->getVertexArray());
    if (!vertices || vertices->size() < static_cast<size_t>(minVertices)) {
        Logger::warning((functionName + ": geometry " + std::to_string(geomIndex)
            + " has insufficient vertices"));
        return false;
    }

    return true;
}

/**
 * 处理Geometry级别的异常
 */
void handleGeometryException(const std::exception& e, const std::string& exceptionType, const std::string& functionName, unsigned int geomIndex, int& failureCount) {
    Logger::error((functionName + ": " + exceptionType + " failed for geometry " + std::to_string(geomIndex)
        + ": " + e.what()));
    failureCount++;
}

/**
 * 统一处理Geometry处理过程中的异常
 * 封装了四种标准异常类型的捕获逻辑
 */
void tryProcessGeometryWithExceptionHandling(const std::function<void()>& processFunc, const std::string& functionName, unsigned int geomIndex, int& failureCount) {
    try {
        processFunc();
    }
    catch (const CGAL::Assertion_exception& e) {
        handleGeometryException(e, "CGAL assertion", functionName, geomIndex, failureCount);
    }
    catch (const std::bad_alloc& e) {
        handleGeometryException(e, "memory allocation", functionName, geomIndex, failureCount);
    }
    catch (const std::exception& e) {
        handleGeometryException(e, "exception", functionName, geomIndex, failureCount);
    }
    catch (...) {
        Logger::error((functionName + ": unknown exception for geometry " + std::to_string(geomIndex)));
        failureCount++;
    }
}

// 顶点属性（位置、纹理坐标和颜色）
struct VertexWithTexCoords {
    osg::Vec3 position;
    osg::Vec2 texCoord;
    osg::Vec4 color;
    const PrecisionParams* precision;

    VertexWithTexCoords() : precision(nullptr) {}
    VertexWithTexCoords(const PrecisionParams* params) : precision(params) {}

    bool operator<(const VertexWithTexCoords& other) const {
        double eps = precision ? precision->texCoordEpsilon : 1e-10;
        if (std::abs(position.x() - other.position.x()) > eps) return position.x() < other.position.x();
        if (std::abs(position.y() - other.position.y()) > eps) return position.y() < other.position.y();
        if (std::abs(position.z() - other.position.z()) > eps) return position.z() < other.position.z();
        if (std::abs(texCoord.x() - other.texCoord.x()) > eps) return texCoord.x() < other.texCoord.x();
        if (std::abs(texCoord.y() - other.texCoord.y()) > eps) return texCoord.y() < other.texCoord.y();
        return color < other.color;
    }

    bool operator==(const VertexWithTexCoords& other) const {
        double eps = precision ? precision->texCoordEpsilon : 1e-10;
        return std::abs(position.x() - other.position.x()) < eps &&
            std::abs(position.y() - other.position.y()) < eps &&
            std::abs(position.z() - other.position.z()) < eps &&
            std::abs(texCoord.x() - other.texCoord.x()) < eps &&
            std::abs(texCoord.y() - other.texCoord.y()) < eps &&
            std::abs(color.r() - other.color.r()) < eps &&
            std::abs(color.g() - other.color.g()) < eps &&
            std::abs(color.b() - other.color.b()) < eps &&
            std::abs(color.a() - other.color.a()) < eps;
    }
};

/**
 * 提取三角形数据
 */
TriangleCollector3D extractTriangles(osg::Geometry* geom) {
	osg::TriangleIndexFunctor<TriangleCollector3D> triangleFunctor;
	triangleFunctor.vertices = dynamic_cast<osg::Vec3Array*>(geom->getVertexArray());
    if (osg::Array* parray = geom->getTexCoordArray(0))
        triangleFunctor.texCoords = dynamic_cast<osg::Vec2Array*>(parray);
    triangleFunctor.normals = dynamic_cast<osg::Vec3Array*>(geom->getNormalArray());
    if (osg::Array* carray = geom->getColorArray())
        triangleFunctor.colors = dynamic_cast<osg::Vec4Array*>(carray);
    
    geom->accept(triangleFunctor);
    return triangleFunctor;
}

/**
 * 构建顶点映射
 */
std::map<osg::Vec3, Surface_mesh::Vertex_index, Vec3Comparator> buildVertexMap(
    const TriangleCollector3D& triangleCollector, 
    Surface_mesh* mesh, 
    const PrecisionParams& precision) {
    std::map<osg::Vec3, Surface_mesh::Vertex_index, Vec3Comparator> positionMap(&precision);
    
    for (const auto& tri : triangleCollector.triangles) {
        osg::Vec3 vertices[3] = { tri.v0, tri.v1, tri.v2 };
        
        for (int i = 0; i < 3; ++i) {
            auto it = positionMap.find(vertices[i]);
            if (it == positionMap.end()) {
                positionMap[vertices[i]] = mesh->add_vertex(K::Point_3(vertices[i].x(), vertices[i].y(), vertices[i].z()));
            }
        }
    }
    
    return positionMap;
}

/**
 * 创建Surface_mesh
 */
void createSurfaceMesh(Surface_mesh* mesh, const TriangleCollector3D& triangleCollector, 
                      const std::map<osg::Vec3, Surface_mesh::Vertex_index, Vec3Comparator>& positionMap) {
    for (const auto& tri : triangleCollector.triangles) {
        osg::Vec3 vertices[3] = { tri.v0, tri.v1, tri.v2 };
        Surface_mesh::Vertex_index vi[3];

        for (int i = 0; i < 3; ++i) {
            auto it = positionMap.find(vertices[i]);
            if (it != positionMap.end()) {
                vi[i] = it->second;
            }
        }

        mesh->add_face(vi[0], vi[1], vi[2]);
    }
}

/**
 * 将OSG Geometry转换为CGAL Surface_mesh
 */
Surface_mesh* geometryToSurfaceMesh(osg::Geometry* geom,
                                   bool& hasTexCoords, 
                                   bool bclose_model,
                                   const PrecisionParams& precision) {
    // 提取三角形数据
    TriangleCollector3D triangleFunctor = extractTriangles(geom);
    
    // 检查是否有纹理坐标
    hasTexCoords = (triangleFunctor.texCoords != nullptr);

    auto mesh = std::make_unique<Surface_mesh>();

    // 构建顶点映射
    std::map<osg::Vec3, Surface_mesh::Vertex_index, Vec3Comparator> positionMap = buildVertexMap(triangleFunctor, mesh.get(), precision);

    // 创建Surface_mesh
    createSurfaceMesh(mesh.get(), triangleFunctor, positionMap);

    // 验证和修复mesh
    if (!validateAndRepairMesh(mesh.get(), bclose_model, "geometryToSurfaceMesh")) {
        return nullptr;
    }
    return mesh.release();
}

/**
 * 验证和修复Surface_mesh
 */
bool validateAndRepairMesh(Surface_mesh* mesh, bool bclose_model, const std::string& functionName) {
    // 检查mesh有效性
    if (mesh->is_empty()) {
        Logger::error((functionName + ": mesh is empty"));
        return false;
    }

    // 第一步：先检查所有问题
    bool isValid = mesh->is_valid();
    bool isClosed = true;
    if (bclose_model)
        isClosed = CGAL::is_closed(*mesh);
    bool isOriented = true;
    bool hasSelfIntersection = false;

    // 检查法线方向
    if (bclose_model)
    {
        try {
            isOriented = CGAL::Polygon_mesh_processing::is_outward_oriented(*mesh);
        }
        catch (...) {
            Logger::error((functionName + ": failed to check orientation"));
            return false;
        }
    }

    // 检查自相交
    try {
        hasSelfIntersection = CGAL::Polygon_mesh_processing::does_self_intersect(*mesh);
    }
    catch (...) {
        Logger::error((functionName + ": failed to check self-intersection"));
        return false;
    }

    // 输出检查结果
    if (!isValid) {
        Logger::warning((functionName + ": mesh is invalid"));
    }
    if (!isClosed) {
        Logger::warning((functionName + ": mesh is not closed"));
    }
    if (!isOriented) {
        Logger::warning((functionName + ": mesh is not outward oriented"));
    }
    if (hasSelfIntersection) {
        Logger::warning((functionName + ": mesh is self-intersecting"));
    }

    // 第二步：根据检查结果决定修复过程
    bool needRepair = !isValid || !isOriented || hasSelfIntersection;
    if (bclose_model)
        needRepair = needRepair || !isClosed;
    if (needRepair) {
        Logger::info((functionName + ": attempting mesh repair..."));
        try {
            // 基础修复（只执行一次）
            // 1. 移除退化面：删除面积为零或顶点重复的面
            CGAL::Polygon_mesh_processing::remove_degenerate_faces(*mesh);
            // 2. 缝合边界：合并共享边但未连接的相邻三角形面片，消除裂缝/间隙
            //    处理因浮点精度问题导致本应相连的面断开的情况
            CGAL::Polygon_mesh_processing::stitch_borders(*mesh);
            // 3. 移除孤立顶点：删除不属于任何面的游离顶点
            CGAL::Polygon_mesh_processing::remove_isolated_vertices(*mesh);
            // 4. 定向到包围体：确保所有面法线向外，使网格可封闭并支持布尔运算
            if (bclose_model)
                CGAL::Polygon_mesh_processing::orient_to_bound_a_volume(*mesh);

            // 自相交修复（有自相交时执行）
            if (hasSelfIntersection) {
                CGAL::Polygon_mesh_processing::experimental::remove_self_intersections(*mesh);
            }

            Logger::info((functionName + ": mesh repair successful"));
        }
        catch (...) {
            Logger::error((functionName + ": mesh repair exception"));
            return false;
        }

        // 修复后重新验证
        if (bclose_model && !isOriented && !CGAL::Polygon_mesh_processing::is_outward_oriented(*mesh)) {
            Logger::error((functionName + ": mesh repair failed, still not outward oriented"));
            return false;
        }
        if (hasSelfIntersection && CGAL::Polygon_mesh_processing::does_self_intersect(*mesh)) {
            Logger::error((functionName + ": mesh repair failed, still self-intersecting"));
            return false;
        }
    }

    return true;
}

/**
 * 将OSG Geode转换为CGAL Surface_mesh（合并所有Geometry）
 */
bool geodeToSurfaceMesh(osg::Geode* geode, Surface_mesh& cgalMesh, bool& hasTexCoords, bool bclose_model, 
                       const std::string& meshName, const PrecisionParams& precision_param) {
    // 初始化hasTexCoords为false
    hasTexCoords = false;
    
    // 遍历Geode中的所有Geometry
    for (unsigned int i = 0; i < geode->getNumDrawables(); ++i) {
        osg::Geometry* geom = geode->getDrawable(i)->asGeometry();
        if (!geom) continue;

        bool geomHasTexCoords = false;
        Surface_mesh* tempMesh = geometryToSurfaceMesh(geom, geomHasTexCoords, bclose_model, precision_param);
        if (!tempMesh) {
            Logger::warning("geodeToSurfaceMesh: failed to convert geometry " + 
                std::to_string(i));
            continue;
        }

        // 如果任何Geometry有纹理坐标，则整体hasTexCoords为true
        hasTexCoords = hasTexCoords || geomHasTexCoords;

        // 将tempMesh的顶点和面合并到cgalMesh
        std::map<Surface_mesh::Vertex_index, Surface_mesh::Vertex_index> vertexIndexMap;
        for (auto v : tempMesh->vertices()) {
            const K::Point_3& point = tempMesh->point(v);
            Surface_mesh::Vertex_index newV = cgalMesh.add_vertex(point);
            vertexIndexMap[v] = newV;
        }

        for (auto f : tempMesh->faces()) {
            auto hf = tempMesh->halfedge(f);
            auto v0 = tempMesh->target(tempMesh->prev(hf));
            auto v1 = tempMesh->target(hf);
            auto v2 = tempMesh->target(tempMesh->next(hf));

            if (vertexIndexMap.find(v0) != vertexIndexMap.end() &&
                vertexIndexMap.find(v1) != vertexIndexMap.end() &&
                vertexIndexMap.find(v2) != vertexIndexMap.end()) {
                cgalMesh.add_face(vertexIndexMap[v0], vertexIndexMap[v1], vertexIndexMap[v2]);
            }
        }

        delete tempMesh;
    }

    if (!meshName.empty()) {
        Logger::info("geodeToSurfaceMesh (" + meshName + "): " +
            std::to_string(cgalMesh.number_of_vertices()) + " vertices, " +
            std::to_string(cgalMesh.number_of_faces()) + " faces" +
            (hasTexCoords ? ", has texture coordinates" : ""));
    }

    return cgalMesh.number_of_faces() > 0;
}

/**
 * 提取顶点数据
 */
osg::ref_ptr<osg::Vec3Array> extractVertices(Surface_mesh& mesh, std::map<Surface_mesh::Vertex_index, int>& vertexIndexMap) {
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    vertices->reserve(mesh.number_of_vertices());
    
    int idx = 0;
    for (auto v : mesh.vertices()) {
        const K::Point_3& p = mesh.point(v);
        vertices->push_back(osg::Vec3(CGAL::to_double(p.x()), CGAL::to_double(p.y()), CGAL::to_double(p.z())));
        vertexIndexMap[v] = idx++;
    }
    
    return vertices;
}

/**
 * 计算法线
 */
osg::ref_ptr<osg::Vec3Array> calculateNormals(Surface_mesh& mesh) {
    osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
    normals->reserve(mesh.number_of_vertices());
    
    // 计算顶点法线
    auto vnormals = mesh.add_property_map<Surface_mesh::Vertex_index, K::Vector_3>("v:normals", K::Vector_3(0, 0, 0)).first;
    CGAL::Polygon_mesh_processing::compute_vertex_normals(mesh, vnormals);

    for (auto v : mesh.vertices()) {
        const K::Vector_3& n = vnormals[v];
        normals->push_back(osg::Vec3(CGAL::to_double(n.x()),
            CGAL::to_double(n.y()),
            CGAL::to_double(n.z())));
    }
    
    return normals;
}

/**
 * 生成纹理坐标
 */
osg::ref_ptr<osg::Vec2Array> generateTextureCoords(Surface_mesh& mesh, osg::Geometry* originalGeom, bool hasTexCoords, const PrecisionParams& precision) {
    if (!hasTexCoords) {
        return nullptr;
    }
    
    osg::ref_ptr<osg::Vec2Array> texCoords = new osg::Vec2Array;
    texCoords->reserve(mesh.number_of_vertices());
    
    // 创建纹理采样器
    TextureSampler textureSampler(originalGeom);
    
    if (textureSampler.hasTexCoords()) {
        for (auto v : mesh.vertices()) {
            const K::Point_3& p = mesh.point(v);
            osg::Vec3 osgPoint(CGAL::to_double(p.x()), CGAL::to_double(p.y()), CGAL::to_double(p.z()));
            osg::Vec2 sampledTexCoord = textureSampler.sample(osgPoint, precision.planeEpsilon);
            texCoords->push_back(sampledTexCoord);
        }
    }
    
    return texCoords;
}

/**
 * 构建OSG Geometry
 */
osg::Geometry* buildGeometry(osg::ref_ptr<osg::Vec3Array> vertices, osg::ref_ptr<osg::Vec3Array> normals, 
                           osg::ref_ptr<osg::Vec2Array> texCoords, Surface_mesh& mesh, 
                           const std::map<Surface_mesh::Vertex_index, int>& vertexIndexMap) {
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    
    // 使用索引方式构建三角形
    std::vector<unsigned int> indices;
    for (auto f : mesh.faces()) {
        std::vector<int> faceVertices;
        for (auto v : CGAL::vertices_around_face(mesh.halfedge(f), mesh)) {
            faceVertices.push_back(vertexIndexMap.at(v));
        }
        // 将多边形面三角化（扇形）
        if (faceVertices.size() >= 3) {
            for (size_t i = 1; i < faceVertices.size() - 1; ++i) {
                indices.push_back(faceVertices[0]);
                indices.push_back(faceVertices[i]);
                indices.push_back(faceVertices[i + 1]);
            }
        }
    }
    
    geom->setVertexArray(vertices);
    geom->setNormalArray(normals);
    geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);

    if (texCoords) {
        geom->setTexCoordArray(0, texCoords);
    }

    // 使用DrawElements进行索引绘制
    osg::ref_ptr<osg::DrawElementsUInt> drawElements = new osg::DrawElementsUInt(GL_TRIANGLES);
    for (auto i : indices) {
        drawElements->push_back(i);
    }
    geom->addPrimitiveSet(drawElements);

    geom->setUseDisplayList(false);
    geom->setUseVertexBufferObjects(true);

    // 设置背面裁剪状态
    osg::ref_ptr<osg::StateSet> stateset = geom->getOrCreateStateSet();
    stateset->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
    stateset->setAttributeAndModes(new osg::CullFace(osg::CullFace::BACK), osg::StateAttribute::ON);

    return geom.release();
}

/**
 * 将Surface_mesh转换回OSG Geometry（通过空间插值采样纹理坐标）
 */
osg::Geometry* createOSGGeometryFromSurfaceMesh(Surface_mesh& mesh,
                                                 osg::Geometry* originalGeom,
                                                 bool hasTexCoords,
                                                 const PrecisionParams& precision) {
    if (mesh.is_empty()) {
        Logger::error("createOSGGeometryFromSurfaceMesh: mesh is empty");
        return nullptr;
    }

    // 提取顶点数据
    std::map<Surface_mesh::Vertex_index, int> vertexIndexMap;
    osg::ref_ptr<osg::Vec3Array> vertices = extractVertices(mesh, vertexIndexMap);
    
    // 计算法线
    osg::ref_ptr<osg::Vec3Array> normals = calculateNormals(mesh);
    
    // 生成纹理坐标
    osg::ref_ptr<osg::Vec2Array> texCoords = generateTextureCoords(mesh, originalGeom, hasTexCoords, precision);
    
    // 构建OSG Geometry
    return buildGeometry(vertices, normals, texCoords, mesh, vertexIndexMap);
}

// ============================================================================
// 2D 转换函数 (OSG Geometry - CGAL Polygon_2)
// ============================================================================

// 2D 三角形数据结构
class TriangleCollector2D {
public:
    struct Triangle {
        osg::Vec3 v0, v1, v2;
        osg::Vec2 tc0, tc1, tc2;
        bool hasTexCoords;
    };

    std::vector<Triangle> triangles;
    osg::Vec2Array* texCoords = nullptr;
    osg::Vec3Array* vertices = nullptr;

    void operator()(unsigned int p1, unsigned int p2, unsigned int p3) {
        Triangle tri;
        tri.v0 = (*vertices)[p1];
        tri.v1 = (*vertices)[p2];
        tri.v2 = (*vertices)[p3];

        // 检查是否有纹理坐标
        tri.hasTexCoords = (texCoords != nullptr);

        if (tri.hasTexCoords) {
            if (p1 < texCoords->size())
                tri.tc0 = (*texCoords)[p1];
            if (p2 < texCoords->size())
                tri.tc1 = (*texCoords)[p2];
            if (p3 < texCoords->size())
                tri.tc2 = (*texCoords)[p3];
        }
        else {
            // 没有纹理坐标时，设置为默认值
            tri.tc0 = tri.tc1 = tri.tc2 = osg::Vec2(0.0, 0.0);
        }

        triangles.push_back(tri);
    }
};

/**
 * 将OSG Geometry转换为三角面片 (2D)
 */
std::vector<TriangleCollector2D::Triangle> geometryToTriangles2D(osg::Geometry* geom) {
    osg::TriangleIndexFunctor<TriangleCollector2D> triangleFunctor;
    triangleFunctor.vertices = dynamic_cast<osg::Vec3Array*>(geom->getVertexArray());
    if (osg::Array* parray = geom->getTexCoordArray(0))
        triangleFunctor.texCoords = dynamic_cast<osg::Vec2Array*>(parray);

    geom->accept(triangleFunctor);

    return triangleFunctor.triangles;
}

/**
 * 从三角面片提取CGAL多边形 (2D)
 */
std::vector<Polygon_2> extractPolygonsFromGeometry(osg::Geometry* geom) {
    std::vector<TriangleCollector2D::Triangle> triangles = geometryToTriangles2D(geom);
    std::vector<Polygon_2> polygons;

    for (const auto& tri : triangles) {
        Polygon_2 polygon;
        polygon.push_back(K::Point_2(tri.v0.x(), tri.v0.y()));
        polygon.push_back(K::Point_2(tri.v1.x(), tri.v1.y()));
        polygon.push_back(K::Point_2(tri.v2.x(), tri.v2.y()));
        polygons.push_back(polygon);
    }

    return polygons;
}

/**
 * 使用CGAL进行多边形裁剪 (2D)
 */
std::vector<Polygon_2> clipPolygons(const std::vector<Polygon_2>& subjects, const Polygon_2& clipper) {
    std::vector<Polygon_2> results;

    for (const auto& subject : subjects) {
        Polygon_set ps(subject);
        ps.intersection(clipper);

        std::list<Polygon_with_holes_2> res;
        ps.polygons_with_holes(std::back_inserter(res));

        for (const auto& poly : res) {
            results.push_back(poly.outer_boundary());
        }
    }

    return results;
}

/**
 * 将CGAL多边形转换为OSG Geometry (2D)
 */
osg::Geometry* createOSGGeometryFromPolygons(const std::vector<Polygon_2>& polygons,
                                               bool hasOriginalTexCoords) {
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec2Array> texCoords = nullptr;

    // 只有当原始有纹理坐标时才创建纹理坐标数组
    if (hasOriginalTexCoords) {
        texCoords = new osg::Vec2Array;
    }

    // 为每个多边形生成顶点和纹理坐标
    for (const auto& polygon : polygons) {
        if (polygon.is_empty() || polygon.size() < 3) continue;

        // 计算多边形的包围盒用于生成纹理坐标
        double minX = DBL_MAX, maxX = DBL_MIN;
        double minY = DBL_MAX, maxY = DBL_MIN;

        for (auto it = polygon.vertices_begin(); it != polygon.vertices_end(); ++it) {
            double x = CGAL::to_double(it->x());
            double y = CGAL::to_double(it->y());
            minX = std::min(minX, x);
            maxX = std::max(maxX, x);
            minY = std::min(minY, y);
            maxY = std::max(maxY, y);
        }

        double width = maxX - minX;
        double height = maxY - minY;

        // 避免除零
        if (width < 1e-10) width = 1.0;
        if (height < 1e-10) height = 1.0;

        // 三角化多边形（扇形）
        auto it = polygon.vertices_begin();
        K::Point_2 first = *it;
        double firstU = (CGAL::to_double(first.x()) - minX) / width;
        double firstV = (CGAL::to_double(first.y()) - minY) / height;

        ++it;
        K::Point_2 prev = *it;
        double prevU = (CGAL::to_double(prev.x()) - minX) / width;
        double prevV = (CGAL::to_double(prev.y()) - minY) / height;

        ++it;

        for (; it != polygon.vertices_end(); ++it) {
            vertices->push_back(osg::Vec3(CGAL::to_double(first.x()), CGAL::to_double(first.y()), 0.0));
            vertices->push_back(osg::Vec3(CGAL::to_double(prev.x()), CGAL::to_double(prev.y()), 0.0));
            vertices->push_back(osg::Vec3(CGAL::to_double(it->x()), CGAL::to_double(it->y()), 0.0));

            // 只有在有纹理坐标时才添加，根据包围盒重新生成
            if (hasOriginalTexCoords) {
                texCoords->push_back(osg::Vec2(firstU, firstV));

                double currU = (CGAL::to_double(it->x()) - minX) / width;
                double currV = (CGAL::to_double(it->y()) - minY) / height;

                texCoords->push_back(osg::Vec2(prevU, prevV));
                texCoords->push_back(osg::Vec2(currU, currV));

                prevU = currU;
                prevV = currV;
            }

            prev = *it;
        }
    }

    geom->setVertexArray(vertices);

    // 只有在有纹理坐标时才设置
    if (hasOriginalTexCoords) {
        geom->setTexCoordArray(0, texCoords);
    }

    geom->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, vertices->size()));

    return geom.release();
}

} // namespace CGAL_OSG_TOOL_NS