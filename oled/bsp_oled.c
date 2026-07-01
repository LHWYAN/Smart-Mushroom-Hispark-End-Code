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

#include "bsp_oled.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <securec.h>

#define I2C_MASTER_ADDR 0x0
#define I2C_SET_BAUDRATE 400000

#define I2C_SLAVE2_ADDR 0x3C
#define SSD1306_CTRL_CMD 0x00
#define SSD1306_CTRL_DATA 0x40
#define SSD1306_MASK_CONT (0x1 << 7)
#define DOUBLE 2

// #define SSD1306_INTERVAL_V (15)

// int g_ssd1306_current_loc_v = 0;

static i2c_bus_t g_bsp_oled_i2c_bus = I2C_BUS_1;

// Screenbuffer
static uint8_t SSD1306_Buffer[SSD1306_BUFFER_SIZE];

// Screen object
static SSD1306_t SSD1306;

static uint32_t ssd1306_SendData(uint8_t *buffer, uint32_t size)
{
    uint16_t dev_addr = I2C_SLAVE2_ADDR;
    i2c_data_t data = {0};
    data.send_buf = buffer;
    data.send_len = size;
    uint32_t retval = uapi_i2c_master_write(g_bsp_oled_i2c_bus, dev_addr, &data);
    if (retval != 0)
    {
        printf("I2cWrite(%02X) failed, %0X!\n", data.send_buf[1], retval);
        return retval;
    }
    return 0;
}

static uint32_t ssd1306_WiteByte(uint8_t regAddr, uint8_t byte)
{
    uint8_t buffer[] = {regAddr, byte};
    return ssd1306_SendData(buffer, sizeof(buffer));
}

// Send a byte to the command register
static void ssd1306_WriteCommand(uint8_t byte)
{
    ssd1306_WiteByte(SSD1306_CTRL_CMD, byte);
}

// Send data
static void ssd1306_WriteData(uint8_t *buffer, uint32_t buff_size)
{
    uint8_t data[OLED_WIDTH * DOUBLE] = {0};
    for (uint32_t i = 0; i < buff_size; i++)
    {
        data[i * DOUBLE] = SSD1306_CTRL_DATA | SSD1306_MASK_CONT;
        data[i * DOUBLE + 1] = buffer[i];
    }
    data[(buff_size - 1) * DOUBLE] = SSD1306_CTRL_DATA;
    ssd1306_SendData(data, sizeof(data));
}

/* Fills the Screenbuffer with values from a given buffer of a fixed length */
SSD1306_Error_t ssd1306_FillBuffer(uint8_t *buf, uint32_t len)
{
    SSD1306_Error_t ret = SSD1306_ERR;
    if (len <= SSD1306_BUFFER_SIZE)
    {
        memcpy_s(SSD1306_Buffer, len + 1, buf, len);
        ret = SSD1306_OK;
    }
    return ret;
}

// 用给定的颜色填充整个屏幕
static void ssd1306_Fill(SSD1306_COLOR color)
{
    /* Set memory */
    uint32_t i;

    for (i = 0; i < sizeof(SSD1306_Buffer); i++)
    {
        SSD1306_Buffer[i] = (color == Black) ? 0x00 : 0xFF;
    }
}

// 设置光标位置
static void ssd1306_SetCursor(uint8_t x, uint8_t y)
{
    SSD1306.CurrentX = x;
    SSD1306.CurrentY = y;
}

// 设置对比度
void ssd1306_SetContrast(const uint8_t value)
{
    const uint8_t kSetContrastControlRegister = 0x81;
    ssd1306_WriteCommand(kSetContrastControlRegister);
    ssd1306_WriteCommand(value);
}

static void ssd1306_Init_CMD(void)
{
    ssd1306_WriteCommand(0xA4); // 0xa4,Output follows RAM content;0xa5,Output ignores RAM content

    ssd1306_WriteCommand(0xD3); // -set display offset - CHECK
    ssd1306_WriteCommand(0x00); // -not offset

    ssd1306_WriteCommand(0xD5); // --set display clock divide ratio/oscillator frequency
    ssd1306_WriteCommand(0xF0); // --set divide ratio

    ssd1306_WriteCommand(0xD9); // --set pre-charge period
    ssd1306_WriteCommand(0x11); // 0x22 by default

    ssd1306_WriteCommand(0xDA); // --set com pins hardware configuration - CHECK
#if (OLED_HEIGHT == 32)
    ssd1306_WriteCommand(0x02);
#elif (OLED_HEIGHT == 64)
    ssd1306_WriteCommand(0x12);
#elif (OLED_HEIGHT == 128)
    ssd1306_WriteCommand(0x12);
#else
#error "Only 32, 64, or 128 lines of height are supported!"
#endif

    ssd1306_WriteCommand(0xDB); // --set vcomh
    ssd1306_WriteCommand(0x30); // 0x20,0.77xVcc, 0x30,0.83xVcc

    ssd1306_WriteCommand(0x8D); // --set DC-DC enable
    ssd1306_WriteCommand(0x14); //
    bsp_oled_SetDisplayOn(1);    // --turn on SSD1306 panel
}

// 初始化OLED设备驱动芯片 -- SSD1306
void ssd1306_Init(i2c_bus_t oled_i2c_bus)
{
    g_bsp_oled_i2c_bus = oled_i2c_bus;

    // Reset OLED
    // ssd1306_Reset();
    
    // Init OLED
    bsp_oled_SetDisplayOn(0); // display off

    ssd1306_WriteCommand(0x20); // Set Memory Addressing Mode
    ssd1306_WriteCommand(0x00); // 00b,Horizontal Addressing Mode; 01b,Vertical Addressing Mode;
                                // 10b,Page Addressing Mode (RESET); 11b,Invalid

    ssd1306_WriteCommand(0xB0); // Set Page Start Address for Page Addressing Mode,0-7

#ifdef SSD1306_MIRROR_VERT
    ssd1306_WriteCommand(0xC0); // Mirror vertically
#else
    ssd1306_WriteCommand(0xC8); // Set COM Output Scan Direction
#endif

    ssd1306_WriteCommand(0x00); // ---set low column address
    ssd1306_WriteCommand(0x10); // ---set high column address

    ssd1306_WriteCommand(0x40); // --set start line address - CHECK

    ssd1306_SetContrast(0xFF);

#ifdef SSD1306_MIRROR_HORIZ
    ssd1306_WriteCommand(0xA0); // Mirror horizontally
#else
    ssd1306_WriteCommand(0xA1); // --set segment re-map 0 to 127 - CHECK
#endif

#ifdef SSD1306_INVERSE_COLOR
    ssd1306_WriteCommand(0xA7); // --set inverse color
#else
    ssd1306_WriteCommand(0xA6); // --set normal color
#endif

// Set multiplex ratio.
#if (OLED_HEIGHT == 128)
    // Found in the Luma Python lib for SH1106.
    ssd1306_WriteCommand(0xFF);
#else
    ssd1306_WriteCommand(0xA8); // --set multiplex ratio(1 to 64) - CHECK
#endif

#if (OLED_HEIGHT == 32)
    ssd1306_WriteCommand(0x1F); //
#elif (OLED_HEIGHT == 64)
    ssd1306_WriteCommand(0x3F); //
#elif (OLED_HEIGHT == 128)
    ssd1306_WriteCommand(0x3F); // Seems to work for 128px high displays too.
#else
#error "Only 32, 64, or 128 lines of height are supported!"
#endif
    ssd1306_Init_CMD();
    // Clear screen
    ssd1306_Fill(Black);

    // Flush buffer to screen
    bsp_oled_UpdateScreen();

    // Set default values for screen object
    SSD1306.CurrentX = 0;
    SSD1306.CurrentY = 0;

    SSD1306.Initialized = 1;
}

/******************** OLED对外接口函数 ********************/

/**
 * @brief  将已更改的屏幕缓冲区写入OLED。
 */
void bsp_oled_UpdateScreen(void)
{
    // Write data to each page of RAM. Number of pages
    // depends on the screen height:
    //
    //  * 32px   ==  4 pages
    //  * 64px   ==  8 pages
    //  * 128px  ==  16 pages
    for (uint8_t i = 0; i < OLED_HEIGHT / 8; i++)
    {
        ssd1306_WriteCommand(0xB0 + i); // Set the current RAM page address.
        ssd1306_WriteCommand(0x00 + SSD1306_X_OFFSET_LOWER);
        ssd1306_WriteCommand(0x10 + SSD1306_X_OFFSET_UPPER);
        ssd1306_WriteData(&SSD1306_Buffer[OLED_WIDTH * i], OLED_WIDTH);
    }
}

/**
 * @brief 清空OLED显示.
 */
void bsp_oled_Clear(void)
{
    ssd1306_Fill(Black);
    // g_ssd1306_current_loc_v = 0;
}

/**
 * @brief  在OLED缓冲区中绘制一个像素。
 * @param  [in] x 像素点所在位置的x坐标。
 * @param  [in] y 像素点所在位置的y坐标。
 * @param  [in] color 像素颜色。
 * @retval ERRCODE_SUCC 成功。
 * @retval Other        失败，参考 @ref errcode_t 。
 */
errcode_t bsp_oled_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT)
    {
        return ERRCODE_INVALID_PARAM;
    }

    SSD1306_COLOR color1 = color;
    // Check if pixel should be inverted
    if (SSD1306.Inverted)
    {
        color1 = (SSD1306_COLOR)!color1;
    }

    // Draw in the right color
    uint32_t c = 8; // 8
    if (color == White)
    {
        SSD1306_Buffer[x + (y / c) * OLED_WIDTH] |= 1 << (y % c);
    }
    else
    {
        SSD1306_Buffer[x + (y / c) * OLED_WIDTH] &= ~(1 << (y % c));
    }

    return ERRCODE_SUCC;
}

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
char bsp_oled_DrawChar(uint8_t x, uint8_t y, char ch, FontDef Font, SSD1306_COLOR color)
{
    uint32_t i, b, j;

    // 设置光标位置
    ssd1306_SetCursor(x, y);

    // Check if character is valid
    uint32_t ch_min = 32;  // 32
    uint32_t ch_max = 126; // 126
    if ((uint32_t)ch < ch_min || (uint32_t)ch > ch_max)
    {
        return 0;
    }

    // Check remaining space on current line
    if (OLED_WIDTH < (SSD1306.CurrentX + Font.FontWidth) || OLED_HEIGHT < (SSD1306.CurrentY + Font.FontHeight))
    {
        // Not enough space on current line
        // osal_printk("Not enough space on current line\r\n");
        // osal_printk("SSD1306.CurrentX = %d, Font.FontWidth = %d\r\n", SSD1306.CurrentX, Font.FontWidth);
        // osal_printk("SSD1306.CurrentX = %d, Font.FontWidth = %d\r\n", SSD1306.CurrentX, Font.FontWidth);
        return 0;
    }

    // Use the font to write
    for (i = 0; i < Font.FontHeight; i++)
    {
        b = Font.data[(ch - ch_min) * Font.FontHeight + i];
        for (j = 0; j < Font.FontWidth; j++)
        {
            if ((b << j) & 0x8000)
            {
                bsp_oled_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR)color);
            }
            else
            {
                bsp_oled_DrawPixel(SSD1306.CurrentX + j, (SSD1306.CurrentY + i), (SSD1306_COLOR)!color);
            }
        }
    }

    // The current space is now taken
    // SSD1306.CurrentX += Font.FontWidth;

    // Return written char for validation
    return ch;
}

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
errcode_t bsp_oled_DrawString(uint8_t x, uint8_t y, char *str, FontDef Font, SSD1306_COLOR color)
{
    // Write until null-byte
    char *str1 = str;
    while (*str1)
    {
        // osal_printk("x = %d, y = %d\n", x, y);
        if (bsp_oled_DrawChar(x, y, *str1, Font, color) != *str1)
        {
            // Char could not be written
            return ERRCODE_FAIL;
        }
        // Next char
        str1++;
        x += Font.FontWidth;
        if(OLED_WIDTH < (x + Font.FontWidth))
        {
            x = 0;
            y += Font.FontHeight;
        }
        
    }

    // Everything ok
    return ERRCODE_SUCC;
}

/**
 * @brief 在OLED上打印字符串.
 * @param [in] x 字符串起始位置x坐标.
 * @param [in] y 字符串起始位置y坐标.
 * @param [in] fmt 要打印的字符串.
 * @param [in] ... 需要打印字符串得到参数.
 */
void bsp_oled_Printf(uint8_t x, uint8_t y, char *fmt, ...)
{
    char buffer[20];
    int ret = 0;
    if (fmt)
    {
        va_list argList;
        va_start(argList, fmt);
        ret = vsnprintf_s(buffer, sizeof(buffer), sizeof(buffer), fmt, argList);
        if (ret < 0)
        {
            printf("buffer is null\r\n");
        }
        va_end(argList);
        // ssd1306_SetCursor(0, g_ssd1306_current_loc_v);
        bsp_oled_DrawString(x, y, buffer, Font_7x10, White);

        bsp_oled_UpdateScreen();
    }
}

/**
 * @brief  用布里森汉姆的算法，在OLED缓冲区画线。
 * @param  [in] x1 线起始位置的x坐标。
 * @param  [in] y1 线起始位置的y坐标。
 * @param  [in] x2 线结束位置的x坐标。
 * @param  [in] y2 线结束位置的y坐标。
 * @param  [in] color 像素颜色。
 */
void bsp_oled_DrawLine(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color)
{
    uint8_t x = x1;
    uint8_t y = y1;
    int32_t deltaX = abs(x2 - x1);
    int32_t deltaY = abs(y2 - y1);
    int32_t signX = ((x1 < x2) ? 1 : -1);
    int32_t signY = ((y1 < y2) ? 1 : -1);
    int32_t error = deltaX - deltaY;
    int32_t error2;
    bsp_oled_DrawPixel(x2, y2, color);
    while ((x1 != x2) || (y1 != y2))
    {
        bsp_oled_DrawPixel(x1, y1, color);
        error2 = error * DOUBLE;
        if (error2 > -deltaY)
        {
            error -= deltaY;
            x += signX;
        }
        else
        {
            /* nothing to do */
        }
        if (error2 < deltaX)
        {
            error += deltaX;
            y += signY;
        }
        else
        {
            /* nothing to do */
        }
    }
}

/**
 * @brief  在OLED缓冲区绘制多段线。
 * @param  [in] par_vertex 多段线顶点数组。
 * @param  [in] par_size 多段线顶点数量。
 * @param  [in] color 像素颜色。
 */
void bsp_oled_DrawPolyline(const SSD1306_VERTEX *par_vertex, uint16_t par_size, SSD1306_COLOR color)
{
    uint16_t i;
    if (par_vertex != 0)
    {
        for (i = 1; i < par_size; i++)
        {
            bsp_oled_DrawLine(par_vertex[i - 1].x, par_vertex[i - 1].y, par_vertex[i].x, par_vertex[i].y, color);
        }
    }
    else
    {
        /* nothing to do */
    }
    return;
}

/**
 * @brief  用Bresenhem算法，在OLED缓冲区画圆。
 * @param  [in] par_x 圆心x坐标。
 * @param  [in] par_y 圆心y坐标。
 * @param  [in] par_r 圆半径。
 * @param  [in] par_color 像素颜色。
 */
void bsp_oled_DrawCircle(uint8_t par_x, uint8_t par_y, uint8_t par_r, SSD1306_COLOR par_color)
{
    int32_t x = -par_r;
    int32_t y = 0;
    int32_t b = 2;
    int32_t err = b - b * par_r;
    int32_t e2;

    if (par_x >= OLED_WIDTH || par_y >= OLED_HEIGHT)
    {
        return;
    }

    do
    {
        bsp_oled_DrawPixel(par_x - x, par_y + y, par_color);
        bsp_oled_DrawPixel(par_x + x, par_y + y, par_color);
        bsp_oled_DrawPixel(par_x + x, par_y - y, par_color);
        bsp_oled_DrawPixel(par_x - x, par_y - y, par_color);
        e2 = err;
        if (e2 <= y)
        {
            y++;
            err = err + (y * b + 1);
            if (-x == y && e2 <= x)
            {
                e2 = 0;
            }
            else
            {
                /* nothing to do */
            }
        }
        else
        {
            /* nothing to do */
        }
        if (e2 > x)
        {
            x++;
            err = err + (x * b + 1);
        }
        else
        {
            /* nothing to do */
        }
    } while (x <= 0);

    return;
}

/**
 * @brief  在OLED缓冲区绘制矩形。
 * @param  [in] par_x1 矩形左上角x坐标。
 * @param  [in] par_y1 矩形左上角y坐标。
 * @param  [in] par_x2 矩形右下角x坐标。
 * @param  [in] par_y2 矩形右下角y坐标。
 * @param  [in] color 像素颜色。
 */
void bsp_oled_DrawRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, SSD1306_COLOR color)
{
    bsp_oled_DrawLine(x1, y1, x2, y1, color);
    bsp_oled_DrawLine(x2, y1, x2, y2, color);
    bsp_oled_DrawLine(x2, y2, x1, y2, color);
    bsp_oled_DrawLine(x1, y2, x1, y1, color);
}

/**
 * @brief  在OLED缓冲区绘制位图。
 * @param  [in] bitmap 位图数据。
 * @param  [in] size 位图数据大小。
 */
void bsp_oled_DrawBitmap(const uint8_t *bitmap, uint32_t size)
{
    unsigned int c = 8;
    uint8_t rows = size * c / OLED_WIDTH;
    if (rows > OLED_HEIGHT)
    {
        rows = OLED_HEIGHT;
    }
    for (uint8_t y = 0; y < rows; y++)
    {
        for (uint8_t x = 0; x < OLED_WIDTH; x++)
        {
            uint8_t byte = bitmap[(y * OLED_WIDTH / c) + (x / c)];
            uint8_t bit = byte & (0x80 >> (x % c));
            bsp_oled_DrawPixel(x, y, bit ? White : Black);
        }
    }
}

/**
 * @brief  在OLED缓冲区绘制指定大小位图。
 * @param  [in] x 位图左上角x坐标。
 * @param  [in] y 位图左上角y坐标。
 * @param  [in] w 位图宽度。
 * @param  [in] data 位图数据。
 * @param  [in] size 位图数据大小。
 */
void bsp_oled_DrawRegion(uint8_t x, uint8_t y, uint8_t w, const uint8_t *data, uint32_t size)
{
    uint32_t stride = w;
    uint8_t h = w; // 字体宽高一样
    uint8_t width = w;
    if (x + w > OLED_WIDTH || y + h > OLED_HEIGHT || w * h == 0)
    {
        // printf("%dx%d @ %d,%d out of range or invalid!\r\n", w, h, x, y);
        return;
    }

    width = (width <= OLED_WIDTH ? width : OLED_WIDTH);
    h = (h <= OLED_HEIGHT ? h : OLED_HEIGHT);
    stride = (stride == 0 ? w : stride);
    unsigned int c = 8;

    uint8_t rows = size * c / stride;
    for (uint8_t i = 0; i < rows; i++)
    {
        uint32_t base = i * stride / c;
        for (uint8_t j = 0; j < width; j++)
        {
            uint32_t idx = base + (j / c);
            uint8_t byte = idx < size ? data[idx] : 0;
            uint8_t bit = byte & (0x80 >> (j % c));
            bsp_oled_DrawPixel(x + j, y + i, bit ? White : Black);
        }
    }
}

/**
 * @brief 设置OLED显示打开/关闭.
 * @param [in] on 0 为关闭显示, 其他为打开显示.
 */
void bsp_oled_SetDisplayOn(const uint8_t on)
{
    uint8_t value;
    if (on)
    {
        value = 0xAF; // Display on
        SSD1306.DisplayOn = 1;
    }
    else
    {
        value = 0xAE; // Display off
        SSD1306.DisplayOn = 0;
    }
    ssd1306_WriteCommand(value);
}

/**
 * @brief 获取OLED显示状态.
 * @return 0 为关闭显示, 其他为打开显示.
 */
uint8_t bsp_oled_GetDisplayOn(void)
{
    return SSD1306.DisplayOn;
}
