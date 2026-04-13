#ifndef CLIP_2D_H
#define CLIP_2D_H

#include <osg/Node>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Vec2>
#include <osg/Vec3>
#include <vector>

#include "api/cpp_api/roadbed_cgal_tools.h"

namespace CGAL_OSG_TOOL_NS {

/**
 * 2D多边形裁剪
 *
 * @param node 要裁剪的OSG节点
 * @param clipper 裁剪多边形
 * @param vectorEpsilon 向量判断阈值
 * @param planeEpsilon 平面法线判断阈值
 * @param texCoordEpsilon 纹理坐标比较阈值
 * @param bboxEpsilon 边界框尺寸判断阈值
 * @return 裁剪后的OSG节点，失败返回nullptr
 *
 * @note 此函数使用CGAL进行2D多边形裁剪
 * @note 适用于2D网格和3D网格的2D投影
 */
osg::Node* clipOSGNodeWithPolygon(osg::Node* node, const std::vector<osg::Vec2>& clipper, double vectorEpsilon = 1e-10, double planeEpsilon = 1e-6, double texCoordEpsilon = 1e-10, double bboxEpsilon = 1e-10);

} // namespace CGAL_OSG_TOOL_NS

#endif // CLIP_2D_H