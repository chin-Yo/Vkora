#pragma once
#include <ctime>
#include <string>

namespace asset
{
    namespace meta
    {
        struct MeshAsset
        {
            std::string name; // File name (without extension), for example "suzanne"
            std::string fullPath; // Absolute path, such as "assets/models/suzanne.obj"
            std::string extension; // ".obj", ".glb"
            time_t lastModified; // Used for determining hot updates
        };
    }
}
