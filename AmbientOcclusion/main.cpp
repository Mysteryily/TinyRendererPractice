#include <algorithm>
#include <chrono>
#include <complex>
#include <random>

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
    DepthShader model_shader(model);
    for (int i = 0; i < model.nfaces(); i++) {
        // iterate through all triangles
        Triangle clip = {
            model_shader.vertex(i, 0), // assemble the primitive
            model_shader.vertex(i, 1),
            model_shader.vertex(i, 2)
        };
        rasterize(clip, model_shader, framebuffer); // rasterize the primitive
    }

    DepthShader floor_shader(floor);
    for (int i = 0; i < floor.nfaces(); i++) {
        // iterate through all triangles
        Triangle clip = {
            floor_shader.vertex(i, 0), // assemble the primitive
            floor_shader.vertex(i, 1),
            floor_shader.vertex(i, 2)
        };
        rasterize(clip, floor_shader, framebuffer); // rasterize the primitive
    }
    framebuffer.write_tga_file("framebuffer.tga");


    constexpr double ao_radius = .1;
    constexpr int nsamples = 128;
    std::mt19937 gen(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_real_distribution<float> dist(-ao_radius, ao_radius);
    auto smoothstep = [](double edge0, double edge1, double x) {
        // smoothstep returns 0 if the input is less than the left edge,
        double t = std::clamp((x - edge0) / (edge1 - edge0), 0., 1.); // 1 if the input is greater than the right edge,
        return t * t * (3 - 2 * t); // Hermite interpolation inbetween. The derivative of the smoothstep function is zero at both edges.
    };


    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            double z = zbuffer[x + y * width];
            if (z < -100) {
                continue;
            }
            vec4 fragment = Viewport.invert() * vec4{static_cast<double>(x), static_cast<double>(y), z, 1.};
            double weight = 0;
            int frag_weights = 0;
            for (int i = 0; i < nsamples; i++) {
                // compute a very rough approximation of the solid angle
                vec4 p = Viewport * (fragment + vec4{dist(gen), dist(gen), dist(gen), 0.});
                if (p.x < 0 || p.x >= width || p.y < 0 || p.y >= height) continue;
                double d = zbuffer[static_cast<int>(p.x) + static_cast<int>(p.y) * width];
                if (z + 5 * ao_radius < d) continue;
                frag_weights++;
                weight += d > p.z;
            }
            weight = smoothstep(0, 1, 1 - weight / frag_weights * .4);
            TGAColor color = framebuffer.get(x, y);
            framebuffer.set(x, y, {static_cast<uint8_t>(color[0] * weight), static_cast<uint8_t>(color[1] * weight), static_cast<uint8_t>(color[2] * weight), 255});
        }
    }
    framebuffer.write_tga_file("shadow.tga");
    return 0;
}
