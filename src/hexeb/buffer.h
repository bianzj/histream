//
// Created by admin on 2024/1/28.
//

#ifndef FIELD_BUFFER_H
#define FIELD_BUFFER_H

#include <nvvk/buffers_vk.hpp>
#include "hexebio.h"



class Buffer {

public:
    Buffer(){}
    bool createBuffer(std::shared_ptr<HexebIO> &voxellstio);
    void destroy(std::shared_ptr<HexebIO> &voxellstio);
};


#endif //FIELD_BUFFER_H
