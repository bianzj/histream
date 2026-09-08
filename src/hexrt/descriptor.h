//
// Created by admin on 2024/1/25.
//

#ifndef FIELD_DESCRIPTOR_H
#define FIELD_DESCRIPTOR_H

#include "hexrtio.h"

class Descriptor {
public:

    enum HexrtbindingInd
    {
        spectral,  // for the specific band
        thermal,  // for the voxel with constant temperature;
        canopy,
        meshLink,
        instanceLink,
        voxelLink,
        nano,
        tlas,
        sensor,
        wave,
        light,
        dir,
        rads,
        netRad,
        storage,
        lad,
        hex, // 异质性体元参数 (Ax, Ay, Az, rho)
    };


    bool createDescriptor(std::shared_ptr<HexrtIO> &raytracingio);
    void destroy(std::shared_ptr<HexrtIO> &raytracingio);

};


#endif //FIELD_DESCRIPTOR_H
