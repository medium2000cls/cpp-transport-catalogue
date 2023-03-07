#pragma once

#include <cmath>
namespace TransportGuide::detail {

struct Coordinates {
    double lat;
    double lng;
    
    bool operator==(const Coordinates& other) const;
    
    bool operator!=(const Coordinates& other) const;
};


inline double ComputeDistance(Coordinates from, Coordinates to);

}
