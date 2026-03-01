#include "utils.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>



struct Model {
    std::vector<Vec3f> verts;
    std::vector<Vec3f> norms;
    std::vector<Vec3f> uvs;
    struct Face {
        int v[3];  
        int vt[3]; 
        int vn[3]; 
    };
    std::vector<Face> faces;

    bool load_obj(const std::string& filename); 
};