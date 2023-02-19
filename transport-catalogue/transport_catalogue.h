#pragma once

#include <string>
#include <utility>
#include <vector>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <optional>

namespace TransportGuide::Core {

struct Stop {
    std::string name;
    double latitude = 0;
    double longitude = 0;
    bool is_fill = false;
    explicit Stop(std::string  name);
    explicit Stop(std::string  name, double latitude, double longitude);
    ~Stop() = default;
    Stop(const Stop& other);
    Stop& operator= (const Stop& other);
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
    explicit Bus(std::string  name, const std::vector<const Stop*>& route, double calc_length, double real_length);
    ~Bus() = default;
    Bus(const Bus& other);
    Bus& operator= (const Bus& other);
    bool Update(const Bus& other);
    bool operator==(const Bus& rhs) const;
    bool operator!=(const Bus& rhs) const;
};

using TrackSection = std::pair<const Stop*, const Stop*>;

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

class TransportCatalogue {
    std::unordered_map<std::string_view, Stop*> stop_name_catalog_;
    mutable std::unordered_map<TrackSection, double, TrackSectionHasher> calculated_distance_catalog_;
    std::unordered_map<TrackSection, double, TrackSectionHasher> real_distance_catalog_;
    std::unordered_map<std::string_view, Bus*> bus_name_catalog_;
    std::unordered_map<const Stop*, std::unordered_set<const Bus*>> stop_buses_catalog_;
    
    double GetCalculatedDistance(TrackSection track_section) const;
    double GetCalculatedDistance(const Stop* left, const Stop* right) const;
    void AddCalculatedDistanceToCatalog(TrackSection track_section) const;
    
    std::optional<double> GetRealDistance(TrackSection track_section) const;
    std::optional<double> GetRealDistance(const Stop* left, const Stop* right) const;
    
    void AddBusInStopBusesCatalog(const Bus* bus);
    void EraseBusInStopBusesCatalog(const Bus* bus);
    
    std::vector<const Bus*> GetBusesByStop(const Stop* stop) const;
    
    template<typename Comparator>
    std::vector<const Bus*> GetBusesByStop(const Stop* stop, Comparator comparator) const {
        std::vector<const Bus*> buses;
        if (stop_buses_catalog_.count(stop)) {
            const auto& buses_catalog = stop_buses_catalog_.at(stop);
            buses = std::move(std::vector<const Bus*>(buses_catalog.begin(), buses_catalog.end()));
            std::sort(buses.begin(), buses.end(), comparator);
        }
        return buses;
    }
    
protected:
    std::deque<Bus> bus_catalog_;
    std::deque<Stop> stop_catalog_;
    
public:
    TransportCatalogue() = default;
    Bus* InsertBus(Bus bus);
    Stop* InsertStop(Stop stop);
    std::optional<const Bus*> FindBus(const std::string_view& name) const;
    std::optional<const Stop*> FindStop(const std::string_view& name) const;
    std::optional<BusInfo> GetBusInfo(const Bus* bus) const;
    std::optional<BusInfo> GetBusInfo(const std::string_view& bus_name) const;
    std::optional<StopInfo> GetStopInfo(const Stop* stop) const;
    std::optional<StopInfo> GetStopInfo(const std::string_view& stop_name) const;
    void AddRealDistanceToCatalog(TrackSection track_section, double distance);
    void AddRealDistanceToCatalog(const Stop* left, const Stop* right, double distance);
    double GetBusCalculateLength(const std::vector<const Stop*>& route) const;
    double GetBusRealLength(const std::vector<const Stop*>& route) const;
    
};
}