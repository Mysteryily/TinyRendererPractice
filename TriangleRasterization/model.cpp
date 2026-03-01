#include <iostream>
#include <sstream>
#include "model.h"

void parse_face_vertex(const std::string& vertex_str, int& v, int& vt, int& vn) {
    std::istringstream iss(vertex_str);
    std::string token;
    
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = vertex_str.find('/');
    
    while (end != std::string::npos) {
        tokens.push_back(vertex_str.substr(start, end - start));
        start = end + 1;
        end = vertex_str.find('/', start);
    }
    tokens.push_back(vertex_str.substr(start));
    
    if (tokens.size() >= 1 && !tokens[0].empty()) {
        v = std::stoi(tokens[0]) - 1;
    }
    if (tokens.size() >= 2 && !tokens[1].empty()) {
        vt = std::stoi(tokens[1]) - 1;
    }
    if (tokens.size() >= 3 && !tokens[2].empty()) {
        vn = std::stoi(tokens[2]) - 1;
    }
}

bool Model::load_obj(const std::string& filename) 
{
    std::ifstream in(filename);
    if (!in.is_open()) {
        std::cerr << "Cannot open file: " << filename << std::endl;
        return false;
    }
    
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream iss(line);
        std::string token;
        iss >> token;
        
        if (token == "v") {
            float x, y, z;
            iss >> x >> y >> z;
            verts.push_back(Vec3f(x, y, z));
        }
        else if (token == "vn") {
            float x, y, z;
            iss >> x >> y >> z;
            norms.push_back(Vec3f(x, y, z));
        }
        else if (token == "vt") {
            float u, v;
            iss >> u >> v;
            uvs.push_back(Vec3f(u, v, 0));
        }
        else if (token == "f") {
            Face face;
            for (int i = 0; i < 3; i++) {
                face.v[i] = face.vt[i] = face.vn[i] = -1;
            }
            
            std::string vertex;
            int idx = 0;
            while (iss >> vertex && idx < 3) {
                parse_face_vertex(vertex, face.v[idx], face.vt[idx], face.vn[idx]);
                idx++;
            }
            
            if (idx == 3) {
                faces.push_back(face);
            }
        }
    }
    std::cout << "Loaded model: " 
                << verts.size() << " verts, " 
                << faces.size() << " faces" << std::endl;
    return true;
}