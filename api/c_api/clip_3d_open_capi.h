/**
 * @file clip_3d_open_capi.h
 * @brief C API 导出接口 for clip_3d_open
 */
 /* 所有复合类型直接使用 void* 传递，按指定内存布局解释
 *
 * ============================================================================
 * 功能摘要
 * ============================================================================
 *
 * 本模块提供开放网格（曲面）裁剪和交线计算的 C API 接口：
 *
 * 1. 裁剪平面操作
 *    - rb_add_clip_plane: 向节点添加裁剪平面
 *    - rb_add_clip_planes: 向节点添加多个裁剪平面
 *    - rb_remove_clip_plane: 从节点移除裁剪平面
 *    - rb_remove_all_clip_planes: 从节点移除所有裁剪平面
 *
 * 2. 平面裁剪
 *    - rb_clip_open_mesh_with_plane: 裁剪不封闭网格（保留边界线）
 *    - rb_clip_open_mesh_simple: 简化平面裁剪
 *
 * 3. 交线计算
 *    - rb_compute_mesh_plane_intersection: 计算网格与平面的交线
 *    - rb_compute_mesh_mesh_intersection: 计算两个网格的交线
 *    - rb_compute_mesh_mesh_intersection_points: 计算网格-网格交线点集
 *
 * 错误处理：
 * - 所有函数通过 rb_get_all_exec_infos() 获取执行信息
 * - 失败时返回 NULL
 */
#ifndef CLIP_3D_OPEN_CAPI_H
#define CLIP_3D_OPEN_CAPI_H

#include <stdbool.h>
#include <stddef.h>
#include "utils/error_handling.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 句柄类型
// ============================================================================

typedef void* RBNodeHandle;           /**< OSG 节点句柄 */
typedef void* RBGeometryHandle;       /**< OSG Geometry 句柄 */
typedef void* RBGroupHandle;          /**< OSG Group 句柄 */
typedef void* RBIntersectionHandle;   /**< 交线结果句柄 */

// ============================================================================
// 内存布局约定
// ============================================================================
//
// 输入参数（外部管理）：
//   plane:      void* -> const osg::Plane*
//   color:      void* -> const osg::Vec4*
//   line:       void* -> const Math::Line3d*
//   planes:     void* -> std::vector<osg::Plane>*
//
// 返回值（内部分配，外部负责释放）：
//   osg::Group*/osg::Node*   裁剪结果，由调用者管理生命周期
//   osg::Geometry*           交线结果，由调用者管理生命周期
//   vector<vector<Vec3>>*     折线点集，由调用者负责delete
//

// ============================================================================
// 裁剪平面操作
// ============================================================================

/**
 * 向节点添加裁剪平面
 * @param node 节点句柄
 * @param plane void* -> const osg::Plane*
 * @param clipPlaneIndex 平面索引
 * @return 成功返回 true
 */
__declspec(dllexport) bool rb_add_clip_plane(void* node, const void* plane, unsigned int clipPlaneIndex);

/**
 * 向节点添加多个裁剪平面
 * @param node 节点句柄
 * @param planes void* -> std::vector<osg::Plane>*
 * @return 成功返回 true
 */
__declspec(dllexport) bool rb_add_clip_planes(void* node, const void* planes);

/**
 * 从节点移除裁剪平面
 * @param node 节点句柄
 * @param clipPlaneIndex 平面索引
 * @return 成功返回 true
 */
__declspec(dllexport) bool rb_remove_clip_plane(void* node, unsigned int clipPlaneIndex);

/**
 * 从节点移除所有裁剪平面
 * @param node 节点句柄
 * @return 成功返回 true
 */
__declspec(dllexport) bool rb_remove_all_clip_planes(void* node);

// ============================================================================
// 平面裁剪
// ============================================================================
/**
 * 简化平面裁剪
 * @param node 节点句柄
 * @param plane void* -> const osg::Plane*
 * @param vectorEps 向量容差
 * @param planeEps 平面容差
 * @param texCoordEps 纹理坐标容差
 * @param bboxEps 包围盒容差
 * @return osg::Node* 句柄（内部分配，外部负责释放），失败返回 NULL
 */
__declspec(dllexport) void* rb_clip_open_mesh_simple(
    void* node,
    const void* plane,
    double vectorEps,
    double planeEps,
    double texCoordEps,
    double bboxEps);

// ============================================================================
// 网格-平面交线
// ============================================================================

/**
 * 计算网格与平面的交线
 * @param node 节点句柄
 * @param plane void* -> const osg::Plane*
 * @param color void* -> const osg::Vec4*
 * @param lineWidth 线宽
 * @return osg::Geometry* 句柄（内部分配，外部负责释放），失败返回 NULL
 */
__declspec(dllexport) void* rb_compute_mesh_plane_intersection(
    void* node,
    const void* plane,
    const void* color,
    double lineWidth);

// ============================================================================
// 网格-网格交线
// ============================================================================

/**
 * 计算两个网格的交线
 * @param meshA 网格A节点
 * @param meshB 网格B节点
 * @param color void* -> const osg::Vec4*
 * @param lineWidth 线宽
 * @param tolerance 容差
 * @return osg::Geometry* 句柄（内部分配，外部负责释放），失败返回 NULL
 */
__declspec(dllexport) void* rb_compute_mesh_mesh_intersection(
    void* meshA,
    void* meshB,
    const void* color,
    double lineWidth,
    double tolerance);

/**
 * 计算网格-网格交线点集（多条独立折线）
 * @param meshA 网格A节点
 * @param meshB 网格B节点
 * @param vectorEps 向量容差
 * @param planeEps 平面容差
 * @param texCoordEps 纹理坐标容差
 * @param bboxEps 包围盒容差
 * @return std::vector<std::vector<osg::Vec3>>* 句柄（内部分配，外部负责delete）
 */
__declspec(dllexport) void* rb_compute_mesh_mesh_intersection_points(
    void* meshA,
    void* meshB,
    double vectorEps,
    double planeEps,
    double texCoordEps,
    double bboxEps);

#ifdef __cplusplus
}
#endif

#endif // CLIP_3D_OPEN_CAPI_H
