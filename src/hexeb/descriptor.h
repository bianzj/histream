//
// Created by admin on 2024/1/25.
//

#ifndef FIELD_DESCRIPTOR_H
#define FIELD_DESCRIPTOR_H

#include "hexebio.h"

class Descriptor {
public:

    enum VoxelebbindingInd
    {
        spectral,  // for the specific band
        fixedSpectral, // for the all band;
        thermal,  // for the voxel with constant temperature;
        tempe, // voxel temperatures
        canopy,
        meshLink,
        instanceLink,
        voxelLink,
        nano,
        tlas,
        sensor,
        wave,
        light,
        atomcond,
        dir,
        rads,
        netRad,
        pnet,
        storage,
        meteo,
        aerocond,
        raa,
        leafBio,
        soilSet,
        rss,
        air,
        flux,
        tLast,
        state,
        lad,
        waterSet,
        hex, // 异质性体元参数 (Ax, Ay, Az, rho)
    };


    bool createDescriptor(std::shared_ptr<HexebIO> &raytracingio);
    void destroy(std::shared_ptr<HexebIO> &raytracingio);

};


#endif //FIELD_DESCRIPTOR_H
