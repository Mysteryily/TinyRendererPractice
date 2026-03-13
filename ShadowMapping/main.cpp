#include <algorithm>
#include <cstdlib>
#include "our_gl.h"
#include "model.h"

extern mat<4, 4> ModelView, Perspective, Viewport; // "OpenGL" state matrices and
extern std::vector<double> zbuffer; // the depth buffer

struct DepthShader : IShader {
    const Model &model;

    DepthShader(const Model &m) : model(m) {
    }

    virtual vec4 vertex(const int face, const int vert) {
        vec4 v = model.vert(face, vert);
        vec4 gl_Position = ModelView * v;
        return Perspective * gl_Position;
    }

    virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const {
        return {false, {255, 255, 255}};
    }
};


struct PhongShader : IShader {
    const Model &model;
    TGAColor color = {};
    vec4 tri[3];
    vec4 light;
    vec2 tri_uv[3];
    vec4 tri_norm[3];

    PhongShader(const Model &m, const vec3 light_) : model(m) {
        // Transform light direction to eye space
        light = normalized(ModelView * vec4{light_.x, light_.y, light_.z, 0.});
    }

    virtual vec4 vertex(const int face, const int vert) {
        vec4 v = model.vert(face, vert); // current vertex in object coordinates
        vec4 gl_Position = ModelView * v;
        tri[vert] = gl_Position; // in eye coordinates
        tri_uv[vert] = model.uv(face, vert);
        tri_norm[vert] = ModelView.invert_transpose() * model.normal(face, vert);
        return Perspective * gl_Position; // in clip coordinates
    }


    virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const {
        vec4 p = tri[0] * bar.x + tri[1] * bar.y + tri[2] * bar.z;
        vec2 uv = tri_uv[0] * bar[0] + tri_uv[1] * bar[1] + tri_uv[2] * bar[2];
        mat<2, 4> E = {tri[1] - tri[0], tri[2] - tri[0]};
        mat<2, 2> U = {tri_uv[1] - tri_uv[0], tri_uv[2] - tri_uv[0]};
        mat<2, 4> T = U.invert() * E;
        mat<4, 4> D = {
            normalized(T[0]), // tangent vector
            normalized(T[1]), // bitangent vector
            normalized(tri_norm[0] * bar[0] + tri_norm[1] * bar[1] + tri_norm[2] * bar[2]), // interpolated normal
            {0, 0, 0, 1}
        }; // Darboux frame
        vec4 n = normalized(D.transpose() * model.normal(uv));

        // Sample diffuse map
        TGAColor diff_color = model.diffuse().get(uv[0] * model.diffuse().width(), uv[1] * model.diffuse().height());
        // Ambient
        double ambient = 0.4;

        // Diffuse
        double diffuse = std::max(0.0, light * n);

        // Specular
        vec4 eye = {0., 0., 0., 1};
        vec4 v = normalized(eye - p); // View direction (eye is at origin)
        vec4 r = normalized(n * (n * light) * 2 - light); // Reflection direction
        TGAColor spec_color = model.specular().get(uv[0] * model.specular().width(), uv[1] * model.specular().height());
        double spec_intensity = spec_color[0] / 255.0;
        double specular = spec_intensity * std::max(0.0, pow(v * r, 32));

        // Final color
        TGAColor frag_color;
        for (int channel: {0, 1, 2})
            frag_color[channel] = std::min<int>(255, diff_color[channel] * (ambient + diffuse + specular));
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
    Model floor("../obj/floor.obj");
    PhongShader model_shader(model, light);
    for (int i = 0; i < model.nfaces(); i++) {
        // iterate through all triangles
        Triangle clip = {
            model_shader.vertex(i, 0), // assemble the primitive
            model_shader.vertex(i, 1),
            model_shader.vertex(i, 2)
        };
        rasterize(clip, model_shader, framebuffer); // rasterize the primitive
    }

    PhongShader floor_shader(floor, light);
    for (int i = 0; i < floor.nfaces(); i++) {
        // iterate through all triangles
        Triangle clip = {
            floor_shader.vertex(i, 0), // assemble the primitive
            floor_shader.vertex(i, 1),
            floor_shader.vertex(i, 2)
        };
        rasterize(clip, floor_shader, framebuffer); // rasterize the primitive
    }
    std::vector<double> zbuffer1 = zbuffer;
    mat<4, 4> M = (Viewport * Perspective * ModelView).invert();
    framebuffer.write_tga_file("framebuffer.tga");

    TGAImage depth_buffer1(width, height, TGAImage::RGB);
    DepthShader model_back_shader(model);
    lookat(light, center, up);
    init_perspective(norm(light - center));
    init_viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8);
    init_zbuffer(width, height);
    for (int i = 0; i < model.nfaces(); i++) {
        Triangle clip = {
            model_back_shader.vertex(i, 0), // assemble the primitive
            model_back_shader.vertex(i, 1),
            model_back_shader.vertex(i, 2)
        };
        rasterize(clip, model_back_shader, depth_buffer1); // rasterize the primitive
    }
    DepthShader floor_back_shader(floor);
    for (int i = 0; i < floor.nfaces(); i++) {
        // iterate through all triangles
        Triangle clip = {
            floor_back_shader.vertex(i, 0), // assemble the primitive
            floor_back_shader.vertex(i, 1),
            floor_back_shader.vertex(i, 2)
        };
        rasterize(clip, floor_back_shader, depth_buffer1); // rasterize the primitive
    }
    depth_buffer1.write_tga_file("depth_buffer.tga");


    std::vector<bool> mask(width * height, false);
    mat<4, 4> N = Viewport * Perspective * ModelView;
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            vec4 fragment = M * vec4{static_cast<double>(x), static_cast<double>(y), zbuffer1[x + y * width], 1.0};
            vec4 light_frag = N * fragment;
            vec3 screen = {light_frag.x / light_frag.w, light_frag.y / light_frag.w, light_frag.z / light_frag.w};
            if (screen.z < -999 || screen.x < 0 || screen.x >= width || screen.y < 0 || screen.y >= height || screen.z > zbuffer[int(screen.x) + int(screen.y) * width] - 0.05) {
                mask[x + y * width] = true;
            }
        }
    }
    TGAImage maskimg(width, height, TGAImage::GRAYSCALE);
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            if (mask[x + y * width]) {
                maskimg.set(x, y, {255, 255, 255, 255});
            }
        }
    }
    maskimg.write_tga_file("mask.tga");


    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            if (mask[x + y * width]) continue;
            TGAColor c = framebuffer.get(x, y);
            vec3 a = {static_cast<double>(c[0]), static_cast<double>(c[1]), static_cast<double>(c[2])};
            if (norm(a) < 80) continue;
            a = normalized(a) * 80;
            framebuffer.set(x, y, {static_cast<uint8_t>(a[0]), static_cast<uint8_t>(a[1]), static_cast<uint8_t>(a[2]), 255});
        }
    }
    framebuffer.write_tga_file("shadow.tga");
    return 0;
}
