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

namespace TransportGuide::BusinessLogic {

class TransportCatalogue;

class UserRouteManager {
public:
    
    explicit UserRouteManager(const TransportCatalogue& catalogue, Domain::RoutingSettings routing_settings);
    /**Установить настройки прохождения маршрута*/
    UserRouteManager& SetRoutingSettings(const Domain::RoutingSettings& routing_settings);
    /**Сконструировать связи между остановками*/
    void ConstructGraph();
    
    /**Получить информацию об оптимальном маршруте с пересадками, по указателю на остановку начала и конца маршрута*/
    std::optional<Domain::UserRouteInfo> GetUserRouteInfo(const Domain::Stop* stop_from, const Domain::Stop* stop_to) const;
    /**Получить информацию об оптимальном маршруте с пересадками, по имени остановок начала и конца маршрута*/
    std::optional<Domain::UserRouteInfo> GetUserRouteInfo(std::string_view stop_name_from, std::string_view stop_name_to) const;

private:
    const TransportCatalogue& catalogue_;
    Domain::RoutingSettings routing_settings_;
    std::optional<graph::Router<Domain::TimeMinuts>> router_ = std::nullopt;
    graph::DirectedWeightedGraph<Domain::TimeMinuts> graph_;
    std::unordered_map<const Domain::Stop*, std::vector<graph::VertexId>> graph_stop_to_vertex_id_catalog_;
    std::unordered_map<graph::EdgeId, Domain::TrackSectionInfo> graph_edge_id_to_info_catalog_;

private:
    void InitGraph();
    void AddBusesToGraph();
    graph::VertexId AddStopToGraph(const Domain::Stop* stop);
    void AddTrackSectionToGraph(graph::VertexId from, graph::VertexId to, const Domain::TrackSection& track_section, Domain::TimeMinuts time, const Domain::RouteEntity& entity);
    
    Domain::UserRouteInfo::RouteItems GetRouteItems(
            const TransportGuide::graph::Router<Domain::TimeMinuts>::RouteInfo& route_info) const;
    
};

} // TransportGuide::BusinessLogic
