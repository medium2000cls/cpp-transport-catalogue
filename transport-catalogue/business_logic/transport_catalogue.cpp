#include <sstream>
#include <iostream>
#include <utility>
#include <iterator>
#include <numeric>
#include <algorithm>
#include "transport_catalogue.h"
#include "../domain/geo.h"

namespace TransportGuide::BusinessLogic {
using namespace std::literals;

//region Public section TransportCatalogue

Domain::Bus* TransportCatalogue::InsertBus(const Domain::Bus& bus) {
    Domain::Bus* bus_ptr;
    if (bus_name_catalog_.count(bus.name)) {
        bus_ptr = bus_name_catalog_.at(bus.name);
        EraseBusInStopBusesCatalog(bus_ptr);
        bus_ptr->Update(bus);
        AddBusInStopBusesCatalog(bus_ptr);
    }
    else {
        bus_catalog_.push_back(bus);
        bus_ptr = &bus_catalog_.back();
        bus_name_catalog_.insert({bus_ptr->name, bus_ptr});
        AddBusInStopBusesCatalog(bus_ptr);
    }
    return bus_ptr;
}

Domain::Stop* TransportCatalogue::InsertStop(const Domain::Stop& stop) {
    Domain::Stop* stop_ptr;
    if (!stop_name_catalog_.count(stop.name)) {
        stop_catalog_.push_back(stop);
        stop_ptr = &stop_catalog_.back();
        stop_name_catalog_.insert({stop_ptr->name, stop_ptr});
    }
    else if (stop.is_fill) {
        stop_ptr = stop_name_catalog_.at(stop.name);
        stop_ptr->Update(stop);
    }
    else {
        stop_ptr = stop_name_catalog_.at(stop.name);
    }
    return stop_ptr;
}

std::optional<const Domain::Bus*> TransportCatalogue::FindBus(std::string_view name) const {
    if (bus_name_catalog_.count(name)) {
        return bus_name_catalog_.at(name);
    }
    else {
        return std::nullopt;
    }
}

std::optional<const Domain::Stop*> TransportCatalogue::FindStop(std::string_view name) const {
    if (stop_name_catalog_.count(name)) {
        return stop_name_catalog_.at(name);
    }
    else {
        return std::nullopt;
    }
}

std::optional<Domain::BusInfo> TransportCatalogue::GetBusInfo(
        const Domain::Bus* bus) const {
    if (bus && bus_name_catalog_.count(bus->name) && bus_name_catalog_.at(bus->name) == bus) {
        Domain::BusInfo bus_info;
        bus_info.name = bus->name;
        bus_info.stops_count = bus->route.size();
        bus_info.unique_stops_count = bus->unique_stops_count;
        //Вынес вычисление реальной и посчитанной длин пути на этап заполнения каталога.
        //transport-catalogue/input_reader.cpp:105,:106
        //Маршруты теперь хранят эти данные
        //transport-catalogue/transport_catalogue.h:34
        bus_info.length = bus->real_length;
        bus_info.curvature = bus->real_length / bus->calc_length;
        return bus_info;
    }
    return std::nullopt;
}

std::optional<Domain::BusInfo> TransportCatalogue::GetBusInfo(std::string_view bus_name) const {
    auto bus = FindBus(bus_name);
    if (bus.has_value()) {
        return TransportCatalogue::GetBusInfo(bus.value());
    }
    else {
        return std::nullopt;
    }
}

std::optional<Domain::StopInfo> TransportCatalogue::GetStopInfo(
        const Domain::Stop* stop) const {
    if (!stop) { return std::nullopt; }
    if (stop_name_catalog_.count(stop->name) && stop_name_catalog_.at(stop->name) == stop) {
        Domain::StopInfo stop_info;
        stop_info.name = stop->name;
        stop_info.buses = GetBusesByStop(stop);
        return stop_info;
    }
    else {
        return std::nullopt;
    }
}

std::optional<Domain::StopInfo> TransportCatalogue::GetStopInfo(std::string_view stop_name) const {
    auto stop = FindStop(stop_name);
    if (stop.has_value()) {
        return TransportCatalogue::GetStopInfo(stop.value());
    }
    else {
        return std::nullopt;
    }
}

void TransportCatalogue::AddRealDistanceToCatalog(Domain::TrackSection track_section, double distance) {
    real_distance_catalog_[track_section] = distance;
}

void TransportCatalogue::AddRealDistanceToCatalog(const Domain::Stop* left, const Domain::Stop* right,
        double distance) {
    AddRealDistanceToCatalog({left, right}, distance);
}

double TransportCatalogue::GetDistance(Domain::TrackSection track_section) const {
    std::optional<double> dist = GetRealDistance(track_section);
    if (dist.has_value()) {
        return dist.value();
    }
    return GetCalculatedDistance(track_section);
}

double TransportCatalogue::GetDistance(const Domain::Stop* left, const Domain::Stop* right) const {
    return GetDistance({left, right});
}

double TransportCatalogue::GetBusCalculateLength(const std::vector<const Domain::Stop*>& route) const {
    //Применил стандартный алгоритм вместо промежуточного контейнера
    auto compute_route_dist = [this](const Domain::Stop* left,
            const Domain::Stop* right) {
        return GetCalculatedDistance({left, right});
    };
    return std::transform_reduce(route.begin(), std::prev(route.end()), std::next(route.begin()), 0., std::plus<>(),
            compute_route_dist);
}

double TransportCatalogue::GetBusRealLength(const std::vector<const Domain::Stop*>& route) const {
    //Применил стандартный алгоритм вместо промежуточного контейнера
    auto compute_route_dist = [this](const Domain::Stop* left,
            const Domain::Stop* right) {
        std::optional<double> distance_opt = GetRealDistance({left, right});
        if (distance_opt.has_value()) {
            return distance_opt.value();
        }
        else {
            return GetCalculatedDistance({left, right});
        }
    };
    return std::transform_reduce(route.begin(), std::prev(route.end()), std::next(route.begin()), 0., std::plus<>(),
            compute_route_dist);
}

const std::unordered_map<std::string_view, Domain::Bus*>& TransportCatalogue::GetBusNameCatalog() const {
    return bus_name_catalog_;
}

const std::unordered_map<std::string_view, Domain::Stop*>& TransportCatalogue::GetStopNameCatalog() const {
    return stop_name_catalog_;
}

const std::deque<Domain::Stop>& TransportCatalogue::GetStops() const {
    return stop_catalog_;
}

const std::deque<Domain::Bus>& TransportCatalogue::GetBuses() const {
    return bus_catalog_;
}

void TransportCatalogue::ConstructUserRouteManager(Domain::RoutingSettings routing_settings) {
    user_route_manager_.emplace(*this, routing_settings);
}

const UserRouteManager& TransportCatalogue::GetUserRouteManager() const {
    if (!user_route_manager_.has_value()) {
        throw std::logic_error("UserRouteManager is not construct."s);
    }
    return *user_route_manager_;
}

//endregion

//region Private section TransportCatalogue

//Получаю дистанцию из каталога, если нужно считаю
double TransportCatalogue::GetCalculatedDistance(Domain::TrackSection track_section) const {
    if (track_section.first == track_section.second) { return 0.; }
    else if (calculated_distance_catalog_.count(track_section)) {
        return calculated_distance_catalog_[track_section];
    }
    else if (track_section.first->is_fill && track_section.second->is_fill) {
        AddCalculatedDistanceToCatalog(track_section);
        return calculated_distance_catalog_[track_section];
    }
    return 0.;
}

double TransportCatalogue::GetCalculatedDistance(const Domain::Stop* left,
        const Domain::Stop* right) const {
    return TransportCatalogue::GetCalculatedDistance({left, right});
}

//Считаю и добавляю дистанцию в каталог
void TransportCatalogue::AddCalculatedDistanceToCatalog(Domain::TrackSection track_section) const {
    double distance = Domain::geo::ComputeDistance({track_section.first->latitude, track_section.first->longitude},
                                           {track_section.second->latitude, track_section.second->longitude});
    calculated_distance_catalog_[track_section] = distance;
    std::swap(track_section.first, track_section.second);
    calculated_distance_catalog_[track_section] = distance;
}

std::optional<double> TransportCatalogue::GetRealDistance(Domain::TrackSection track_section) const {
    if (real_distance_catalog_.count(track_section)) {
        return real_distance_catalog_.at(track_section);
    }
    std::swap(track_section.first, track_section.second);
    if (real_distance_catalog_.count(track_section)) {
        return real_distance_catalog_.at(track_section);
    }
    return std::nullopt;
}

std::optional<double> TransportCatalogue::GetRealDistance(const Domain::Stop* left, const Domain::Stop* right) const {
    return TransportCatalogue::GetRealDistance({left, right});
}

void TransportCatalogue::AddBusInStopBusesCatalog(const Domain::Bus* bus) {
    if (bus->route.empty()) { return; }
    std::for_each(bus->route.begin(), std::prev(bus->route.end()), [this, bus](const Domain::Stop* stop) {
        stop_buses_catalog_[stop].insert(bus);
    });
}

void TransportCatalogue::EraseBusInStopBusesCatalog(const Domain::Bus* bus) {
    if (bus->route.empty()) { return; }
    std::for_each(bus->route.begin(), std::prev(bus->route.end()),
            [this, bus](const Domain::Stop* stop) {
                if (stop_buses_catalog_.count(stop)) {
                    stop_buses_catalog_.at(stop).erase(bus);
                }
            });
}

std::vector<const Domain::Bus*> TransportCatalogue::GetBusesByStop(
        const Domain::Stop* stop) const {
    return TransportCatalogue::GetBusesByStop(stop,
            [](const Domain::Bus* lhs, const Domain::Bus* rhs) {
                return lhs->name < rhs->name;
            });
}

//endregion

}

