#include <algorithm>
#include <chrono>
#include <complex>
#include <random>

#include "our_gl.h"
#include "model.h"

extern mat<4, 4> ModelView, Perspective, Viewport; // "OpenGL" state matrices and
extern std::vector<double> zbuffer; // the depth buffer

struct ToonShader : IShader {
    const Model &model;
    TGAColor color = {};
    vec4 tri[3];
    vec4 light;
    vec4 tri_norm[3];

    ToonShader(const Model &m, const vec3 light_) : model(m) {
        light = normalized(ModelView * vec4{light_.x, light_.y, light_.z, 0.});
    }

    virtual vec4 vertex(const int face, const int vert) {
        vec4 v = model.vert(face, vert);
        vec4 gl_Position = ModelView * v;
        tri_norm[vert] = ModelView.invert_transpose() * model.normal(face, vert);
        return Perspective * gl_Position;
    }


    virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const {
        vec4 p = tri[0] * bar.x + tri[1] * bar.y + tri[2] * bar.z;
        vec4 n = normalized(tri_norm[0] * bar[0] + tri_norm[1] * bar[1] + tri_norm[2] * bar[2]);

        // Ambient
        double ambient = 0.1;
        // Diffuse
        double diffuse = std::max(0.0, light * n);
        // Specular
        vec4 eye = {0., 0., 0., 1};
        vec4 v = normalized(eye - p); // View direction (eye is at origin)
        vec4 r = normalized(n * (n * light) * 2 - light); // Reflection direction
        double specular = std::max(0.0, pow(v * r, 32));
        double intensity = std::min(1.0, ambient + diffuse + specular);
        if (intensity > .66) {
            intensity = 1;
        } else if (intensity > .33) {
            intensity = .66;
        } else {
            intensity = .33;
        }
        TGAColor frag_color;
        for (int channel: {0, 1, 2})
            frag_color[channel] = std::min<int>(255, color[channel] * intensity);
        return {false, frag_color};
    }
};


int main(int argc, char **argv) {
    constexpr int width = 800; // output image size
    constexpr int height = 800;
    const vec3 light{1, 1, 1};
    const vec3 eye{-1, 0, 2}; // camera position
    const vec3 center{0, 0, 0}; // camera direction
    const vec3 up{0, 1, 0}; // camera up vector

    lookat(eye, center, up); // build the ModelView matrix
    init_perspective(norm(eye - center)); // build the Perspective matrix
    init_viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8); // build the Viewport    matrix
    init_zbuffer(width, height);
    TGAImage framebuffer(width, height, TGAImage::RGB);

    Model model("../obj/diablo3_pose/diablo3_pose.obj");
    ToonShader model_shader(model, light);
    model_shader.color = TGAColor{0, 0, 255};
    for (int i = 0; i < model.nfaces(); i++) {
        // iterate through all triangles
        Triangle clip = {
            model_shader.vertex(i, 0), // assemble the primitive
            model_shader.vertex(i, 1),
            model_shader.vertex(i, 2)
        };
        rasterize(clip, model_shader, framebuffer); // rasterize the primitive
    }
    constexpr double threshold = .15;
    constexpr int Gx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    constexpr int Gy[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};
    for (int x = 1; x < framebuffer.width() - 1; ++x) {
        for (int y = 1; y < framebuffer.height() - 1; ++y) {
            vec2 sum;
            for (int i = -1; i <= 1; ++i) {
                for (int j = -1; j <= 1; ++j) {
                    sum = sum + vec2{
                              Gx[j + 1][i + 1] * zbuffer[x + i + (y + j) * width],
                              Gy[j + 1][i + 1] * zbuffer[x + i + (y + j) * width]
                          };
                }
            }
            if (norm(sum) > threshold)
                framebuffer.set(x, y, TGAColor{0, 0, 0, 255});
        }
    }
    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}
