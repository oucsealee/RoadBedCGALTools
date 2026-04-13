#ifndef TEXTURE_SAMPLER_H
#define TEXTURE_SAMPLER_H

#include <osg/Geometry>
#include <osg/Vec3>
#include <osg/Vec2>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/AABB_tree.h>
#include <CGAL/AABB_traits.h>
#include <CGAL/AABB_triangle_primitive.h>
#include <vector>
#include <unordered_map>
#include <memory>
#include "utils/logging.h"

namespace CGAL_OSG_TOOL_NS {

// 三角形数据结构
class TriangleCollector3D {
public:
    struct Triangle {
        osg::Vec3 v0, v1, v2;
        osg::Vec2 tc0, tc1, tc2;
        osg::Vec3 n0, n1, n2;
        osg::Vec4 c0, c1, c2;
    };

    std::vector<Triangle> triangles;
    osg::Vec2Array* texCoords = nullptr;
    osg::Vec3Array* vertices = nullptr;
    osg::Vec3Array* normals = nullptr;
    osg::Vec4Array* colors = nullptr;

    void operator()(unsigned int p1, unsigned int p2, unsigned int p3) {
        Triangle tri;
        if (!vertices || p1 >= vertices->size() || p2 >= vertices->size() || p3 >= vertices->size()) {
            Logger::error("TriangleCollector3D: invalid vertex indices");
            return;
        }
        tri.v0 = (*vertices)[p1];
        tri.v1 = (*vertices)[p2];
        tri.v2 = (*vertices)[p3];

        if (texCoords && p1 < texCoords->size())
            tri.tc0 = (*texCoords)[p1];
        if (texCoords && p2 < texCoords->size())
            tri.tc1 = (*texCoords)[p2];
        if (texCoords && p3 < texCoords->size())
            tri.tc2 = (*texCoords)[p3];

        if (normals && p1 < normals->size())
            tri.n0 = (*normals)[p1];
        if (normals && p2 < normals->size())
            tri.n1 = (*normals)[p2];
        if (normals && p3 < normals->size())
            tri.n2 = (*normals)[p3];

        if (colors && p1 < colors->size())
            tri.c0 = (*colors)[p1];
        if (colors && p2 < colors->size())
            tri.c1 = (*colors)[p2];
        if (colors && p3 < colors->size())
            tri.c2 = (*colors)[p3];

        triangles.push_back(tri);
    }
};

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
typedef CGAL::Triangle_3<K> Triangle_3;
typedef CGAL::AABB_triangle_primitive<K, std::vector<Triangle_3>::iterator> Primitive;
typedef CGAL::AABB_traits<K, Primitive> Traits;
typedef CGAL::AABB_tree<Traits> Tree;

/**
 * 纹理坐标采样器类，用于处理纹理坐标的计算
 */
class TextureSampler {
private:
    std::shared_ptr<std::vector<Triangle_3>> triangles;
    std::shared_ptr<std::vector<TriangleCollector3D::Triangle>> osgTriangles;
    std::shared_ptr<Tree> tree;
    bool _hasTexCoords = false;
    
    // AABB树缓存
    static std::unordered_map<osg::Geometry*, std::shared_ptr<Tree>> treeCache;
    static std::unordered_map<osg::Geometry*, std::shared_ptr<std::vector<Triangle_3>>> trianglesCache;
    static std::unordered_map<osg::Geometry*, std::shared_ptr<std::vector<TriangleCollector3D::Triangle>>> osgTrianglesCache;

public:
    /**
     * 初始化纹理采样器
     * @param originalGeom 原始OSG Geometry
     */
    TextureSampler(osg::Geometry* originalGeom);
    
    /**
     * 采样纹理坐标
     * @param point 要采样的3D点
     * @param tolerance 容差
     * @return 采样得到的纹理坐标
     */
    osg::Vec2 sample(const osg::Vec3& point, double tolerance) const;
    
    /**
     * 检查是否有纹理坐标
     * @return 是否有纹理坐标
     */
    bool hasTexCoords() const { 
        return _hasTexCoords;
    }
    
    /**
     * 清除缓存
     */
    static void clearCache();
};

} // namespace CGAL_OSG_TOOL_NS

#endif // TEXTURE_SAMPLER_H