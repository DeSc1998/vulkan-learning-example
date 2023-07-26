
#include "pch.hpp"

#include "component.hpp"
#include "impl.hpp"

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
  print_components( verts );

  engine.set_components( verts );

  engine.init_window( );
  engine.init_vulkan( );

  engine.loop( );
}
