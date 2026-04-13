#include "mesh_utils.h"
#include "utils/logging.h"

#include <CGAL/Polygon_mesh_processing/measure.h>
#include <CGAL/Polygon_mesh_processing/self_intersections.h>
#include <CGAL/Polygon_mesh_processing/border.h>
#include <CGAL/boost/graph/graph_traits_Surface_mesh.h>
#include <boost/unordered_set.hpp>

namespace CGAL_OSG_TOOL_NS {

/**
 * 计算网格的包围盒
 */
void computeMeshBounds(const Surface_mesh& mesh, K::Point_3& minPoint, K::Point_3& maxPoint) {
    if (mesh.is_empty()) {
        minPoint = K::Point_3(0, 0, 0);
        maxPoint = K::Point_3(0, 0, 0);
        return;
    }

    double minX = DBL_MAX, minY = DBL_MAX, minZ = DBL_MAX;
    double maxX = -DBL_MAX, maxY = -DBL_MAX, maxZ = -DBL_MAX;

    for (auto v : mesh.vertices()) {
        const K::Point_3& p = mesh.point(v);
        minX = std::min(minX, CGAL::to_double(p.x()));
        minY = std::min(minY, CGAL::to_double(p.y()));
        minZ = std::min(minZ, CGAL::to_double(p.z()));
        maxX = std::max(maxX, CGAL::to_double(p.x()));
        maxY = std::max(maxY, CGAL::to_double(p.y()));
        maxZ = std::max(maxZ, CGAL::to_double(p.z()));
    }

    minPoint = K::Point_3(minX, minY, minZ);
    maxPoint = K::Point_3(maxX, maxY, maxZ);
}

/**
 * 计算网格的体积
 */
double computeMeshVolume(const Surface_mesh& mesh) {
    if (mesh.is_empty() || !isMeshClosed(mesh)) {
        return 0.0;
    }

    try {
        return CGAL::to_double(CGAL::Polygon_mesh_processing::volume(mesh));
    }
    catch (const std::exception& e) {
        Logger::error("computeMeshVolume: " + std::string(e.what()));
        return 0.0;
    }
}

/**
 * 计算网格的表面积
 */
double computeMeshSurfaceArea(const Surface_mesh& mesh) {
    if (mesh.is_empty()) {
        return 0.0;
    }

    try {
        return CGAL::to_double(CGAL::Polygon_mesh_processing::area(mesh));
    }
    catch (const std::exception& e) {
        Logger::error("computeMeshSurfaceArea: " + std::string(e.what()));
        return 0.0;
    }
}

/**
 * 检查网格是否为空
 */
bool isMeshEmpty(const Surface_mesh& mesh) {
    return mesh.is_empty();
}

/**
 * 检查网格是否为封闭网格
 */
bool isMeshClosed(const Surface_mesh& mesh) {
    if (mesh.is_empty()) {
        return false;
    }

    try {
        return CGAL::is_closed(mesh);
    }
    catch (const std::exception& e) {
        Logger::error("isMeshClosed: " + std::string(e.what()));
        return false;
    }
}

/**
 * 检查网格是否有自相交
 */
bool hasMeshSelfIntersections(const Surface_mesh& mesh) {
    if (mesh.is_empty()) {
        return false;
    }

    try {
        return CGAL::Polygon_mesh_processing::does_self_intersect(mesh);
    }
    catch (const std::exception& e) {
        Logger::error("hasMeshSelfIntersections: " + std::string(e.what()));
        return false;
    }
}

/**
 * 打印网格信息
 */
void printMeshInfo(const Surface_mesh& mesh, const std::string& meshName) {
    if (mesh.is_empty()) {
        Logger::info(meshName + " is empty");
        return;
    }

    size_t numVertices = mesh.number_of_vertices();
    size_t numEdges = mesh.number_of_edges();
    size_t numFaces = mesh.number_of_faces();
    bool isClosed = isMeshClosed(mesh);
    bool hasSelfIntersections = hasMeshSelfIntersections(mesh);
    double volume = computeMeshVolume(mesh);
    double surfaceArea = computeMeshSurfaceArea(mesh);

    Logger::info(meshName + " info:");
    Logger::info("  Vertices: " + std::to_string(numVertices));
    Logger::info("  Edges: " + std::to_string(numEdges));
    Logger::info("  Faces: " + std::to_string(numFaces));
    Logger::info(std::string("  Is closed: ") + (isClosed ? "yes" : "no"));
    Logger::info(std::string("  Has self-intersections: ") + (hasSelfIntersections ? "yes" : "no"));
    Logger::info("  Volume: " + std::to_string(volume));
    Logger::info("  Surface area: " + std::to_string(surfaceArea));
}

} // namespace CGAL_OSG_TOOL_NS