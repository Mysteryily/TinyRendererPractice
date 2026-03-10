#include <algorithm>
#include <cstdlib>
#include "our_gl.h"
#include "model.h"

extern mat<4, 4> ModelView, Perspective; // "OpenGL" state matrices and
extern std::vector<double> zbuffer; // the depth buffer

struct PhongShader : IShader {
    const Model &model;
    TGAColor color = {};
    vec4 tri[3];
    vec4 light;
    vec2 tri_uv[3];

    PhongShader(const Model &m, const vec3 light_) : model(m) {
        // Transform light direction to eye space
        light = normalized(ModelView * vec4{light_.x, light_.y, light_.z, 0.});
    }

    virtual vec4 vertex(const int face, const int vert) {
        vec4 v = model.vert(face, vert); // current vertex in object coordinates
        vec4 gl_Position = ModelView * v;
        tri[vert] = gl_Position; // in eye coordinates
        tri_uv[vert] = model.uv(face, vert);
        return Perspective * gl_Position; // in clip coordinates
    }


    virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const {
        vec4 p = tri[0] * bar.x + tri[1] * bar.y + tri[2] * bar.z;
        vec2 uv = tri_uv[0] * bar[0] + tri_uv[1] * bar[1] + tri_uv[2] * bar[2];
        vec4 n = normalized(ModelView.invert_transpose() * model.normal(uv));

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
        double specular = spec_intensity * std::max(0.0, pow( v * r, 32));

        // Final color
        TGAColor frag_color;
        for (int channel : {0,1,2})
            frag_color[channel] = std::min<int>(255, diff_color[channel]*(ambient + diffuse + specular));
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

    Model model("../obj/african_head/african_head.obj");
    PhongShader shader(model, light);

    for (int i = 0; i < model.nfaces(); i++) {
        // iterate through all triangles
        Triangle clip = {
            shader.vertex(i, 0), // assemble the primitive
            shader.vertex(i, 1),
            shader.vertex(i, 2)
        };
        rasterize(clip, shader, framebuffer); // rasterize the primitive
    }

    Model eye_inner_model("../obj/african_head/african_head_eye_inner.obj");
    for (int i = 0; i < eye_inner_model.nfaces(); i++) {
        Triangle clip = {
            shader.vertex(i, 0),
            shader.vertex(i, 1),
            shader.vertex(i, 2)
        };
        rasterize(clip, shader, framebuffer);
    }

    Model eye_outer_model("../obj/african_head/african_head_eye_outter.obj");
    for (int i = 0; i < eye_outer_model.nfaces(); i++) {
        Triangle clip = {
            shader.vertex(i, 0),
            shader.vertex(i, 1),
            shader.vertex(i, 2)
        };
        rasterize(clip, shader, framebuffer);
    }
    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}
