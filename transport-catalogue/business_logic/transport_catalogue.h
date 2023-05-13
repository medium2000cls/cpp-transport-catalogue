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
#include "../external/graph.h"
#include "../external/router.h"
#include "UserRouteManager.h"

namespace TransportGuide::BusinessLogic {

class UserRouteManager;

class TransportCatalogue {
public:
    TransportCatalogue() = default;
    /**Вставить маршрут, если маршрут с таким именем есть, то обновить данные*/
    Domain::Bus* InsertBus(const Domain::Bus& bus);
    /**Вставить остановку, если остановка с таким именем есть, то обновить данные*/
    Domain::Stop* InsertStop(const Domain::Stop& stop);
    /**Найти маршрут по имени, если маршрута нет, возвращается nullopt*/
    std::optional<const Domain::Bus*> FindBus(std::string_view name) const;
    /**Найти остановку по имени, если остановки нет, возвращается nullopt*/
    std::optional<const Domain::Stop*> FindStop(std::string_view name) const;
    /**Получить информацию о маршруте, по указателю на маршрут*/
    std::optional<Domain::BusInfo> GetBusInfo(const Domain::Bus* bus) const;
    /**Получить информацию о маршруте, по имени маршрута*/
    std::optional<Domain::BusInfo> GetBusInfo(std::string_view bus_name) const;
    /**Получить информацию об остановке, по указателю на остановку*/
    std::optional<Domain::StopInfo> GetStopInfo(const Domain::Stop* stop) const;
    /**Получить информацию об остановке, по имени остановки*/
    std::optional<Domain::StopInfo> GetStopInfo(std::string_view stop_name) const;
    /**Добавить в каталог реальное расстояние между остановками*/
    void AddRealDistanceToCatalog(Domain::TrackSection track_section, double distance);
    /**Добавить в каталог реальное расстояние между остановками*/
    void AddRealDistanceToCatalog(const Domain::Stop* left, const Domain::Stop* right, double distance);
    
    double GetDistance(Domain::TrackSection track_section) const;
    
    double GetDistance(const Domain::Stop* left, const Domain::Stop* right) const;
    
    /**Получить посчитанное расстояние между списком остановок*/
    double GetBusCalculateLength(const std::vector<const Domain::Stop*>& route) const;
    /**Получить реальное расстояние между списком остановок, если реального расстояния нет, вместо него используется посчитанное*/
    double GetBusRealLength(const std::vector<const Domain::Stop*>& route) const;
    /**Получить словарь маршрутов, с ключом по имени*/
    const std::unordered_map<std::string_view, Domain::Bus*>& GetBusNameCatalog() const;
    /**Получить словарь остановок, с ключом по имени*/
    const std::unordered_map<std::string_view, Domain::Stop*>& GetStopNameCatalog() const;

    const std::deque<Domain::Stop>& GetStops() const;

    const std::deque<Domain::Bus>& GetBuses() const;
    
    void ConstructUserRouteManager(Domain::RoutingSettings routing_settings);
    
    const UserRouteManager& GetUserRouteManager() const;

private:
    std::deque<Domain::Bus> bus_catalog_;
    std::deque<Domain::Stop> stop_catalog_;
    std::unordered_map<std::string_view, Domain::Stop*> stop_name_catalog_;
    mutable std::unordered_map<Domain::TrackSection, double, Domain::TrackSectionHasher> calculated_distance_catalog_;
    std::unordered_map<Domain::TrackSection, double, Domain::TrackSectionHasher> real_distance_catalog_;
    std::unordered_map<std::string_view, Domain::Bus*> bus_name_catalog_;
    std::unordered_map<const Domain::Stop*, std::unordered_set<const Domain::Bus*>> stop_buses_catalog_;
    std::optional<UserRouteManager> user_route_manager_;
    
private:
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
            buses = std::vector<const Domain::Bus*>(buses_catalog.begin(), buses_catalog.end());
            std::sort(buses.begin(), buses.end(), comparator);
        }
        return buses;
    }
    
};


}