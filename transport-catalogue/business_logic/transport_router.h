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

class TransportRouter {
    friend struct SerializerTransportRouter;
    
public:
    explicit TransportRouter(const TransportCatalogue& catalogue, Domain::RoutingSettings routing_settings);
    /**Установить настройки прохождения маршрута*/
    TransportRouter& SetRoutingSettings(const Domain::RoutingSettings& routing_settings);
    
    Domain::RoutingSettings GetRoutingSettings() const;
    /**Сконструировать связи между остановками*/
    void ConstructRouter();
    
    /**Получить информацию об оптимальном маршруте с пересадками, по указателю на остановку начала и конца маршрута*/
    std::optional<Domain::UserRouteInfo> GetUserRouteInfo(const Domain::Stop* stop_from, const Domain::Stop* stop_to) const;
    /**Получить информацию об оптимальном маршруте с пересадками, по имени остановок начала и конца маршрута*/
    std::optional<Domain::UserRouteInfo> GetUserRouteInfo(std::string_view stop_name_from, std::string_view stop_name_to) const;

private:
    const TransportCatalogue& catalogue_;
    Domain::RoutingSettings routing_settings_;
    graph::DirectedWeightedGraph<Domain::TimeMinuts> graph_;
    std::optional<graph::Router<Domain::TimeMinuts>> router_;
    std::unordered_map<const Domain::Stop*, graph::VertexId> graph_stop_to_vertex_id_catalog_;
    std::unordered_map<graph::EdgeId, Domain::TrackSectionInfo> graph_edge_id_to_info_catalog_;

private:
    explicit TransportRouter(const TransportCatalogue& catalogue);
    
    void InitGraph();
    void AddBusesToGraph();
    void AddTrackSectionToGraph(graph::VertexId from, graph::VertexId to, Domain::TimeMinuts time, size_t span_count, const Domain::RouteEntity& entity);
    
    Domain::UserRouteInfo::RouteItems GetRouteItems(const TransportGuide::graph::Router<Domain::TimeMinuts>::RouteInfo& route_info) const;
};

struct SerializerTransportRouter final {
    SerializerTransportRouter(BusinessLogic::TransportRouter& transport_router);
    ~SerializerTransportRouter() = default;
    
    static TransportRouter ConstructTransportRouter(const TransportCatalogue& catalogue);
    void ConstructGraph();
    Domain::RoutingSettings& GetRoutingSettings();
    std::optional<graph::Router<Domain::TimeMinuts>>& GetRouter();
    graph::DirectedWeightedGraph<Domain::TimeMinuts>& GetGraph();
    std::unordered_map<const Domain::Stop*, graph::VertexId>& GetGraphStopToVertexIdCatalog();
    std::unordered_map<graph::EdgeId, Domain::TrackSectionInfo>& GetGraphEdgeIdToInfoCatalog();

private:
    BusinessLogic::TransportRouter& transport_router_;
};



} // TransportGuide::BusinessLogic
