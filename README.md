# RAW Process - ISP 图像处理仿真流水线

这是一个完整的**ISP（Image Signal Processing，图像信号处理）仿真工程**，实现了从原始 RAW Bayer 图像输入到最终 RGB 图像输出的完整图像处理流水线，支持每个模块参数独立配置，方便算法调试和效果对比。

---

## 一、用户使用文档

### 1.1 简介

本工程用于**对摄像头输出的原始 RAW 图像做 ISP 全流程仿真处理**，适用于：
- ISP 参数调优（LSC、AWB、CCM 等）
- 新算法原型验证
- 图像质量分析评估
- ISP 流程学习理解

工程特点：
- 模块化设计，每个模块独立可开关
- DEBUG 模式下自动保存每个模块的输出结果，便于对比
- C/C++ + OpenCV，代码简洁易读，便于二次开发

### 1.2 环境要求

- **操作系统**：Windows（Visual Studio 2019/2022）
- **依赖库**：OpenCV 4.x
- **编译工具**：MSVC 编译器

> 注：Qt / CMake 编译方法可自行适配，工程本身不依赖 VS 特有功能。

### 1.3 快速开始

#### 步骤 1：准备 RAW 文件
将你的 raw 图放入 `data/` 目录，默认文件名设为 `raw.raw`。

RAW 文件要求：
- 纯二进制裸数据，无文件头
- 单像素 `bit` 位深对齐

示例：工程自带 `data/raw.raw` 作为测试，分辨率 1440×1080，10bit 有效位，BGGR 排列，小端字节序。

#### 步骤 2：修改配置参数
打开 [load_ini.cpp](file:///c:/Work/PerDoc/Github/raw_process/load_ini.cpp)，在 `load_cfg()` 函数中修改：

```cpp
cfg->bit = 16;          // 存储每像素占用多少位（一般是10/12/16）
cfg->used_bit = 10;     // 实际有效位
cfg->order = LITTLE_ENDIAN;  // 字节序：LITTLE_ENDIAN 或 BIG_ENDIAN
cfg->pattern = BGGR;    // Bayer排列：RGGB/GRBG/GBRG/BGGR
cfg->width = 1440;      // 图像宽度
cfg->height = 1048;     // 图像高度
```

各算法模块可以通过开关控制启用/禁用：
```cpp
cfg->ob_on = 1;      // 1 启用，0 禁用（禁用则跳过该模块）
cfg->lsc_on = 1;
cfg->gic_on = 1;
...
```

调整各模块参数，例如 AWB 增益：
```cpp
cfg->r_gain = 1024 * 1.85;   // 增益以 1024 为基数，1.85× 增益即 1024*1.85
cfg->g_gain = 1024 * 1.0;
cfg->b_gain = 1024 * 1.85;
```

#### 步骤 3：修改输入文件路径（可选）
如果你的 RAW 文件不是 `data/raw.raw`，打开 [raw_process.cpp](file:///c:/Work/PerDoc/Github/raw_process/raw_process.cpp#L18) 修改：

```cpp
const char* filename = "data/your_raw.raw";
```

#### 步骤 4：编译运行
1. 在 Visual Studio 中打开 `raw_process.sln`
2. 选择 Release 配置，x64 平台
3. 生成解决方案
4. 运行 `raw_process.exe`

#### 步骤 5：查看结果

程序会自动：
- 启动时清空当前目录下所有 `.jpg/.png/.bmp`
- DEBUG 模式下，每个模块处理完后保存一张中间结果图
- 文件名格式：`HHMMSSmmm_模块名.jpg`（时间戳 + 模块后缀）
- 最终结果输出：`HHMMSSmmm_end.jpg`

示例输出顺序：
```
070945626_origin.jpg    // 原始RAW可视化
070945656_ob.jpg        // OB处理后
070945697_lsc.jpg       // LSC处理后
070945729_igain.jpg     // ISP Gain处理后
...
070952568_end.jpg       // 最终结果
```

### 1.4 配置参数表

| 参数分类 | 参数名 | 说明 |
|---------|--------|------|
| 图像参数 | bit | 存储每像素占用比特数（通常16） |
| | used_bit | 实际有效比特数（10/12） |
| | order | 字节序：`LITTLE_ENDIAN`/`BIG_ENDIAN` |
| | pattern | Bayer排列：`RGGB`/`GRBG`/`GBRG`/`BGGR` |
| | width/height | 图像宽高 |
| | rgb_bit/yuv_bit | RGB/YUV域位深（默认16） |
| 模块开关 | `*_on` | 1=启用，0=禁用，禁用则跳过该模块 |
| OB | ob | 黑电平值 |
| LSC | lsc_type | 0=双线性插值，1=flat分块 |
| | lsc_wblock/hblock | 宽/高方向分块数 |
| | lsc_max_gain | 最大增益限制 |
| | lsc_rgain/lsc_grgain/lsc_gbgain/lsc_bgain | R/Gr/Gb/B四路增益表 |
| ISP Gain | isp_gain | 全局增益值（基数1024） |
| GIC | gic_str | 校正强度，0~1 |
| | gic_thd | 差值阈值，超过阈值不校正 |
| AWB | r_gain/g_gain/b_gain | 三路增益（基数1024） |
| LTM | ltm_r | 高斯滤波半径 |
| | ltm_str | 增强强度 |
| | ltm_gain_limit_min/max | gain值截断范围 |
| CCM | ccm[9] | 3×3颜色校正矩阵，行优先存储 |
| RGB Gamma | gamma_x/gamma_y | 49点Gamma曲线控制点 |
| Defog | defog_smp_ratio | 下采样比例 |
| | light_ratio | 大气光估计比例 |
| | defog_str | 去雾强度 |
| YNR | ynr_r | 中值滤波半径 |
| CNR | cnr_r | U/V滤波半径 |
| YUV TXI | txi_r_detail | 高斯滤波半径提取细节 |
| | txi_r_bifilter | 细节中值滤波半径 |
| | txi_str | 细节增强强度 |

### 1.5 常见问题

**Q: 输出图像颜色错乱/偏色严重？**
> A: 检查两个地方：(1) `pattern` Bayer 排列是否和传感器一致；(2) `order` 字节序是否正确。

**Q: 图像整体偏暗或过曝？**
> A: 调整 `isp_gain` 全局增益，以及检查 `ob` 黑电平值是否正确。

**Q: 如何关闭某个模块看效果？**
> A: 把对应模块的 `*_on` 设为 0 即可，不影响其他模块。

**Q: 如何添加新的算法模块？**
> A: 参考已有模块结构：新建 `xxx.h` + `xxx.cpp`，实现 `U8 xxx_process(...)`，然后在 `raw_process.cpp` 主流程中添加调用即可。详见「开发文档」。

---

## 二、算法文档

### 2.1 整体处理流程

```
RAW (Bayer) 
  ↓
┌─────────────────────────────────────────────┐
│                 RAW 域处理                   │
│  OB → LSC → ISP Gain → GIC → AWB → LTM      │
└─────────────────────────────────────────────┘
  ↓
Demosaic → RGB
  ↓
┌─────────────────────────────────────────────┐
│                 RGB 域处理                   │
│  CCM → RGB Gamma → Defog → R2Y → YUV         │
└─────────────────────────────────────────────┘
  ↓
┌─────────────────────────────────────────────┐
│                 YUV 域处理                   │
│  YNR → CNR → Sharp → YUV TXI                │
└─────────────────────────────────────────────┘
  ↓
Y2R → RGB → 输出 JPEG
```

处理流程遵循主流 ISP 流水线顺序，每个模块处理结果都可独立保存可视化。

### 2.2 RAW 域算法

#### 2.2.1 OB - Optical Black，光学黑电平校正

**算法原理**：
传感器读出的 RAW 数据存在黑电平偏移，每个像素实际值 = 原始值 - 黑电平。校正后再线性拉伸到满量程：

```
raw[i] = max(0, raw[i] - ob)
raw[i] = raw[i] * (U16MAX / (U16MAX - ob))
```

**为什么需要**：消除传感器固定偏置，保证黑场正确，提升动态范围利用率。

**代码位置**：[ob.cpp](file:///c:/Work/PerDoc/Github/raw_process/ob.cpp)

---

#### 2.2.2 LSC - Lens Shading Correction，镜头阴影校正

**算法原理**：
镜头光学特性导致图像边缘亮度比中心低，且不同通道衰减不一致。通过预先标定的分块增益表对每个像素乘上相应增益，补偿亮度衰减。

支持两种插值模式：
- **flat模式**：每个像素使用所在分块的增益（不插值）
- **插值模式**：使用双线性插值，由相邻四个分块增益加权得到当前像素增益

R/Gr/Gb/B 四个通道独立存储增益表，根据 Bayer 位置分别应用对应增益。

**公式**：
```
// 双线性插值
gain = (1-x_weight)*(1-y_weight)*gain[lt] + 
       x_weight*(1-y_weight)*gain[rt] + 
       (1-x_weight)*y_weight*gain[lb] + 
       x_weight*y_weight*gain[rb]
raw[i] = clamp(raw[i] * gain, 0, U16MAX)
```

**代码位置**：[lsc.cpp](file:///c:/Work/PerDoc/Github/raw_process/lsc.cpp)

---

#### 2.2.3 ISP Gain - 数字增益

**算法原理**：简单的全局乘性增益调整曝光亮度：

```
raw[i] = clamp((raw[i] * isp_gain) >> 10, 0, U16MAX)
```

增益以 1024 为基数，即 `gain = 1024 * k`，k 为实际倍数。

**代码位置**：[isp_gain.cpp](file:///c:/Work/PerDoc/Github/raw_process/isp_gain.cpp)

---

#### 2.2.4 GIC - Green Imbalance Correction，绿色不平衡校正

**算法原理**：
同一 Bayer 单元中有两个绿色像素：Gr（R行）和 Gb（B行），由于工艺原因二者响应可能不一致，导致伪色。算法对差值较小的两个G像素进行加权混合：

```
如果 |g1 - g2| < gic_thd * min(g1, g2):
    g1' = (1 - gic_str/2) * g1 + (gic_str/2) * g2
    g2' = (1 - gic_str/2) * g2 + (gic_str/2) * g1
```

`gic_str` 控制校正强度，0 表示不校正，1 表示完全混合。

**代码位置**：[gic.cpp](file:///c:/Work/PerDoc/Github/raw_process/gic.cpp)

---

#### 2.2.5 AWB - Auto White Balance，自动白平衡

**算法原理**：
根据统计得到的白平衡增益，对 R/G/B 通道分别增益：

```
// 按 Bayer 位置乘对应增益
raw[i] = clamp((raw[i] * gain) >> 10, 0, U16MAX)
```

本工程目前使用**增益参数输入方式**，适合离线调优。如果需要自动AWB算法，可在这一步之前添加统计估计。

**代码位置**：[awb.cpp](file:///c:/Work/PerDoc/Github/raw_process/awb.cpp)

---

#### 2.2.6 LTM - Local Tone Mapping，局部色调映射

**算法原理**：
局部对比度增强，提升暗部细节：
1. 将 2×2 Bayer 块平均下采样得到亮度图 Y
2. 高斯模糊得到低频分量 Y_gauss
3. 增强：Y_enh = Y + (Y - Y_gauss) * ltm_str
4. 计算 gain_map = Y_enh / Y
5. 将 gain_map 应用回原 RAW 四个像素

**增益限制**：`ltm_gain_limit_min` ~ `ltm_gain_limit_max` 防止溢出。

**代码位置**：[ltm.cpp](file:///c:/Work/PerDoc/Github/raw_process/ltm.cpp)

---

#### 2.2.7 Demosaic - 去马赛克

**算法原理**：
将单通道 Bayer RAW 插值成全分辨率 RGB：每个像素根据邻域同色像素平均恢复缺失的两个颜色分量。

例如 RGGB 排列中，若当前像素是 R：
```
pixel.R = raw[i]
pixel.G = (上 + 下 + 左 + 右) / 4
pixel.B = (左上 + 右上 + 左下 + 右下) / 4
```

支持四种 Bayer 排列。

> 注：这是最简单的双线性插值，如需要更高质量可替换为更多高级算法（如梯度方向插值、Malvar算法等）。

**代码位置**：[demosaic.cpp](file:///c:/Work/PerDoc/Github/raw_process/demosaic.cpp)

---

### 2.3 RGB 域算法

#### 2.3.1 CCM - Color Correction Matrix，颜色校正矩阵

**算法原理**：
传感器的RGB颜色空间和标准sRGB不一致，通过 3×3 矩阵变换校正到目标色彩空间：

```
[R']   [c0 c1 c2] [R]
[G'] = [c3 c4 c5] [G]
[B']   [c6 c7 c8] [B]
```

结果裁剪到合法范围。同时 `calc_NAI()` 会计算噪声放大系数用于评估。

**代码位置**：[ccm.cpp](file:///c:/Work/PerDoc/Github/raw_process/ccm.cpp)

---

#### 2.3.2 RGB Gamma - Gamma 校正

**算法原理**：
对 RGB 三通道分别做 Gamma 映射，将线性数据编码为非线性空间适配显示。使用分段线性插值：
1. 控制点对 `(gamma_x[i], gamma_y[i])`
2. 对输入值，找到所在区间，做线性插值得到输出

本工程默认提供 49 点的标准 sRGB Gamma 曲线，可根据需要替换。

**代码位置**：[rgbgamma.cpp](file:///c:/Work/PerDoc/Github/raw_process/rgbgamma.cpp)

---

#### 2.3.3 Defog - 去雾（暗通道先验）

**算法原理**：基于何恺明暗通道先验去雾算法：

1. **计算暗通道**：每个像素取 RGB 三通道最小值
   ```
   dark[i] = min(r, g, b)
   ```
2. 中值滤波平滑暗通道
3. 下采样加速计算
4. **估计大气光**：在暗通道最亮的位置取原图像素
5. **估计透射率**：
   ```
   trans[i] = 1 - defog_str * min(r/Ar, g/Ag, b/Ab)
   ```
6. **恢复图像**：
   ```
   J[i] = (I[i] - A * (1 - trans[i])) / max(trans[i], 0.001)
   ```

支持通过 `defog_str` 控制去雾强度，0 = 不去雾，值越大去雾越强。

**代码位置**：[defog.cpp](file:///c:/Work/PerDoc/Github/raw_process/defog.cpp)

---

#### 2.3.4 R2Y - RGB 转 YUV

**算法原理**：使用 BT.601 标准转换矩阵：

```
Y  = 0.299R + 0.587G + 0.114B
U  = -0.1687R - 0.3313G + 0.5B + 2^(yuv_bit-1)
V  = 0.5R - 0.4187G - 0.0813B + 2^(yuv_bit-1)
```

**代码位置**：[r2y.cpp](file:///c:/Work/PerDoc/Github/raw_process/r2y.cpp)

---

### 2.4 YUV 域算法

#### 2.4.1 YNR - Y 通道降噪

**算法原理**：对 Y 通道做中值滤波，去除椒盐噪声同时较好保留边缘。

**代码位置**：[ynr.cpp](file:///c:/Work/PerDoc/Github/raw_process/ynr.cpp)

---

#### 2.4.2 CNR - 色度降噪

**算法原理**：对 U/V 通道依次做中值滤波 + 高斯滤波，平滑色度噪声。

**代码位置**：[cnr.cpp](file:///c:/Work/PerDoc/Github/raw_process/cnr.cpp)

---

#### 2.4.5 Sharp - 锐化

**算法原理**：使用 3×5 边缘检测卷积核提取边缘信息。（完整增强部分待实现）

**代码位置**：[sharp.cpp](file:///c:/Work/PerDoc/Github/raw_process/sharp.cpp)

---

#### 2.4.4 YUV TXI - 纹理细节增强

**算法原理**：
1. 高斯模糊原图得到低频背景：`Y_bg = gauss_filter(Y)`
2. 提取高频细节：`detail = Y - Y_bg`
3. 中值滤波去除细节中的噪声
4. 增强细节：`detail_enh = detail * txi_str`
5. 融合：`Y_out = Y_bg + detail_enh`

通过 `txi_str` 控制增强强度，大于 1 增强细节，小于 1 削弱。

**代码位置**：[yuv_txi.cpp](file:///c:/Work/PerDoc/Github/raw_process/yuv_txi.cpp)

---

### 2.5 输出阶段

#### Y2R - YUV 转 RGB

逆 BT.601 转换，用于最终保存图像：

```
R = Y + 1.402 * V
G = Y - 0.3441 * U - 0.7141 * V
B = Y + 1.7720 * U
```

**代码位置**：[y2r.cpp](file:///c:/Work/PerDoc/Github/raw_process/y2r.cpp)

---

## 三、开发文档

### 3.1 文件说明

| 文件 | 说明 |
|------|------|
| **raw_process.sln** | Visual Studio 解决方案文件 |
| **raw_process.vcxproj** | VS 项目文件 |
| **raw_process.h/cpp** | 主入口 + 核心数据结构定义 + 公共工具函数 |
| **load_ini.h/cpp** | 配置加载（硬编码默认参数 + INI 文件解析支持） |
| **ob.h/cpp** | 黑电平校正 |
| **lsc.h/cpp** | 镜头阴影校正 |
| **isp_gain.h/cpp** | 数字增益 |
| **gic.h/cpp** | 绿色不平衡校正 |
| **awb.h/cpp** | 白平衡增益应用 |
| **ltm.h/cpp** | 局部色调映射 |
| **demosaic.h/cpp** | 去马赛克 |
| **ccm.h/cpp** | 颜色校正矩阵 |
| **rgbgamma.h/cpp** | Gamma 校正 |
| **defog.h/cpp** | 暗通道去雾 |
| **r2y.h/cpp** | RGB 转 YUV |
| **ynr.h/cpp** | Y 通道降噪 |
| **cnr.h/cpp** | 色度降噪 |
| **yuv_txi.h/cpp** | 纹理细节增强 |
| **sharp.h/cpp** | 锐化（边缘提取） |
| **y2r.h/cpp** | YUV 转 RGB |
| **data/** | 测试 RAW 文件存放目录 |

### 3.2 核心数据结构

#### `G_CONFIG` - 全局配置
```cpp
typedef struct _G_CONFIG
{
    // 图像基本参数
    U8 bit;         // 每像素存储位数
    U8 used_bit;    // 有效位数
    U8 order;       // 字节序
    U8 pattern;     // Bayer排列
    U16 width;      // 宽度
    U16 height;     // 高度
    
    // 位深设置
    U8 rgb_bit;     // RGB域位深
    U8 yuv_bit;     // YUV域位深
    
    // 模块开关
    U8 ob_on;
    U8 lsc_on;
    ... // 每个模块一个开关，1=启用 0=禁用
    
    // 各模块参数...
} G_CONFIG;
```

所有算法参数都存在这里，传递给每个模块。

#### `RGB` - RGB像素
```cpp
typedef struct _RGB
{
    U16 b;
    U16 g;
    U16 r;
} RGB;
```
存储顺序 BGR 是匹配 OpenCV `cv::Vec3b` 顺序。

#### `YUV` - YUV图像
```cpp
typedef struct _YUV
{
    U16 *y;
    U16 *u;
    U16 *v;
} YUV;
```
三个分量独立分配一维数组。

#### `IMG_CONTEXT` - 图像上下文
```cpp
typedef struct _IMG_CONTEXT
{
    U16 height;
    U16 width;
    U32 full_size;  // width * height
    ...
} IMG_CONTEXT;
```

### 3.3 主流程详解

入口函数 `main()` 在 [raw_process.cpp](file:///c:/Work/PerDoc/Github/raw_process/raw_process.cpp#L15-L54)：

```cpp
int main() 
{
    clear_tmp();              // 清空上次输出的图片
    load_cfg(&cfg);           // 加载配置
    IMG_CONTEXT context = {...};
    
    // 读取RAW
    raw = readraw(filename, context, cfg);
    
    // ========== RAW域 ==========
    ob_process(raw, context, cfg);
    lsc_process(raw, context, cfg);
    isp_gain_process(raw, context, cfg);
    gic_process(raw, context, cfg);
    awb_process(raw, context, cfg);
    ltm_process(raw, context, cfg);
    rgb_data = demosaic_process(raw, context, cfg);
    
    // ========== RGB域 ==========
    ccm_process(rgb_data, context, cfg);
    rgbgamma_process(rgb_data, context, cfg);
    defog_process(rgb_data, context, cfg);
    yuv_data = r2y_process(rgb_data, context, cfg);
    
    // ========== YUV域 ==========
    ynr_process(yuv_data, context, cfg);
    cnr_process(yuv_data, context, cfg);
    sharp_process(yuv_data, context, cfg);
    yuv_txi_process(yuv_data, context, cfg);
    
    // YUV -> RGB
    rgb_data = y2r_process(yuv_data, context, cfg);
    
    // 保存最终结果
    save_img_with_timestamp(rgb_data, &context, "_end");
    
    // 释放内存
    free(raw); free(rgb_data); free(yuv_data);
    return 0;
}
```

**模块调用约定**：
- 返回值：`U8`，`OK`(0) 成功，`ERROR`(-1) 失败
- 参数：`(数据指针, context, cfg)`
- 模块内部第一步：检查 `cfg->module_on`，如果是 0 直接返回 OK
- DEBUG_MODE 开启时，处理完自动保存中间结果图

### 3.4 文件读写详解

#### 读取 RAW 文件 - `readraw()`

位置：[raw_process.cpp](file:///c:/Work/PerDoc/Github/raw_process/raw_process.cpp#L320-L366)

流程：
1. 根据 `cfg.bit` 计算每像素字节数：`bytesPerPixel = (bit + 7) / 8`
2. 分配缓冲区，`fopen` 二进制读
3. 根据字节序 `order` 组装 `U16`：
   - 小端：`raw[i] = buffer[2i] | (buffer[2i+1] << 8)`
   - 大端：`raw[i] = (buffer[2i] << 8) | buffer[2i+1]`
4. 根据 `used_bit` 左移对齐：`raw[i] <<= (bit - used_bit)`
5. DEBUG_MODE 下保存 `*_origin.jpg` 可视化原图
6. 返回 `U16*` 指针给调用者

**注意**：调用者需要 `free()` 释放返回的内存。

#### 保存 RGB 图像 - `save_img()`

位置：[raw_process.cpp](file:///c:/Work/PerDoc/Github/raw_process/raw_process.cpp#L420-L458)

流程：
1. 创建 `cv::Mat(h, w, CV_8UC3)`
2. 将 `U16` RGB 右移到 8bit 填入 `cv::Mat`
3. 调用 `cv::imwrite()` 写入文件
4. 支持 JPEG 质量参数设置

#### 保存 Y 通道灰度图 - `save_y()`

位置：[raw_process.cpp](file:///c:/Work/PerDoc/Github/raw_process/raw_process.cpp#L380-L418)

用于保存中间结果（如LTM gain图、暗通道图等），将U16归一化到8bit后保存。

#### 带时间戳保存 - `save_img_with_timestamp()`

位置：[raw_process.cpp](file:///c:/Work/PerDoc/Github/raw_process/raw_process.cpp#L461-L483)

使用 `GetSystemTime()` 获取当前时分秒毫秒，生成文件名如 `HHMMSSmmm_suffix.jpg`，避免多次运行文件覆盖。

### 3.5 配置加载

#### `load_cfg()` - 硬编码加载默认配置

位置：[load_ini.cpp](file:///c:/Work/PerDoc/Github/raw_process/load_ini.cpp#L3-L195)

直接在代码中写死所有参数，包括：
- 图像基本参数
- LSC 四路增益表
- AWB 增益
- CCM 矩阵
- Gamma 曲线控制点

适合固定参数离线仿真，调试方便。

#### `load_cfg_from_ini()` - 从 INI 文件加载

位置：[load_ini.cpp](file:///c:/Work/PerDoc/Github/raw_process/load_ini.cpp#L240-L386)

支持从 INI 文件解析配置，支持基本参数、模块开关、OB、LSC增益表、AWB、CCM 等。可扩展支持更多参数。

INI 格式示例：
```ini
[raw_param]
bit = 16
used_bit = 10
order = LITTLE_ENDIAN
pattern = BGGR
width = 1440
height = 1080

[module_on]
ob_on = 1
lsc_on = 1
...

[ob]
ob = 64*16

[awb]
r_gain = 1024*1.85
g_gain = 1024*1.0
b_gain = 1024*1.85
```

支持简单表达式 `a*b`。

#### `calc_NAI()` - 计算CCM噪声放大指数

```
NAI_c = Σ (coeff^2 * gain_in^2)
```
用于评估CCM对噪声的放大程度，越小越好。

### 3.6 公共工具函数

全部在 [raw_process.cpp](file:///c:/Work/PerDoc/Github/raw_process/raw_process.cpp)：

| 函数 | 功能 |
|------|------|
| `clp_range(min, x, max)` | 裁剪x到[min, max] |
| `calc_inter(x0, x, y, len)` | 分段线性插值（用于Gamma查找） |
| `clear_tmp()` | 清空当前目录下所有图片文件 |
| `gauss_filter(y, h, w, r)` | 高斯滤波，返回新分配数组 |
| `mid_filter(y, h, w, r)` | 中值滤波 |
| `min_filter(y, h, w, r)` | 最小值滤波 |
| `raw2rgb()` | 将RAW Bayer转换为RGB用于可视化 |
| `print_prog(cur, total)` | 进度打印，控制每秒最多刷新一次 |
| `safe_free(p)` | 安全释放内存 |

### 3.7 添加新模块步骤

以添加一个新模块 `foo` 为例：

1. **新建文件**：`foo.h` 和 `foo.cpp`
2. **声明函数**（foo.h）：
   ```cpp
   #pragma once
   #include "raw_process.h"
   U8 foo_process(XXX* data, IMG_CONTEXT context, G_CONFIG cfg);
   ```
3. **实现函数**（foo.cpp）：
   ```cpp
   #include "foo.h"
   U8 foo_process(XXX* data, IMG_CONTEXT context, G_CONFIG cfg)
   {
       // 1. 检查开关
       if (cfg.foo_on == 0) return OK;
       
       // 2. 你的算法...
       
       // 3. DEBUG模式保存结果
   #if DEBUG_MODE
       LOG("done.");
       RGB* rgb = ...; // 转换为RGB
       save_img_with_timestamp(rgb, &context, "_foo");
   #endif
       return OK;
   }
   ```
4. **添加参数到 `G_CONFIG`**（raw_process.h）：
   ```cpp
   U8 foo_on;      // 开关
   int foo_param;  // 你的参数...
   ```
5. **在 `load_cfg()` 中初始化参数**（load_ini.cpp）
6. **在 `main()` 主流程合适位置添加调用**（raw_process.cpp）：
   ```cpp
   foo_process(data, context, cfg);
   ```
7. 编译运行，即可看到输出 `*_foo.jpg`

### 3.8 编译说明

**Visual Studio 编译**：
1. 打开 `raw_process.sln`
2. 配置 OpenCV 包含目录和库目录（VS 项目 → 属性 → VC++ 目录）
3. 生成解决方案
4. 可执行文件输出在 `x64/Release/raw_process.exe`

**依赖配置**：
- 需配置 OpenCV 头文件路径：`C:\Work\Documents\opencv454\build\include`
- 需配置库路径：`C:\Work\Documents\opencv454\build\x64\vc15\lib`
- 链接器输入：`opencv_world454.lib`（根据你的版本调整）

### 3.9 DEBUG 模式

在 [raw_process.h](file:///c:/Work/PerDoc/Github/raw_process/raw_process.h#L15) 定义：
```cpp
#define DEBUG_MODE 1
```
- `1`：每个模块处理完自动保存中间结果，输出日志
- `0`：只保存最终结果，速度更快

`LOG` 宏自动打印函数名和行号：
```cpp
LOG("time = %.2f s.", elapsed);
// 输出：main [xx]: time = 0.50 s.
```

---

## 四、处理效果示例

| 阶段 | 输出示例 |
|------|---------|
| 原始RAW | 070945626_origin.jpg |
| OB | 070945656_ob.jpg |
| LSC | 070945697_lsc.jpg |
| ISP Gain | 070945729_igain.jpg |
| GIC | 070945761_gic.jpg |
| AWB | 070945795_awb.jpg |
| Demosaic | 070945835_demosaic.jpg |
| CCM | 070945871_ccm.jpg |
| RGB Gamma | 070945995_rgbgamma.jpg |
| Defog | 070946320_defog.jpg |
| R2Y | 070946477_r2y.jpg |
| YNR | 070946817_ynr.jpg |
| CNR | 070952084_cnr.jpg |
| YUV TXI | 070952474_yuv_txi.jpg |
| **最终结果** | **070952568_end.jpg** |

---

## 许可证

本项目用于ISP学习和研究。