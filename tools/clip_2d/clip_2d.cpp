#include "clip_2d.h"
#include "../clip_3d_closed/clip_3d_closed.h"
#include "core/osg_cgal_converter/converter.h"
#include "core/geometry/mesh_utils.h"
#include "utils/error_handling.h"
#include "utils/logging.h"

#include <osg/Node>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Vec2>
#include <osg/Vec3>
#include <osg/TriangleIndexFunctor>
#include <osg/Array>
#include <osg/PrimitiveSet>

#include <vector>
#include <memory>
#include <string>
#include <cmath>

namespace CGAL_OSG_TOOL_NS {

/**
 * 2D三角形数据结构
 */
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
 * 2D多边形裁剪
 */
osg::Node* clipOSGNodeWithPolygon(osg::Node* node, const std::vector<osg::Vec2>& clipper, double vectorEpsilon, double planeEpsilon, double texCoordEpsilon, double bboxEpsilon) {
    if (!node) {
        Logger::error("clipOSGNodeWithPolygon: node is nullptr");
        return nullptr;
    }

    // 验证输入节点
    NodeValidationResult validationResult = validateOSGNode(node, "clipOSGNodeWithPolygon");
    if (!validationResult.valid) {
        Logger::error(validationResult.errorMessage);
        return nullptr;
    }
    osg::Geode* geode = validationResult.geode;

    // 创建裁剪多边形
    Polygon_2 clipperPolygon;
    for (const auto& point : clipper) {
        clipperPolygon.push_back(K::Point_2(point.x(), point.y()));
    }

    if (clipperPolygon.is_empty() || clipperPolygon.size() < 3) {
        Logger::error("clipOSGNodeWithPolygon: invalid clipper polygon");
        return nullptr;
    }

    osg::ref_ptr<osg::Geode> resultGeode = new osg::Geode;
    int successCount = 0, failureCount = 0;

    // 遍历Geode中的所有Geometry
    for (unsigned int i = 0; i < geode->getNumDrawables(); ++i) {
        osg::Geometry* geom = geode->getDrawable(i)->asGeometry();
        if (!geom || !validateGeometry(geom, "clipOSGNodeWithPolygon", i)) {
            failureCount++;
            continue;
        }

        // 尝试处理Geometry，捕获异常
        tryProcessGeometryWithExceptionHandling([&]() {
            // 提取2D多边形
            std::vector<Polygon_2> polygons = extractPolygonsFromGeometry(geom);
            if (polygons.empty()) {
                failureCount++;
                return;
            }

            // 裁剪多边形
            std::vector<Polygon_2> clippedPolygons = clipPolygons(polygons, clipperPolygon);
            if (clippedPolygons.empty()) {
                failureCount++;
                return;
            }

            // 检查是否有纹理坐标
            bool hasTexCoords = (geom->getTexCoordArray(0) != nullptr);

            // 转换回OSG Geometry
            osg::Geometry* resultGeom = createOSGGeometryFromPolygons(clippedPolygons, hasTexCoords);
            if (resultGeom) {
                resultGeode->addDrawable(resultGeom);
                successCount++;
            } else {
                failureCount++;
            }
        }, "clipOSGNodeWithPolygon", i, failureCount);
    }

    return finalizeProcessing(resultGeode.release(), successCount, failureCount, "clipOSGNodeWithPolygon");
}

} // namespace CGAL_OSG_TOOL_NS