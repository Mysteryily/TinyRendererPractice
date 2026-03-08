#include <algorithm>
#include <cstdlib>
#include "our_gl.h"
#include "model.h"

extern mat<4, 4> ModelView, Perspective; // "OpenGL" state matrices and
extern std::vector<double> zbuffer; // the depth buffer

struct PhongShader : IShader {
    const Model &model;
    TGAColor color = {};
    vec3 tri[3]; // triangle in eye coordinates
    double clip_w[3];
    vec3 l; // light direction in eye coordinates
    vec3 eye; // eye position in eye coordinates (origin)

    PhongShader(const Model &m, const vec3 light_, const vec3 eye_) : model(m) {
        // Transform light direction to eye space
        l = normalized((ModelView * vec4{light_.x, light_.y, light_.z, 0.}).xyz());
        // Eye in eye space is at origin
        eye = vec3{0, 0, 0};
    }

    virtual vec4 vertex(const int face, const int vert) {
        vec3 v = model.vert(face, vert).xyz(); // current vertex in object coordinates
        vec4 gl_Position = ModelView * vec4{v.x, v.y, v.z, 1.};
        tri[vert] = gl_Position.xyz(); // in eye coordinates
        // Save clip space w for perspective correction
        vec4 clipPos = Perspective * gl_Position;
        clip_w[vert] = clipPos.w;

        return clipPos; // in clip coordinates
    }

    virtual vec3 get_w() const {
        return vec3{clip_w[0], clip_w[1], clip_w[2]};
    }

    virtual std::pair<bool, TGAColor> fragment(const vec3 bar) const {
        // Perspective-correct interpolation of eye-space position
        double alpha = bar.x / clip_w[0];
        double beta = bar.y / clip_w[1];
        double gamma = bar.z / clip_w[2];
        double sum = alpha + beta + gamma;
        vec3 p = (tri[0] * alpha + tri[1] * beta + tri[2] * gamma) / sum;


        // Compute normal from interpolated position (in eye space)
        vec3 n = normalized(cross((tri[0] - tri[1]), (tri[0] - tri[2])));

        // Ambient
        float ambient = 0.1;

        // Diffuse (l is already normalized)
        float diffuse = std::max(0.0, l * n);

        // Specular
        vec3 v = normalized(eye - p); // View direction (eye is at origin)
        vec3 r = normalized(2.0 * n * (n * l) - l); // Reflection direction
        float specular = std::max(0.0, pow(v * r, 32.0));

        // Final intensity
        float intensity = ambient + diffuse + specular;
        intensity = std::min(1.0f, intensity); // Clamp to [0, 1]

        // Use white light color with intensity
        TGAColor frag_color = {
            static_cast<uint8_t>(255 * intensity),
            static_cast<uint8_t>(255 * intensity),
            static_cast<uint8_t>(255 * intensity),
            255
        };
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

    lookat(eye, center, up); // build the ModelView   matrix
    init_perspective(norm(eye - center)); // build the Perspective matrix
    init_viewport(width / 16, height / 16, width * 7 / 8, height * 7 / 8); // build the Viewport    matrix
    init_zbuffer(width, height);


    TGAImage framebuffer(width, height, TGAImage::RGB);

    Model model("../obj/diablo3_pose/diablo3_pose.obj");
    PhongShader shader(model, light, eye);

    for (int i = 0; i < model.nfaces(); i++) {
        // iterate through all triangles
        Triangle clip = {
            shader.vertex(i, 0), // assemble the primitive
            shader.vertex(i, 1),
            shader.vertex(i, 2)
        };
        rasterize(clip, shader, framebuffer); // rasterize the primitive
    }
    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}
