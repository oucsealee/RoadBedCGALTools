#ifndef CLIP_3D_OPEN_H
#define CLIP_3D_OPEN_H

#include <osg/Node>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Plane>
#include <osg/Vec3>
#include <osg/Vec4>
#include <vector>

#include "api/cpp_api/roadbed_cgal_tools.h"

namespace CGAL_OSG_TOOL_NS {

/**
 * 向节点添加裁剪平面
 *
 * @param node 要添加裁剪平面的节点
 * @param plane 裁剪平面
 * @param clipPlaneIndex 裁剪平面索引
 * @return 成功返回true，失败返回false
 */
bool addClipPlaneToNode(osg::Node* node, const osg::Plane& plane, unsigned int clipPlaneIndex = 0);

/**
 * 向节点添加多个裁剪平面
 *
 * @param node 要添加裁剪平面的节点
 * @param planes 裁剪平面数组
 * @return 成功返回true，失败返回false
 */
bool addClipPlanesToNode(osg::Node* node, const std::vector<osg::Plane>& planes);

/**
 * 从节点移除裁剪平面
 *
 * @param node 要移除裁剪平面的节点
 * @param clipPlaneIndex 裁剪平面索引
 * @return 成功返回true，失败返回false
 */
bool removeClipPlaneFromNode(osg::Node* node, unsigned int clipPlaneIndex = 0);

/**
 * 从节点移除所有裁剪平面
 *
 * @param node 要移除裁剪平面的节点
 * @return 成功返回true，失败返回false
 */
bool removeAllClipPlanesFromNode(osg::Node* node);

/**
 * 简化平面裁剪（开放网格）
 *
 * @param node 要裁剪的OSG节点
 * @param plane 裁剪平面
 * @param vectorEpsilon 向量判断阈值
 * @param planeEpsilon 平面法线判断阈值
 * @param texCoordEpsilon 纹理坐标比较阈值
 * @param bboxEpsilon 边界框尺寸判断阈值
 * @return 裁剪后的OSG节点，失败返回nullptr
 *
 * @note 此函数使用OSG的裁剪平面功能进行简单裁剪
 * @note 适用于开放网格和封闭网格
 */
osg::Node* clipOpenMeshSimple(osg::Node* node, const osg::Plane& plane, double vectorEpsilon = 1e-10, double planeEpsilon = 1e-6, double texCoordEpsilon = 1e-10, double bboxEpsilon = 1e-10);

/**
 * 计算网格与平面的交线
 *
 * @param node 要计算交线的OSG节点
 * @param plane 平面
 * @param color 交线颜色
 * @param lineWidth 线宽
 * @return 交线的OSG Geometry，失败返回nullptr
 */
osg::Geometry* computeMeshPlaneIntersection(osg::Node* node, const osg::Plane& plane, const osg::Vec4& color = osg::Vec4(1.0, 0.0, 0.0, 1.0), double lineWidth = 2.0);

/**
 * 计算两个网格的交线
 *
 * @param meshA 第一个网格节点
 * @param meshB 第二个网格节点
 * @param color 交线颜色
 * @param lineWidth 线宽
 * @param tolerance 容差
 * @return 交线的OSG Geometry，失败返回nullptr
 */
osg::Geometry* computeMeshMeshIntersection(osg::Node* meshA, osg::Node* meshB, const osg::Vec4& color = osg::Vec4(1.0, 0.0, 0.0, 1.0), double lineWidth = 2.0, double tolerance = 1e-6);

/**
 * 计算网格-网格交线点集（多条独立折线）
 *
 * @param meshA 第一个网格节点
 * @param meshB 第二个网格节点
 * @param vectorEpsilon 向量判断阈值
 * @param planeEpsilon 平面法线判断阈值
 * @param texCoordEpsilon 纹理坐标比较阈值
 * @param bboxEpsilon 边界框尺寸判断阈值
 * @return 交线点集，每条交线是一个点的向量
 */
std::vector<std::vector<osg::Vec3>> computeMeshMeshIntersectionPoints(osg::Node* meshA, osg::Node* meshB, double vectorEpsilon = 1e-10, double planeEpsilon = 1e-6, double texCoordEpsilon = 1e-10, double bboxEpsilon = 1e-10);

} // namespace CGAL_OSG_TOOL_NS

#endif // CLIP_3D_OPEN_H