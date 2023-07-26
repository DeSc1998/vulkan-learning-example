#pragma once

#include <cassert>
#define VULKAN_HPP_DISABLE_ENHANCED_MODE
#include <vulkan/vulkan.hpp>

#include "utility.hpp"
#include <fmt/format.h>

namespace ds {

  template < typename T >
  class GPU_Memory {
    vk::Device       device;
    vk::Buffer       buffer;
    vk::DeviceMemory device_memory;
    T*               memory = nullptr;
    size_t           size   = 0;

  public:
    GPU_Memory( ) = default;

    GPU_Memory( const GPU_Memory& other )            = delete;
    GPU_Memory& operator=( const GPU_Memory& other ) = delete;

    GPU_Memory( GPU_Memory&& other )
      : device( other.device )
      , buffer( other.buffer )
      , device_memory( other.device_memory )
      , memory( other.memory )
      , size( other.size ) {
      other.device        = nullptr;
      other.buffer        = nullptr;
      other.device_memory = nullptr;
      other.memory        = nullptr;
      other.size          = 0;
    }

    GPU_Memory& operator=( GPU_Memory&& other ) {
      if ( memory != nullptr ) {
        this->~GPU_Memory( );
      }
      device        = other.device;
      buffer        = other.buffer;
      device_memory = other.device_memory;
      memory        = other.memory;
      size          = other.size;

      other.device        = nullptr;
      other.buffer        = nullptr;
      other.device_memory = nullptr;
      other.memory        = nullptr;
      other.size          = 0;

      return *this;
    }

    GPU_Memory( vk::Device dev, vk::PhysicalDevice physical_device,
                size_t          count,
                vk::SharingMode mode = vk::SharingMode::eExclusive )
      : device( dev )
      , size( count ) {
      auto find_mem_type
        = [physical_device]( uint32_t filter, vk::MemoryPropertyFlags flags ) {
            vk::PhysicalDeviceMemoryProperties mem_props;
            physical_device.getMemoryProperties( &mem_props );
            for ( size_t i = 0; i < mem_props.memoryTypeCount; ++i ) {
              if ( filter & ( 1 << i )
                   && ( mem_props.memoryTypes[i].propertyFlags & flags )
                        == flags ) {
                return i;
              }
            }

            error( "failed to find suitable memory" );
            exit( 1 );
          };

      using Flags = vk::MemoryPropertyFlagBits;

      vk::BufferCreateInfo   buffer_info { };
      vk::MemoryAllocateInfo alloc_info { };
      vk::MemoryRequirements requirements { };

      buffer_info.sharingMode = mode;
      buffer_info.size        = sizeof( T ) * size;
      buffer_info.usage       = vk::BufferUsageFlagBits::eVertexBuffer;

      auto result = device.createBuffer( &buffer_info, nullptr, &buffer );
      device.getBufferMemoryRequirements( buffer, &requirements );

      alloc_info.allocationSize = requirements.size;
      alloc_info.memoryTypeIndex
        = find_mem_type( requirements.memoryTypeBits,
                         Flags::eHostVisible | Flags::eHostCoherent );

      result = device.allocateMemory( &alloc_info, nullptr, &device_memory );
      exit_on_fail( "failed to allocate device memory", result );

      (void)device.bindBufferMemory( buffer, device_memory, 0 );

      result = device.mapMemory( device_memory, 0, buffer_info.size,
                                 (vk::MemoryMapFlags)0, (void**)&memory,
                                 vk::DispatchLoaderStatic( ) );
      exit_on_fail( "failed to map memory", result );
    }

    const T& operator[]( size_t offset ) const {
      assert( offset < size && "trying to access memory out of bound on GPU" );
      return memory[offset];
    }

    T& operator[]( size_t offset ) {
      assert( offset < size && "trying to access memory out of bound on GPU" );
      return memory[offset];
    }

    vk::Buffer* expose_buffer( ) { return &buffer; }

    ~GPU_Memory( ) {
      if ( memory != nullptr ) {
        (void)device.waitIdle( );

        device.unmapMemory( device_memory );
        device.freeMemory( device_memory, nullptr );
        device.destroy( buffer, nullptr );

        memory = nullptr;
      }
    }
  };

}