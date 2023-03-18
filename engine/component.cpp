
#include "component.hpp"

namespace ds {

  vk::VertexInputBindingDescription Component::binding_description( ) {
    vk::VertexInputBindingDescription tmp { };

    tmp.binding   = 0;
    tmp.stride    = sizeof( Component );
    tmp.inputRate = vk::VertexInputRate::eVertex;

    return tmp;
  }

  std::array< vk::VertexInputAttributeDescription, 3 >
    Component::attribute_description( ) {
    std::array< vk::VertexInputAttributeDescription, 3 > out { };

    out[0].binding  = 0;
    out[0].location = 0;
    out[0].format   = vk::Format::eR32G32B32A32Sfloat;
    out[0].offset   = offsetof( Component, pos );

    out[1].binding  = 0;
    out[1].location = 1;
    out[1].format   = vk::Format::eR32G32B32A32Sfloat;
    out[1].offset   = offsetof( Component, color );

    out[2].binding  = 0;
    out[2].location = 2;
    out[2].format   = vk::Format::eR32Sfloat;
    out[2].offset   = offsetof( Component, time );

    return out;
  }

}