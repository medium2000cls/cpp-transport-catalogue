#pragma once

#include <string>
#include <utility>
#include <vector>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <optional>

namespace TransportGuide::Domain {

struct Stop {
    std::string name;
    double latitude = 0;
    double longitude = 0;
    bool is_fill = false;
    explicit Stop(std::string name);
    explicit Stop(std::string name, double latitude, double longitude);
    ~Stop() = default;
    Stop(const Stop& other);
    Stop& operator=(const Stop& other);
    bool Update(const Stop& other);
    bool operator==(const Stop& rhs) const;
    bool operator!=(const Stop& rhs) const;
};


struct Bus {
    std::string name;
    std::vector<const Stop*> route;
    size_t unique_stops_count;
    //Маршруты теперь хранят длины пути
    double calc_length;
    double real_length;
    explicit Bus(std::string name, const std::vector<const Stop*>& route, double calc_length, double real_length);
    ~Bus() = default;
    Bus(const Bus& other);
    Bus& operator=(const Bus& other);
    bool Update(const Bus& other);
    bool operator==(const Bus& rhs) const;
    bool operator!=(const Bus& rhs) const;
};


using TrackSection = std::pair<const TransportGuide::Domain::Stop*, const TransportGuide::Domain::Stop*>;


struct TrackSectionHasher {
    size_t operator()(const TrackSection& e) const;
};


struct BusInfo {
    std::string_view name;
    size_t stops_count;
    size_t unique_stops_count;
    double length;
    double curvature = 1.;
    bool operator==(const BusInfo& rhs) const;
    bool operator!=(const BusInfo& rhs) const;
};


struct StopInfo {
    std::string_view name;
    std::vector<const Bus*> buses;
    bool operator==(const StopInfo& rhs) const;
    bool operator!=(const StopInfo& rhs) const;
};
}