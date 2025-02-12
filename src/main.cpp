
#include "component.hpp"
#include "impl.hpp"
#include "object.hpp"
#include <fmt/base.h>

#define Unused( x ) (void)x

void print_components( const std::vector< ds::Component >& verts ) {
  for ( const auto& vert : verts ) {
    std::cout << '{' << vert.pos[0] << ' ' << vert.pos[1] << ' ' << vert.pos[2]
              << " | " << vert.color[0] << ' ' << vert.color[1] << ' '
              << vert.color[2] << ' ' << vert.color[3] << " | " << vert.time
              << "}\n";
  }
}

int main( int argc, char* argv[] ) {
  Unused( argc );
  Unused( argv );

  ds::Engine engine;

  const auto verts = ds::Component::read_from_file( "obj/pyramid.obj" );
  const auto model = ds::model_from_file( "obj/pyramid.obj.bak" );
  print_components( verts );

  fmt::print( "INFO: vertices\n" );
  for ( const auto vertex : model.vertices ) {
    const auto [x, y, z] = vertex;
    fmt::print( "{{ {}, {}, {} }}\n", x, y, z );
  }

  fmt::print( "INFO: faces\n" );
  for ( const auto face : model.faces ) {
    const auto [x, y, z] = face;
    fmt::print( "{{ {}, {}, {} }}\n", x, y, z );
  }

  engine.set_components( verts );

  engine.init_window( );
  engine.init_vulkan( );

  engine.loop( );
}
