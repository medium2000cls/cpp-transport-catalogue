#include <algorithm>
#include <numeric>
#include <iterator>
#include <utility>
#include <iostream>
#include <sstream>
#include "domain.h"

namespace TransportGuide::Domain {

//region geo

constexpr double ACCURACY_COMPARISON = 1e-1;

size_t TrackSectionHasher::operator()(const TrackSection& e) const {
    size_t hash_result = reinterpret_cast<size_t>(e.first) + reinterpret_cast<size_t>(e.second) * sizeof(TrackSection) * 1837;
    return hash_result;
}

//endregion

//region Stop

bool Stop::operator==(const Stop& rhs) const {
    return name == rhs.name && latitude == rhs.latitude && longitude == rhs.longitude;
}

bool Stop::operator!=(const Stop& rhs) const {
    return !(rhs == *this);
}

Stop::Stop(std::string name) : name(std::move(name)) {}

Stop::Stop(std::string name, double latitude, double longitude) : name(std::move(name)), latitude(latitude), longitude(
        longitude) {
    is_fill = true;
}

Stop::Stop(const Stop& other) {
    name = other.name;
    latitude = other.latitude;
    longitude = other.longitude;
    is_fill = other.is_fill;
}

Stop& Stop::operator=(const Stop& other) {
    if (this != &other) {
        Stop stop(other);
        std::swap(name, stop.name);
        std::swap(latitude, stop.latitude);
        std::swap(longitude, stop.longitude);
        std::swap(is_fill, stop.is_fill);
    }
    return *this;
}

bool Stop::Update(const Stop& other) {
    if (name == other.name) {
        latitude = other.latitude;
        longitude = other.longitude;
        is_fill = other.is_fill;
        return true;
    }
    return false;
}

//endregion

//region Bus

bool Bus::operator==(const Bus& rhs) const {
    return name == rhs.name && route == rhs.route;
}

bool Bus::operator!=(const Bus& rhs) const {
    return !(rhs == *this);
}

Bus::Bus(std::string name, const std::vector<const Stop*>& route, double calc_length, double real_length) : name(
        std::move(name)), route(route), calc_length(calc_length), real_length(real_length) {
    std::unordered_set<const Stop*> unique_stops(route.begin(), route.end());
    unique_stops_count = unique_stops.size();
}

Bus::Bus(const Bus& other) {
    name = other.name;
    route = other.route;
    unique_stops_count = other.unique_stops_count;
    calc_length = other.calc_length;
    real_length = other.real_length;
}

Bus& Bus::operator=(const Bus& other) {
    if (this != &other) {
        Bus bus(other);
        std::swap(name, bus.name);
        std::swap(route, bus.route);
        std::swap(unique_stops_count, bus.unique_stops_count);
        std::swap(calc_length, bus.calc_length);
        std::swap(real_length, bus.real_length);
    }
    return *this;
}

bool Bus::Update(const Bus& other) {
    if (name == other.name) {
        route = other.route;
        unique_stops_count = other.unique_stops_count;
        calc_length = other.calc_length;
        real_length = other.real_length;
        return true;
    }
    return false;
    
}

//endregion

//region BusInfo

bool BusInfo::operator==(const BusInfo& rhs) const {
    return name == rhs.name && stops_count == rhs.stops_count && unique_stops_count == rhs.unique_stops_count && std::abs(
            length - rhs.length) < ACCURACY_COMPARISON && std::abs(curvature - rhs.curvature) < ACCURACY_COMPARISON;
}

bool BusInfo::operator!=(const BusInfo& rhs) const {
    return !(rhs == *this);
}

//endregion

//region StopInfo

bool StopInfo::operator==(const StopInfo& rhs) const {
    return name == rhs.name && buses == rhs.buses;
}

bool StopInfo::operator!=(const StopInfo& rhs) const {
    return !(rhs == *this);
}

//endregion

}