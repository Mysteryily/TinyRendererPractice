## TinyRenderer学习笔记

### LineDrawing

![image-20260516222234587](D:\cproject\TinyRendererPractice\assets\image-20260516222234587.png)

```c++
void line(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color) {
    bool steep = std::abs(ax-bx) < std::abs(ay-by);
    if (steep) { // if the line is steep, we transpose the image
        std::swap(ax, ay);
        std::swap(bx, by);
    }
    if (ax>bx) { // make it left−to−right
        std::swap(ax, bx);
        std::swap(ay, by);
    }
    float y = ay;
    for (int x=ax; x<=bx; x++) {
        if (steep) // if transposed, de−transpose
            framebuffer.set(y, x, color);
        else
            framebuffer.set(x, y, color);
        y += (by-ay) / static_cast<float>(bx-ax);
    }
}
```

### Triangle rasterization

判断点是否在三角形内部，三次叉乘即可。

![image-20260516224140669](D:\cproject\TinyRendererPractice\assets\image-20260516224140669.png)

有向三角形面积公式
![image-20260518230436895](D:\cproject\TinyRendererPractice\assets\image-20260518230436895.png)



### Hidden faces removal

画家算法:从后往前绘制从而前面的面片可以覆盖后面的面片

逐像素画家算法:通过z缓冲区对每个像素进行背面剔除

```
for each triangle t:
    for each pixel p that t covers:
        compute its depth z
        if depth buffer at p < z:
            update the depth buffer with z
            paint the pixel
```

对于点(x,y) = alpha * (ax,ay,az) + beta * (bx,by,bz) + gamma * (cx,cy,cz)，其z = alpha * az + beta * bz + gamma + cz

### Camera

朴素相机推导

![image-20260518231832981](D:\cproject\TinyRendererPractice\assets\image-20260518231832981.png)

Viewport视口矩阵

![image-20260519081726297](D:\cproject\TinyRendererPractice\assets\image-20260519081726297.png)

Perspective deformation透视

```
vec3 persp(vec3 v) {
    constexpr double f = 3.;
    return v / (1-v.z/f);
}
```

![image-20260519081805962](D:\cproject\TinyRendererPractice\assets\image-20260519081805962.png)

![image-20260520000217052](D:\cproject\TinyRendererPractice\assets\image-20260520000217052.png)

![image-20260520000232242](D:\cproject\TinyRendererPractice\assets\image-20260520000232242.png)

关于

### Shading

![image-20260520081805656](D:\cproject\TinyRendererPractice\assets\image-20260520081805656.png)

#### **Phong reflection model**

![image-20260521073707046](D:\cproject\TinyRendererPractice\assets\image-20260521073707046.png)

![image-20260521073839390](D:\cproject\TinyRendererPractice\assets\image-20260521073839390.png)通过**插值**平滑着色

通过纹理贴图、法线贴图、高光贴图、环境光贴图优化

### Tangent Space

![image-20260602072250579](D:\cproject\TinyRendererPractice\assets\image-20260602072250579.png)

## Shadow mapping

做法流程：

先计算相机视角的zbuffer1深度，然后从光源视角渲染得到光源视角的zbuffer，阴影判断

- 对每个屏幕像素 `(x, y)`，将 `(x, y, zbuffer1[x+y*width])` 变换回世界坐标 `fragment`。
- 再用将 `fragment` 变换到光源屏幕空间 `screen`。
- 比较 `screen.z` 与 `zbuffer` 中对应位置的值：若 `screen.z > zbuffer[...] - 0.05`，说明该点被遮挡，标记 `mask` 为 `true`（表示处于阴影中）



## Ambient occlusion

### 暴力AO

类似shadow mapping里面实现的，随机在周围取大量光源点，判断阴影，注意不是在shadow mapping里面的mask 只有true false而是一个浮点数来表示比例，最后得到一个整体的阴影比例



### SSAO

![image-20260608155912823](D:\cproject\TinyRendererPractice\assets\image-20260608155912823.png)



## Toon Shading

- **Start with standard lighting**

- **Quantize the intensities**，不要直接显示原始光照，将其限制在几个级别
- **Add outlines** 添加轮廓线，sobel算子边缘检测