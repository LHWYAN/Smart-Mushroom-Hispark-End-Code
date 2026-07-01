/*
 * Copyright (c) 2024 HiSilicon Technologies CO., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _BSP_OLED__H
#define _BSP_OLED__H

#include <stddef.h>
#include <stdint.h>
#include "oled_fonts.h"

#include "pinctrl.h"
#include "gpio.h"
#include "i2c.h"
#include "soc_osal.h"

#ifdef SSD1306_X_OFFSET
#define SSD1306_X_OFFSET_LOWER (SSD1306_X_OFFSET & 0x0F)
#define SSD1306_X_OFFSET_UPPER ((SSD1306_X_OFFSET >> 4) & 0x07)
#else
#define SSD1306_X_OFFSET_LOWER 0
#define SSD1306_X_OFFSET_UPPER 0
#endif

// SSD1306 OLED height in pixels
#ifndef OLED_HEIGHT 
#define OLED_HEIGHT 64
#endif

// SSD1306 width in pixels
#ifndef OLED_WIDTH
#define OLED_WIDTH 128
#endif

// some LEDs don't display anything in first two columns

#ifndef SSD1306_BUFFER_SIZE
#define SSD1306_BUFFER_SIZE (OLED_WIDTH * OLED_HEIGHT / 8)
#endif

// Enumeration for screen colors
typedef enum
{
    Black = 0x00, // Black color, no pixel
    White = 0x01  // Pixel is set. Color depends on OLED
} SSD1306_COLOR;

typedef enum
{
    SSD1306_OK = 0x00,
    SSD1306_ERR = 0x01 // Generic error.
} SSD1306_Error_t;

// Struct to store transformations
typedef struct
{
    uint16_t CurrentX;
    uint16_t CurrentY;
    uint8_t Inverted;
    uint8_t Initialized;
    uint8_t DisplayOn;
} SSD1306_t;
typedef struct
{
    uint8_t x;
    uint8_t y;
} SSD1306_VERTEX;

// OLED接口函数

/**
 * @brief  将已更改的屏幕缓冲区写入OLED。
 */
void bsp_oled_UpdateScreen(void);

/**
 * @brief 清空OLED显示.
 */
void bsp_oled_Clear(void);

/**
 * @brief  在OLED缓冲区中绘制一个像素。
 * @param  [in] x 像素点所在位置的x坐标。
 * @param  [in] y 像素点所在位置的y坐标。
 * @param  [in] color 像素颜色。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 */
errcode_t bsp_oled_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color);

/**
 * @brief  绘制一个字符到OLED缓冲区。
 * @param  [in] x 字符起始位置的x坐标。
 * @param  [in] y 字符起始位置的y坐标。
 * @param  [in] ch 要绘制的字符。
 * @param  [in] Font 字体。
 * @param  [in] color 像素颜色。
 * @retval 返回写好的字符  成功。
 * @retval 0  失败。
 */
char bsp_oled_DrawChar(uint8_t x, uint8_t y, char ch, FontDef Font, SSD1306_COLOR color);

/**
 * @brief  将完整字符串写入OLED缓冲区。
 * @param  [in] x 字符串起始位置的x坐标。
 * @param  [in] y 字符串起始位置的y坐标。
 * @param  [in] str 要绘制的字符串。
 * @param  [in] Font 字体。
 * @param  [in] color 像素颜色。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 */
errcode_t bsp_oled_DrawString(uint8_t x, uint8_t y, char *str, FontDef Font, SSD1306_COLOR color);

/**
 * @brief 在OLED上打印字符串.
 * @param [in] x 字符串起始位置x坐标.
 * @param [in] y 字符串起始位置y坐标.
 * @param [in] fmt 要打印的字符串.
 * @param [in] ... 需要打印字符串得到参数.
 */
void bsp_oled_Printf(uint8_t x, uint8_t y, char *fmt, ...);

/**
 * @brief  用布里森汉姆的算法，在OLED缓冲区画线。
 * @param  [in] x1 线起始位置的x坐标。
 * @param  [in] y1 线起始位置的y坐标。
 * @param  [in] x2 线结束位置的x坐标。
 * @param  [in] y2 线结束位置的y坐标。
 * @param  [in] color 像素颜色。
 */
void bsp_oled_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color);

/**
 * @brief  在OLED缓冲区绘制多段线。
 * @param  [in] par_vertex 多段线顶点数组。
 * @param  [in] par_size 多段线顶点数量。
 * @param  [in] color 像素颜色。
 */
void bsp_oled_DrawPolyline(const SSD1306_VERTEX *par_vertex, uint16_t par_size, SSD1306_COLOR color);

/**
 * @brief  用Bresenhem算法，在OLED缓冲区画圆。
 * @param  [in] par_x 圆心x坐标。
 * @param  [in] par_y 圆心y坐标。
 * @param  [in] par_r 圆半径。
 * @param  [in] par_color 像素颜色。
 */
void bsp_oled_DrawCircle(uint8_t par_x, uint8_t par_y, uint8_t par_r, SSD1306_COLOR par_color);

/**
 * @brief  在OLED缓冲区绘制矩形。
 * @param  [in] par_x1 矩形左上角x坐标。
 * @param  [in] par_y1 矩形左上角y坐标。
 * @param  [in] par_x2 矩形右下角x坐标。
 * @param  [in] par_y2 矩形右下角y坐标。
 * @param  [in] color 像素颜色。
 */
void bsp_oled_DrawRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color);

/**
 * @brief  在OLED缓冲区绘制位图。
 * @param  [in] bitmap 位图数据。
 * @param  [in] size 位图数据大小。
 */
void bsp_oled_DrawBitmap(const uint8_t *bitmap, uint32_t size);

/**
 * @brief  在OLED缓冲区绘制指定大小位图。
 * @param  [in] x 位图左上角x坐标。
 * @param  [in] y 位图左上角y坐标。
 * @param  [in] w 位图宽度。
 * @param  [in] data 位图数据。
 * @param  [in] size 位图数据大小。
 */
void bsp_oled_DrawRegion(uint8_t x, uint8_t y, uint8_t w, const uint8_t *data, uint32_t size);

/**
 * @brief 设置OLED显示打开/关闭.
 * @param [in] on 0 为关闭显示, 其他为打开显示.
 */
void bsp_oled_SetDisplayOn(const uint8_t on);

/**
 * @brief 获取OLED显示状态.
 * @return 0 为关闭显示, 其他为打开显示.
 */
uint8_t bsp_oled_GetDisplayOn(void);


/**
 * @brief SSD1306模块初始化
 * @param [in] oled_i2c_bus 被使用的IIC总线的编号
 * @return 0 为关闭显示, 其他为打开显示.
 */
void ssd1306_Init(i2c_bus_t oled_i2c_bus);

#endif // !_BSP_OLED__H