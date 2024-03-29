#pragma once

#include <string>
#include <utility>
#include <vector>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <optional>
#include <variant>

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
    size_t number_final_stop_ = 0;
    explicit Bus(std::string name, const std::vector<const Stop*>& route, double calc_length, double real_length);
    explicit Bus(std::string name, const std::vector<const Stop*>& route, size_t number_final_stop, double calc_length, double real_length);
    ~Bus() = default;
    Bus(const Bus& other);
    Bus& operator=(const Bus& other);
    bool Update(const Bus& other);
    bool IsRoundtrip() const;
    std::vector<const Stop*> GetForwardRoute() const;
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


using PixelDelta = std::pair<double, double>;

struct RenderSettings {
    double width = 600.0;
    double height = 400.0;
    double padding = 50.0;
    double line_width = 14.0;
    double stop_radius = 5.0;
    int32_t bus_label_font_size = 20;
    PixelDelta bus_label_offset = {7.0, 15.0};
    int32_t stop_label_font_size = 20;
    PixelDelta stop_label_offset = {7.0, -3.0};
    std::string underlayer_color = "rgba(255,255,255,0.85)";
    double underlayer_width = 3.0;
    std::vector<std::string> color_palette = {"green", "rgb(255,160,0)", "red"};
};

using TimeMinuts = double;
using RouteEntity = std::variant<const Stop*, const Bus*>;

struct RoutingSettings {
    TimeMinuts bus_wait_time = 0;
    double bus_velocity = 0;
};

struct TrackSectionInfo {
    TimeMinuts time;
    size_t span_count;
    RouteEntity entity;
};

struct UserRouteInfo {
    struct UserWait {
        const Stop* stop;
        TimeMinuts time;
    };
    
    struct UserBus {
        const Bus* bus;
        size_t span_count;
        TimeMinuts time;
    };
    
    using RouteItems = std::vector<std::variant<UserWait, UserBus>>;
    
    TimeMinuts total_time;
    RouteItems items;
};
}