#ifndef CLIP_3D_CLOSED_H
#define CLIP_3D_CLOSED_H

#include <osg/Node>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Plane>
#include <osg/Vec3>
#include <osg/Vec4>

#include "api/cpp_api/roadbed_cgal_tools.h"

namespace CGAL_OSG_TOOL_NS {

/**
 * 完成处理并返回结果节点
 */
osg::Node* finalizeProcessing(osg::Geode* resultGeode, int successCount, int failureCount, const std::string& functionName);

/**
 * 使用Math::Line3d创建一个立方体网格
 *
 * @param line3d 3D线段，作为立方体的一条边
 * @param normal 平面的法线向量，作为立方体的高度方向
 * @param size 立方体大小，沿normal方向延伸size长度
 * @param precision 精度参数，默认使用默认精度
 * @return 创建的Surface_mesh网格指针
 *
 * @note 此函数创建一个以line3d为边，沿normal方向延伸size长度的立方体网格
 * @note 如果line3d是零线段或normal是零向量，返回nullptr
 * @note 如果line3d与normal平面平行，返回nullptr
 * @note 使用std::unique_ptr管理内存，确保内存安全
 */
std::unique_ptr<Surface_mesh> createSurfaceMeshFromLine3d(const Math::Line3d& line3d, const Math::Vector3d& normal, double size, const PrecisionParams& precision = PrecisionParams());

/**
 * 使用Math::Line3d创建一个立方体网格并转换为osg::Geometry
 *
 * @param line3d 3D线段，作为立方体的一条边
 * @param normal 平面的法线向量，作为立方体的高度方向
 * @param size 立方体大小，沿normal方向延伸size长度
 * @param color 立方体颜色，默认为白色
 * @param vectorEpsilon 向量判断阈值，默认1e-10
 * @param planeEpsilon 平面法线判断阈值，默认1e-6
 * @param texCoordEpsilon 纹理坐标比较阈值，默认1e-10
 * @param bboxEpsilon 边界框尺寸判断阈值，默认1e-10
 * @return 创建的osg::Geometry指针，失败返回nullptr
 *
 * @note 此函数创建一个以line3d为边，沿normal方向延伸size长度的立方体osg::Geometry
 * @note 如果line3d是零线段、normal是零向量或normal与线段方向平行，返回nullptr
 * @note 使用createSurfaceMeshFromLine3d创建Surface_mesh，然后转换为osg::Geometry
 */
osg::Geometry* createBoxGeometryFromLine3d(const Math::Line3d& line3d, const Math::Vector3d& normal, double size, const osg::Vec4& color = osg::Vec4(1.0, 1.0, 1.0, 1.0), double vectorEpsilon = 1e-10, double planeEpsilon = 1e-6, double texCoordEpsilon = 1e-10, double bboxEps = 1e-10);

/**
 * 使用Math::Line3d创建一个立方体网格并转换为osg::Geometry - osg::Vec3版本
 *
 * @param line3d 3D线段，作为立方体的一条边
 * @param normal 平面的法线向量，作为立方体的高度方向（osg::Vec3类型）
 * @param size 立方体大小，沿normal方向延伸size长度
 * @param color 立方体颜色，默认为白色
 * @param vectorEpsilon 向量判断阈值，默认1e-10
 * @param planeEpsilon 平面法线判断阈值，默认1e-6
 * @param texCoordEpsilon 纹理坐标比较阈值，默认1e-10
 * @param bboxEpsilon 边界框尺寸判断阈值，默认1e-10
 * @return 创建的osg::Geometry指针，失败返回nullptr
 *
 * @note 此函数创建一个以line3d为边，沿normal方向延伸size长度的立方体osg::Geometry
 * @note 如果line3d是零线段、normal是零向量或normal与线段方向平行，返回nullptr
 * @note 使用createSurfaceMeshFromLine3d创建Surface_mesh，然后转换为osg::Geometry
 */
osg::Geometry* createBoxGeometryFromLine3d(const Math::Line3d& line3d, const osg::Vec3& normal, double size, const osg::Vec4& color = osg::Vec4(1.0, 1.0, 1.0, 1.0), double vectorEpsilon = 1e-10, double planeEpsilon = 1e-6, double texCoordEpsilon = 1e-10, double bboxEps = 1e-10);

/**
 * 平面裁剪函数 (3D裁剪)
 * 使用CGAL进行3D网格裁剪，支持纹理和颜色信息
 *
 * @param node 要裁剪的OSG节点
 * @param plane 裁剪平面（由平面法线和一点定义）
 * @param vectorEpsilon 向量判断阈值
 * @param planeEpsilon 平面法线判断阈值
 * @param texCoordEpsilon 纹理坐标比较阈值
 * @param bboxEpsilon 边界框尺寸判断阈值
 * @return 裁剪后的OSG节点，失败返回nullptr
 * @note 裁剪算法基于CGAL布尔运算
 * @note 纹理处理：如果原始节点有纹理坐标，尝试保留纹理坐标，新生成的顶点使用投影计算
 * @note 如果原始节点没有纹理坐标，使用默认纹理坐标
 * @note 仅支持osg::Geode类型的节点
 * @note 注意：裁剪操作不会修改原始节点数据
*/
osg::Node* clipOSGNodeWithPlane(osg::Node* node, const osg::Plane& plane, double vectorEpsilon = 1e-10, double planeEpsilon = 1e-6, double texCoordEpsilon = 1e-10, double bboxEpsilon = 1e-10);

/**
 * 3D几何体裁剪（支持差集/交集/并集）
 * 使用CGAL进行3D布尔运算，支持纹理和颜色信息
 *
 * @param subjectNode 被裁剪的OSG节点
 * @param clipperNode 裁剪器OSG节点
 * @param clipType 裁剪类型: 0-差集(subject-clipper), 1-交集, 2-并集
 * @param vectorEpsilon 向量判断阈值，默认1e-10
 * @param planeEpsilon 平面法线判断阈值，默认1e-6
 * @param texCoordEpsilon 纹理坐标比较阈值，默认1e-10
 * @param bboxEpsilon 边界框尺寸判断阈值，默认1e-10
 * @return 裁剪后的OSG节点，失败返回nullptr
 *
 * @note 裁剪算法基于CGAL布尔运算
 * @note 纹理处理：如果原始节点有纹理坐标，尝试保留纹理坐标，新生成的顶点使用投影计算
 * @note 如果原始节点没有纹理坐标，使用默认纹理坐标
 * @note 仅支持osg::Geode类型的节点
 * @note 注意：裁剪操作不会修改原始节点数据
 * @note 支持三种裁剪类型：0-差集, 1-交集, 2-并集
 */
osg::Node* clipOSGGeometryWithGeometry(osg::Node* subjectNode, osg::Node* clipperNode, int clipType, double vectorEpsilon = 1e-10, double planeEpsilon = 1e-6, double texCoordEpsilon = 1e-10, double bboxEpsilon = 1e-10);

/**
 * 3D直线布尔运算裁剪
 * 使用3D直线创建一个立方体作为裁剪器，然后与原始几何体进行布尔运算
 *
 * @param node 要裁剪的OSG节点
 * @param line3d 3D直线
 * @param normal 裁剪方向（归一化法向量）
 * @param size 直线厚度（半宽度）
 * @param clipType 裁剪类型: 0-差集(node-clipper), 1-交集, 2-并集
 * @param vectorEpsilon 向量判断阈值，默认1e-10
 * @param planeEpsilon 平面法线判断阈值，默认1e-6
 * @param texCoordEpsilon 纹理坐标比较阈值，默认1e-10
 * @param bboxEpsilon 边界框尺寸判断阈值，默认1e-10
 * @return 裁剪后的OSG节点，失败返回nullptr
 *
 * @note 使用createSurfaceMeshFromLine3d创建立方体作为裁剪盒
 * @note 使用CGAL进行3D布尔运算
 * @note 支持三种裁剪类型：0-差集, 1-交集, 2-并集
 * @note 保留原始纹理坐标，新生成的顶点使用投影计算
 * @note 仅支持osg::Geode类型的节点
 */
osg::Node* clipNodeWithLine3dBoolean(osg::Node* node, const Math::Line3d& line3d, const Math::Vector3d& normal, double size, int clipType, double vectorEpsilon = 1e-10, double planeEpsilon = 1e-6, double texCoordEpsilon = 1e-10, double bboxEpsilon = 1e-10);

} // namespace CGAL_OSG_TOOL_NS

#endif // CLIP_3D_CLOSED_H