#include <sstream>
#include <iostream>
#include <utility>
#include <iterator>
#include <numeric>
#include <algorithm>
#include "transport_catalogue.h"
#include "geo.h"

namespace TransportGuide::Core {
using namespace std::literals;

//region detail

constexpr double ACCURACY_COMPARISON = 1e-1;

size_t TrackSectionHasher::operator()(const TrackSection& e) const {
    size_t hash_result = reinterpret_cast<size_t>(e.first) + reinterpret_cast<size_t>(e.second) * sizeof(TrackSection) * 1837;
    return hash_result;
}

//endregion

//region Private section TransportCatalogue

//Получаю дистанцию из каталога, если нужно считаю
double TransportCatalogue::GetCalculatedDistance(TrackSection track_section) const {
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

double TransportCatalogue::GetCalculatedDistance(const Stop* left, const Stop* right) const {
    return TransportCatalogue::GetCalculatedDistance({left, right});
}

//Считаю и добавляю дистанцию в каталог
void TransportCatalogue::AddCalculatedDistanceToCatalog(TrackSection track_section) const {
    double distance = detail::ComputeDistance({track_section.first->latitude, track_section.first->longitude},
            {track_section.second->latitude, track_section.second->longitude});
    calculated_distance_catalog_[track_section] = distance;
    std::swap(track_section.first, track_section.second);
    calculated_distance_catalog_[track_section] = distance;
}

std::optional<double> TransportCatalogue::GetRealDistance(TrackSection track_section) const {
    if (real_distance_catalog_.count(track_section)) {
        return real_distance_catalog_.at(track_section);
    }
    std::swap(track_section.first, track_section.second);
    if (real_distance_catalog_.count(track_section)) {
        return real_distance_catalog_.at(track_section);
    }
    return std::nullopt;
}

std::optional<double> TransportCatalogue::GetRealDistance(const Stop* left, const Stop* right) const {
    return TransportCatalogue::GetRealDistance({left, right});
}

void TransportCatalogue::AddBusInStopBusesCatalog(const Bus* bus) {
    if (bus->route.empty()) { return; }
    std::for_each(bus->route.begin(), std::prev(bus->route.end()), [this, bus](const Stop* stop) {
        stop_buses_catalog_[stop].insert(bus);
    });
}

void TransportCatalogue::EraseBusInStopBusesCatalog(const Bus* bus) {
    if (bus->route.empty()) { return; }
    std::for_each(bus->route.begin(), std::prev(bus->route.end()), [this, bus](const Stop* stop){
        if (stop_buses_catalog_.count(stop)){
            stop_buses_catalog_.at(stop).erase(bus);
        }
    });
}

std::vector<const Bus*> TransportCatalogue::GetBusesByStop(const Stop* stop) const {
    return TransportCatalogue::GetBusesByStop(stop, [](const Bus* lhs, const Bus* rhs) {
        return lhs->name < rhs->name;
    });
}

//endregion

//region Public section TransportCatalogue

Bus* TransportCatalogue::InsertBus(Bus bus) {
    Bus* bus_ptr;
    if (bus_name_catalog_.count(bus.name)) {
        bus_ptr = bus_name_catalog_.at(bus.name);
        EraseBusInStopBusesCatalog(bus_ptr);
        bus_ptr->Update(bus);
        AddBusInStopBusesCatalog(bus_ptr);
    }
    else {
        bus_catalog_.push_back(std::forward<Bus>(bus));
        bus_ptr = &bus_catalog_.back();
        bus_name_catalog_.insert({bus_ptr->name, bus_ptr});
        AddBusInStopBusesCatalog(bus_ptr);
    }
    return bus_ptr;
}

Stop* TransportCatalogue::InsertStop(Stop stop) {
    Stop* stop_ptr;
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

std::optional<const Bus*> TransportCatalogue::FindBus(const std::string_view& name) const {
    if (bus_name_catalog_.count(name)) {
        return bus_name_catalog_.at(name);
    }
    else {
        return std::nullopt;
    }
}

std::optional<const Stop*> TransportCatalogue::FindStop(const std::string_view& name) const {
    if (stop_name_catalog_.count(name)) {
        return stop_name_catalog_.at(name);
    }
    else {
        return std::nullopt;
    }
}

std::optional<BusInfo> TransportCatalogue::GetBusInfo(const Bus* bus) const {
    if (bus && bus_name_catalog_.count(bus->name) && bus_name_catalog_.at(bus->name) == bus) {
        BusInfo bus_info;
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

std::optional<BusInfo> TransportCatalogue::GetBusInfo(const std::string_view& bus_name) const {
    auto bus = FindBus(bus_name);
    if (bus.has_value()) {
        return TransportCatalogue::GetBusInfo(bus.value());
    }
    else {
        return std::nullopt;
    }
}

std::optional<StopInfo> TransportCatalogue::GetStopInfo(const Stop* stop) const {
    if (!stop) { return std::nullopt; }
    if (stop_name_catalog_.count(stop->name) && stop_name_catalog_.at(stop->name) == stop) {
        StopInfo stop_info;
        stop_info.name = stop->name;
        stop_info.buses = GetBusesByStop(stop);
        return stop_info;
    }
    else {
        return std::nullopt;
    }
}

std::optional<StopInfo> TransportCatalogue::GetStopInfo(const std::string_view& stop_name) const {
    auto stop = FindStop(stop_name);
    if (stop.has_value()) {
        return TransportCatalogue::GetStopInfo(stop.value());
    }
    else {
        return std::nullopt;
    }
}

void TransportCatalogue::AddRealDistanceToCatalog(TrackSection track_section, double distance) {
    real_distance_catalog_[track_section] = distance;
}

void TransportCatalogue::AddRealDistanceToCatalog(const Stop* left, const Stop* right, double distance) {
    AddRealDistanceToCatalog({left, right}, distance);
}

double TransportCatalogue::GetBusCalculateLength(const std::vector<const Stop*>& route) const {
    //Применил стандартный алгоритм вместо промежуточного контейнера
    auto compute_route_dist = [this](const Stop* left, const Stop* right) {
        return GetCalculatedDistance({left, right});
    };
    return std::transform_reduce(route.begin(), std::prev(route.end()), std::next(route.begin()), 0., std::plus<>(),
            compute_route_dist);
}

double TransportCatalogue::GetBusRealLength(const std::vector<const Stop*>& route) const {
    //Применил стандартный алгоритм вместо промежуточного контейнера
    auto compute_route_dist = [this](const Stop* left, const Stop* right) {
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

//endregion

//region Stop

bool Stop::operator==(const Stop& rhs) const {
    return name == rhs.name && latitude == rhs.latitude && longitude == rhs.longitude;
}

bool Stop::operator!=(const Stop& rhs) const {
    return !(rhs == *this);
}

Stop::Stop(std::string name) : name(std::move(name)) {}

Stop::Stop(std::string name, double latitude, double longitude) : name(std::move(name)), latitude(latitude),
    longitude(longitude) {
    is_fill = true;
}

Stop::Stop(const Stop& other) {
    name = other.name;
    latitude = other.latitude;
    longitude = other.longitude;
    is_fill = other.is_fill;
}

Stop& Stop::operator=(const Stop& other) {
    if (this != &other)
    {
        Stop stop(other);
        std::swap(name, stop.name);
        std::swap(latitude, stop.latitude);
        std::swap(longitude, stop.longitude);
        std::swap(is_fill, stop.is_fill);
    }
    return *this;
}

bool Stop::Update(const Stop& other) {
    if (name == other.name){
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

