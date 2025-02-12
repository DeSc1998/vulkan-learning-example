#pragma once

#include <string>
#include <vector>

namespace ds {

  struct Vertex {
    float x, y, z;
  };

  struct Face {
    size_t x, y, z;
  };

  struct Model {
    std::vector< Vertex > vertices;
    std::vector< Face >   faces;
  };

  Model model_from_file( const std::string );
}
