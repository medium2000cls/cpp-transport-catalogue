#pragma once

#include <string>
#include <utility>
#include <vector>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <optional>
#include "../domain/domain.h"

namespace TransportGuide::BusinessLogic {

class TransportCatalogue {
public:
    TransportCatalogue() = default;
    Domain::Bus* InsertBus(const Domain::Bus& bus);
    Domain::Stop* InsertStop(const Domain::Stop& stop);
    std::optional<const Domain::Bus*> FindBus(const std::string_view& name) const;
    std::optional<const Domain::Stop*> FindStop(const std::string_view& name) const;
    std::optional<Domain::BusInfo> GetBusInfo(const Domain::Bus* bus) const;
    std::optional<Domain::BusInfo> GetBusInfo(const std::string_view& bus_name) const;
    std::optional<Domain::StopInfo> GetStopInfo(const Domain::Stop* stop) const;
    std::optional<Domain::StopInfo> GetStopInfo(const std::string_view& stop_name) const;
    void AddRealDistanceToCatalog(Domain::TrackSection track_section, double distance);
    void AddRealDistanceToCatalog(const Domain::Stop* left, const Domain::Stop* right, double distance);
    double GetBusCalculateLength(const std::vector<const Domain::Stop*>& route) const;
    double GetBusRealLength(const std::vector<const Domain::Stop*>& route) const;
    
protected:
    std::deque<Domain::Bus> bus_catalog_;
    std::deque<Domain::Stop> stop_catalog_;

private:
    std::unordered_map<std::string_view, Domain::Stop*> stop_name_catalog_;
    mutable std::unordered_map<Domain::TrackSection, double, Domain::TrackSectionHasher> calculated_distance_catalog_;
    std::unordered_map<Domain::TrackSection, double, Domain::TrackSectionHasher> real_distance_catalog_;
    std::unordered_map<std::string_view, Domain::Bus*> bus_name_catalog_;
    std::unordered_map<const Domain::Stop*, std::unordered_set<const Domain::Bus*>> stop_buses_catalog_;
    
    double GetCalculatedDistance(Domain::TrackSection track_section) const;
    double GetCalculatedDistance(const Domain::Stop* left, const Domain::Stop* right) const;
    void AddCalculatedDistanceToCatalog(Domain::TrackSection track_section) const;
    
    std::optional<double> GetRealDistance(Domain::TrackSection track_section) const;
    std::optional<double> GetRealDistance(const Domain::Stop* left, const Domain::Stop* right) const;
    
    void AddBusInStopBusesCatalog(const Domain::Bus* bus);
    void EraseBusInStopBusesCatalog(const Domain::Bus* bus);
    
    std::vector<const Domain::Bus*> GetBusesByStop(const Domain::Stop* stop) const;
    
    template<typename Comparator>
    std::vector<const Domain::Bus*> GetBusesByStop(const Domain::Stop* stop, Comparator comparator) const {
        std::vector<const Domain::Bus*> buses;
        if (stop_buses_catalog_.count(stop)) {
            const auto& buses_catalog = stop_buses_catalog_.at(stop);
            buses = std::move(std::vector<const Domain::Bus*>(buses_catalog.begin(), buses_catalog.end()));
            std::sort(buses.begin(), buses.end(), comparator);
        }
        return buses;
    }
};

}