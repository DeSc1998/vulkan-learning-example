
#include "pch.hpp"

#include "impl.hpp"

#define Unused( x ) (void)x

int main( int argc, char* argv[] ) {
  Unused( argc );
  Unused( argv );

  ds::Engine engine;

  engine.init_window( );
  engine.init_vulkan( );

  engine.loop( );
}
