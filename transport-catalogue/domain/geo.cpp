#include "geo.h"

//Вынес радиус земли в константу
static const int AVERAGE_RADIUS_EARTH = 6371000;

bool TransportGuide::Domain::geo::Coordinates::operator==(const Coordinates& other) const {
    return lat == other.lat && lng == other.lng;
}

bool TransportGuide::Domain::geo::Coordinates::operator!=(const Coordinates& other) const {
    return !(*this == other);
}

double TransportGuide::Domain::geo::ComputeDistance(Coordinates from, Coordinates to) {
    using namespace std;
    if (from == to) {
        return 0;
    }
    static const double dr = 3.1415926535 / 180.;
    return acos(sin(from.lat * dr) * sin(to.lat * dr) + cos(from.lat * dr) * cos(to.lat * dr) * cos(
            abs(from.lng - to.lng) * dr)) * AVERAGE_RADIUS_EARTH;
}
