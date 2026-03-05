#include <cmath>
#include <tuple>
#include <algorithm>
#include "geometry.h"
#include "model.h"
#include "tgaimage.h"

constexpr int width = 800;
constexpr int height = 800;

double signed_triangle_area(int ax, int ay, int bx, int by, int cx, int cy) {
    return .5 * ((by - ay) * (bx + ax) + (cy - by) * (cx + bx) + (ay - cy) * (ax + cx));
}

vec3 rot(vec3 v) {
    constexpr double a = M_PI / 6;
    mat<3, 3> Ry;
    Ry[0] = vec<3>(std::cos(a), 0, std::sin(a));
    Ry[1] = vec<3>(0, 1, 0);
    Ry[2] = vec<3>(-std::sin(a), 0, std::cos(a));
    return Ry * v;
}

vec3 persp(vec3 v) {
    constexpr double c = 3.;
    return v / (1 - v.z / c);
}


void triangle(int ax, int ay, int az, int bx, int by, int bz, int cx, int cy, int cz, TGAImage &zbuffer, TGAImage &framebuffer, TGAColor color) {
    int bbminx = std::min(std::min(ax, bx), cx); // bounding box for the triangle
    int bbminy = std::min(std::min(ay, by), cy); // defined by its top left and bottom right corners
    int bbmaxx = std::max(std::max(ax, bx), cx);
    int bbmaxy = std::max(std::max(ay, by), cy);
    double total_area = signed_triangle_area(ax, ay, bx, by, cx, cy);
    if (total_area < 1) return; // backface culling + discarding triangles that cover less than a pixel

    // [-1, 1] rot-> [(1 - sqrt(3))/2, (sqrt(3)+3)/2] project-> [(1 - sqrt(3))/2, (sqrt(3)+3)/2] * 255 / 2
    const double sqrt3 = std::sqrt(3.0);
    const double low = (1.0 - sqrt3) / 2.0 * 255.0 / 2.0;
    const double high = (sqrt3 + 3.0) / 2.0 * 255.0 / 2.0;
    const double range = high - low;

    for (int x = bbminx; x <= bbmaxx; x++) {
        for (int y = bbminy; y <= bbmaxy; y++) {
            double alpha = signed_triangle_area(x, y, bx, by, cx, cy) / total_area;
            double beta = signed_triangle_area(x, y, cx, cy, ax, ay) / total_area;
            double gamma = signed_triangle_area(x, y, ax, ay, bx, by) / total_area;
            if (alpha < 0 || beta < 0 || gamma < 0) continue; // negative barycentric coordinate => the pixel is outside the triangle

            double z_interp = alpha * az + beta * bz + gamma * cz;
            double z_mapped = (z_interp - low) * 255.0 / range;
            if (z_mapped <= zbuffer.get(x, y)[0]) continue;
            uint8_t z = static_cast<uint8_t>(std::clamp(z_mapped, 0.0, 255.0));
            zbuffer.set(x, y, {z});
            // Use {z, 0, 0} as color to visualize more clearly
            framebuffer.set(x, y, {z,0,0});
        }
    }
}

std::tuple<int, int, int> project(vec3 v) {
    // First of all, (x,y) is an orthogonal projection of the vector (x,y,z).
    return {
        (v.x + 1.) * width / 2, // Second, since the input models are scaled to have fit in the [-1,1]^3 world coordinates,
        (v.y + 1.) * height / 2, // we want to shift the vector (x,y) and then scale it to span the entire screen.
        (v.z + 1.) * 255. / 2
    };
}

int main(int argc, char **argv) {
    Model model("../obj/diablo3_pose/diablo3_pose.obj");
    TGAImage framebuffer(width, height, TGAImage::RGB);
    TGAImage zbuffer(width, height, TGAImage::GRAYSCALE);

    for (int i = 0; i < model.nfaces(); i++) {
        // iterate through all triangles
        auto [ax, ay, az] = project(persp(rot(model.vert(i, 0).xyz())));
        auto [bx, by, bz] = project(persp(rot(model.vert(i, 1).xyz())));
        auto [cx, cy, cz] = project(persp(rot(model.vert(i, 2).xyz())));
        TGAColor rnd;
        for (int c = 0; c < 3; c++) rnd[c] = std::rand() % 255;
        triangle(ax, ay, az, bx, by, bz, cx, cy, cz, zbuffer, framebuffer, rnd);
    }

    framebuffer.write_tga_file("framebuffer.tga");
    zbuffer.write_tga_file("zbuffer.tga");
    return 0;
}
