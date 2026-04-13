#ifndef MESH_UTILS_H
#define MESH_UTILS_H

#include <CGAL/Surface_mesh.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <string>

namespace CGAL_OSG_TOOL_NS {

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Surface_mesh<K::Point_3> Surface_mesh;

/**
 * 计算网格的包围盒
 * @param mesh 输入网格
 * @param minPoint 输出最小点
 * @param maxPoint 输出最大点
 */
void computeMeshBounds(const Surface_mesh& mesh, K::Point_3& minPoint, K::Point_3& maxPoint);

/**
 * 计算网格的体积
 * @param mesh 输入网格
 * @return 网格体积
 */
double computeMeshVolume(const Surface_mesh& mesh);

/**
 * 计算网格的表面积
 * @param mesh 输入网格
 * @return 网格表面积
 */
double computeMeshSurfaceArea(const Surface_mesh& mesh);

/**
 * 检查网格是否为空
 * @param mesh 输入网格
 * @return 是否为空
 */
bool isMeshEmpty(const Surface_mesh& mesh);

/**
 * 检查网格是否为封闭网格
 * @param mesh 输入网格
 * @return 是否为封闭网格
 */
bool isMeshClosed(const Surface_mesh& mesh);

/**
 * 检查网格是否有自相交
 * @param mesh 输入网格
 * @return 是否有自相交
 */
bool hasMeshSelfIntersections(const Surface_mesh& mesh);

/**
 * 打印网格信息
 * @param mesh 输入网格
 * @param meshName 网格名称
 */
void printMeshInfo(const Surface_mesh& mesh, const std::string& meshName = "Mesh");

} // namespace CGAL_OSG_TOOL_NS

#endif // MESH_UTILS_H