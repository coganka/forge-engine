#include "ObjLoader.hpp"


#include <array>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <tuple>


namespace
{
    int resolveIndex(const std::string& text, size_t count)
    {
        if (text.empty())
        {
            return -1;
        }

        int index = std::stoi(text);

        if (index == 0)
        {
            throw std::runtime_error("Invalid OBJ index: 0");
        }

        int resolved = index > 0 ? index - 1 : static_cast<int>(count) + index;

        if (resolved < 0 || static_cast<size_t>(resolved) >= count)
        {
            throw std::runtime_error("OBJ index out of range");
        }

        return resolved;
    }
}

MeshData ObjLoader::load(const std::string& path)
{
    std::ifstream file(path);

    if (!file)
    {
        throw std::runtime_error("Cannot open OBJ: " + path);
    }

    using Vec2 = std::array<float, 2>;
    using Vec3 = std::array<float, 3>;
    using Key = std::tuple<int, int, int>;

    std::vector<Vec3> positions;
    std::vector<Vec2> texcoords;
    std::vector<Vec3> normals;

    std::map<Key, uint32_t> vertexLookup;
    MeshData result;

    std::string line;

    while (std::getline(file, line))
    {
        std::istringstream stream(line);
        std::string type;
        stream >> type;

        if (type == "v")
        {
            Vec3 p{};

            if (!(stream >> p[0] >> p[1] >> p[2]))
            {
                throw std::runtime_error("Invalid OBJ position");
            }

            positions.push_back(p);
        }
        else if (type == "vt")
        {
            Vec2 uv{};

            if (!(stream >> uv[0] >> uv[1]))
            {
                throw std::runtime_error("Invalid OBJ UV");
            }

            texcoords.push_back(uv);
        }
        else if (type == "vn")
        {
            Vec3 n{};

            if (!(stream >> n[0] >> n[1] >> n[2]))
            {
                throw std::runtime_error("Invalid OBJ normal");
            }

            normals.push_back(n);
        }
        else if (type == "f")
        {
            std::vector<uint32_t> face;
            std::string token;

            while (stream >> token)
            {
                if (token[0] == '#')
                {
                    break;
                }

                size_t first = token.find('/');
                size_t second = first == std::string::npos ? std::string::npos
                                                           : token.find('/', first + 1);

                std::string p = token.substr(0, first);
                std::string t =
                    first == std::string::npos
                        ? ""
                        : token.substr(
                              first + 1,
                              second == std::string::npos ? second : second - first - 1
                          );

                std::string n =
                    second == std::string::npos ? "" : token.substr(second + 1);

                int pi = resolveIndex(p, positions.size());
                int ti = resolveIndex(t, texcoords.size());
                int ni = resolveIndex(n, normals.size());

                if (pi < 0)
                {
                    throw std::runtime_error("Face missing position");
                }

                Key key{pi, ti, ni};

                auto it = vertexLookup.find(key);

                if (it == vertexLookup.end())
                {
                    const auto& pos = positions[pi];

                    Vec2 uv = ti >= 0 ? texcoords[ti] : Vec2{};
                    Vec3 normal = ni >= 0 ? normals[ni] : Vec3{};

                    Vertex vertex{
                        pos[0],
                        pos[1],
                        pos[2],
                        1.0f,
                        1.0f,
                        1.0f,
                        uv[0],
                        uv[1],
                        normal[0],
                        normal[1],
                        normal[2]
                    };

                    uint32_t index = static_cast<uint32_t>(result.vertices.size());

                    result.vertices.push_back(vertex);
                    vertexLookup.emplace(key, index);
                    face.push_back(index);
                }
                else
                {
                    face.push_back(it->second);
                }
            }

            if (face.size() < 3)
            {
                throw std::runtime_error("Face needs 3+ vertices");
            }

            for (size_t i = 1; i + 1 < face.size(); ++i)
            {
                result.indices.push_back(face[0]);
                result.indices.push_back(face[i]);
                result.indices.push_back(face[i + 1]);
            }
        }
    }

    if (result.indices.empty())
    {
        throw std::runtime_error("OBJ contains no faces");
    }

    return result;
}
