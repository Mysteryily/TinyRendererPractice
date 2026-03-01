#include <cmath>
#include <cstdlib>
#include <iostream>
#include <chrono>
#include "tgaimage.h"
#include "model.h"

const TGAColor white   = {255, 255, 255, 255}; // RGB order (note: constructor maps R->b, G->g, B->r, A->a)
const TGAColor green   = {  0, 255,   0, 255};
const TGAColor red     = {255,   0,   0, 255};
const TGAColor blue    = {  0,   0, 255, 255};
const TGAColor yellow  = {255, 255,   0, 255};


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
    int y = ay;
    int ierror = 0;
    for (int x=ax; x<=bx; x++) {
        if (steep) // if transposed, de−transpose
            framebuffer.set(y, x, color);
        else
            framebuffer.set(x, y, color);
        ierror += 2 * std::abs(by-ay);
        if (ierror > bx - ax) {
            y += by > ay ? 1 : -1;
            ierror -= 2 * (bx-ax);
        }
    }
}
int main(int argc, char** argv) {
    constexpr int width  = 800;
    constexpr int height = 800;
    TGAImage framebuffer(width, height, TGAImage::RGB);
    Model model;
    if (!model.load_obj("../diablo3_pose.obj")) {
        std::cerr << "Failed to load model" << std::endl;
        return -1;
    }

    // Scale and translate model to fit in the image
    // First, find the bounding box of the model
    float min_x = model.verts[0].x, max_x = model.verts[0].x;
    float min_y = model.verts[0].y, max_y = model.verts[0].y;
    for (const auto& v : model.verts) {
        min_x = std::min(min_x, v.x);
        max_x = std::max(max_x, v.x);
        min_y = std::min(min_y, v.y);
        max_y = std::max(max_y, v.y);
    }

    // Calculate scaling and translation to fit in image with padding
    float padding = 10.0f;
    float scale_x = (width - 2 * padding) / (max_x - min_x);
    float scale_y = (height - 2 * padding) / (max_y - min_y);
    float scale = std::min(scale_x, scale_y);

    // Center the model
    float offset_x = (width - (max_x - min_x) * scale) / 2.0f - min_x * scale;
    float offset_y = (height - (max_y - min_y) * scale) / 2.0f - min_y * scale;

    for (const auto& face : model.faces) {
        for (int i = 0; i < 3; i++) {
            Vec3f v0 = model.verts[face.v[i]];
            Vec3f v1 = model.verts[face.v[(i+1)%3]];

            // Transform coordinates
            int x0 = static_cast<int>((v0.x * scale + offset_x));
            int y0 = static_cast<int>((v0.y * scale + offset_y));
            int x1 = static_cast<int>((v1.x * scale + offset_x));
            int y1 = static_cast<int>((v1.y * scale + offset_y));

            line(x0, y0, x1, y1, framebuffer, red);
        }
    }
    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}
