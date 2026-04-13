/**
 * @file clip_3d_closed_capi.h
 * @brief C API 导出接口 for clip_3d_closed
 */
 /* 所有复合类型直接使用 void* 传递，按指定内存布局解释
 *
 * ============================================================================
 * 功能摘要
 * ============================================================================
 *
 * 本模块提供封闭网格裁剪的 C API 接口：
 *
 * 1. 平面裁剪
 *    - rb_clip_closed_node_with_plane: 使用单个平面裁剪封闭网格
 *
 * 2. 几何体裁剪
 *    - rb_clip_closed_node_with_geometry: 使用另一个几何体裁剪，支持交集/差集/并集运算
 *
 * 3. 3D 直线裁剪
 *    - rb_clip_closed_node_with_line3d: 使用3D直线布尔运算裁剪封闭网格
 *
 * 错误处理：
 * - 所有函数通过 rb_get_all_exec_infos() 获取执行信息
 * - 失败时返回 NULL
 */
#ifndef CLIP_3D_CLOSED_CAPI_H
#define CLIP_3D_CLOSED_CAPI_H

#include <stdbool.h>
#include <stddef.h>
#include "utils/error_handling.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// 内存布局约定
// ============================================================================
//
// 输入参数（外部管理）：
//   plane:      void* -> const osg::Plane*
//   line:       void* -> const Math::Line3d*
//   subjectNode: void* -> osg::Node* (被裁剪的节点)
//   clipperNode: void* -> osg::Node* (裁剪用的几何体节点)
//
// 返回值（内部分配，外部负责释放）：
//   osg::Node*  裁剪结果，由调用者管理生命周期
//

// ============================================================================
// 平面裁剪
// ============================================================================

/**
 * 使用平面裁剪OSG节点（封闭网格）
 * @param node 节点句柄
 * @param plane void* -> const osg::Plane*
 * @param vectorEps 向量容差
 * @param planeEps 平面容差
 * @param texCoordEps 纹理坐标容差
 * @param bboxEps 包围盒容差
 * @return osg::Node* 句柄（内部分配，外部负责释放），失败返回 NULL
 */
__declspec(dllexport) void* rb_clip_closed_node_with_plane(
    void* node,
    const void* plane,
    double vectorEps,
    double planeEps,
    double texCoordEps,
    double bboxEps);

// ============================================================================
// 几何体裁剪
// ============================================================================

/**
 * 使用几何体裁剪OSG节点（封闭网格）
 * @param subjectNode 被裁剪的节点句柄
 * @param clipperNode 裁剪几何体节点句柄
 * @param clipType 裁剪类型：0=交集, 1=差集, 2=并集
 * @param vectorEps 向量容差
 * @param planeEps 平面容差
 * @param texCoordEps 纹理坐标容差
 * @param bboxEps 包围盒容差
 * @return osg::Node* 句柄（内部分配，外部负责释放），失败返回 NULL
 */
__declspec(dllexport) void* rb_clip_closed_node_with_geometry(
    void* subjectNode,
    void* clipperNode,
    int clipType,
    double vectorEps,
    double planeEps,
    double texCoordEps,
    double bboxEps);

// ============================================================================
// 3D 直线裁剪
// ============================================================================

/**
 * 使用3D直线布尔运算裁剪OSG节点（封闭网格）
 * @param node 节点句柄
 * @param line void* -> const Math::Line3d*
 * @param normal 裁剪方向（归一化法向量）void* -> const Math::Vector3d*
 * @param thickness 直线厚度（半宽度）
 * @param clipType 裁剪类型：0=交集, 1=差集, 2=并集
 * @param vectorEps 向量容差
 * @param planeEps 平面容差
 * @param texCoordEps 纹理坐标容差
 * @param bboxEps 包围盒容差
 * @return osg::Node* 句柄（内部分配，外部负责释放），失败返回 NULL
 */
__declspec(dllexport) void* rb_clip_closed_node_with_line3d(
    void* node,
    const void* line,
    const void* normal,
    double thickness,
    int clipType,
    double vectorEps,
    double planeEps,
    double texCoordEps,
    double bboxEps);

#ifdef __cplusplus
}
#endif

#endif // CLIP_3D_CLOSED_CAPI_H
