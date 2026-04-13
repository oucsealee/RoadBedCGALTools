#include "texture_sampler.h"
#include <osg/TriangleIndexFunctor>

namespace CGAL_OSG_TOOL_NS {

// 静态成员初始化
std::unordered_map<osg::Geometry*, std::shared_ptr<Tree>> TextureSampler::treeCache;
std::unordered_map<osg::Geometry*, std::shared_ptr<std::vector<Triangle_3>>> TextureSampler::trianglesCache;
std::unordered_map<osg::Geometry*, std::shared_ptr<std::vector<TriangleCollector3D::Triangle>>> TextureSampler::osgTrianglesCache;

/**
 * 初始化纹理采样器
 */
TextureSampler::TextureSampler(osg::Geometry* originalGeom) {
    // 尝试从缓存中获取
    auto treeIt = treeCache.find(originalGeom);
    auto trianglesIt = trianglesCache.find(originalGeom);
    auto osgTrianglesIt = osgTrianglesCache.find(originalGeom);
    
    if (treeIt != treeCache.end() && trianglesIt != trianglesCache.end() && osgTrianglesIt != osgTrianglesCache.end()) {
        // 使用缓存的数据
        tree = treeIt->second;
        triangles = trianglesIt->second;
        osgTriangles = osgTrianglesIt->second;
        _hasTexCoords = osgTriangles && !osgTriangles->empty();
        return;
    }
    
    // 预处理原始几何体，构建AABB树和三角形数据
	osg::TriangleIndexFunctor<TriangleCollector3D> triangleFunctor;
	triangleFunctor.vertices = dynamic_cast<osg::Vec3Array*>(originalGeom->getVertexArray());
    if (osg::Array* parray = originalGeom->getTexCoordArray(0)) {
        triangleFunctor.texCoords = dynamic_cast<osg::Vec2Array*>(parray);
        _hasTexCoords = (triangleFunctor.texCoords != nullptr);
    }
    triangleFunctor.normals = dynamic_cast<osg::Vec3Array*>(originalGeom->getNormalArray());
    if (osg::Array* carray = originalGeom->getColorArray())
        triangleFunctor.colors = dynamic_cast<osg::Vec4Array*>(carray);
    
    originalGeom->accept(triangleFunctor);

    // 转换为CGAL三角形
    auto trianglesPtr = std::make_shared<std::vector<Triangle_3>>();
    for (const auto& tri : triangleFunctor.triangles) {
        trianglesPtr->push_back(Triangle_3(
            K::Point_3(tri.v0.x(), tri.v0.y(), tri.v0.z()),
            K::Point_3(tri.v1.x(), tri.v1.y(), tri.v1.z()),
            K::Point_3(tri.v2.x(), tri.v2.y(), tri.v2.z())
        ));
    }

    // 保存OSG三角形数据
    auto osgTrianglesPtr = std::make_shared<std::vector<TriangleCollector3D::Triangle>>(triangleFunctor.triangles);

    // 构建AABB树
    std::shared_ptr<Tree> treePtr;
    if (!trianglesPtr->empty()) {
        treePtr = std::make_shared<Tree>(trianglesPtr->begin(), trianglesPtr->end());
    }

    // 缓存数据
    if (treePtr) {
        treeCache[originalGeom] = treePtr;
    }
    trianglesCache[originalGeom] = trianglesPtr;
    osgTrianglesCache[originalGeom] = osgTrianglesPtr;

    // 赋值给成员变量
    tree = treePtr;
    triangles = trianglesPtr;
    osgTriangles = osgTrianglesPtr;
}

/**
 * 采样纹理坐标
 */
osg::Vec2 TextureSampler::sample(const osg::Vec3& point, double tolerance) const {
    if (!_hasTexCoords || !osgTriangles || osgTriangles->empty()) {
        return osg::Vec2(0.0, 0.0);
    }
    
    // 实现采样逻辑
    // 这里应该使用AABB树查找最近的三角形，然后进行纹理坐标插值
    // 暂时返回默认值，实际实现需要根据具体算法完成
    return osg::Vec2(0.0, 0.0);
}

/**
 * 清除缓存
 */
void TextureSampler::clearCache() {
    treeCache.clear();
    trianglesCache.clear();
    osgTrianglesCache.clear();
}

} // namespace CGAL_OSG_TOOL_NS
