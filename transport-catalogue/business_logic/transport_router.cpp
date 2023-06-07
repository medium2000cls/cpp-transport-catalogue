#include <numeric>
#include <utility>
#include "transport_router.h"
#include "transport_catalogue.h"

namespace TransportGuide::BusinessLogic {

using namespace std::literals;

TransportRouter::TransportRouter(const TransportCatalogue& catalogue,
        Domain::RoutingSettings routing_settings) : catalogue_(catalogue), routing_settings_(routing_settings) {
    ConstructGraph();
}

TransportRouter& TransportRouter::SetRoutingSettings(const Domain::RoutingSettings& routing_settings) {
    routing_settings_ = routing_settings;
    return *this;
}

void TransportRouter::ConstructGraph() {
    InitGraph();
    AddBusesToGraph();
    router_.emplace(graph_);
}

std::optional<Domain::UserRouteInfo> TransportRouter::GetUserRouteInfo(const Domain::Stop* stop_from,
        const Domain::Stop* stop_to) const {
    graph::VertexId id_from = graph_stop_to_vertex_id_catalog_.at(stop_from);
    graph::VertexId id_to = graph_stop_to_vertex_id_catalog_.at(stop_to);
    auto route_info = router_->BuildRoute(id_from, id_to);
    
    if (route_info.has_value()) {
        Domain::UserRouteInfo::RouteItems items = GetRouteItems(route_info.value());
        //        auto total_time = std::transform_reduce(items.begin(), items.end(), 0., std::plus<>{},
        //                [](const std::variant<Domain::UserRouteInfo::UserWait, Domain::UserRouteInfo::UserBus>& element){
        //                    return std::holds_alternative<Domain::UserRouteInfo::UserWait>(element) ? std::get<Domain::UserRouteInfo::UserWait>(element).time : std::get<Domain::UserRouteInfo::UserBus>(element).time;
        //        });
        return Domain::UserRouteInfo{.total_time = route_info->weight, .items = std::move(items)};
    }
    return std::nullopt;
}

Domain::UserRouteInfo::RouteItems TransportRouter::GetRouteItems(const graph::Router<Domain::TimeMinuts>::RouteInfo& route_info) const {
    Domain::UserRouteInfo::RouteItems items;
    
    for (graph::EdgeId id : route_info.edges) {
        if (graph_edge_id_to_info_catalog_.count(id)) {
            Domain::TrackSectionInfo track_section_info = graph_edge_id_to_info_catalog_.at(id);
            
            if (std::holds_alternative<const Domain::Stop*>(track_section_info.entity)) {
                items.emplace_back(Domain::UserRouteInfo::UserWait{ .stop = std::get<const Domain::Stop*>(track_section_info.entity),
                                                                    .time = track_section_info.time});
            }
            else {
                items.emplace_back(Domain::UserRouteInfo::UserBus{.bus = std::get<const Domain::Bus*>(track_section_info.entity),
                                                                  .span_count = track_section_info.span_count,
                                                                  .time = track_section_info.time});
            }
        }
        else {
            throw std::range_error(
                    "EdgeId "s + std::to_string(id) + " is not count in graph_edge_id_to_info_catalog."s);
        }
    }
    return items;
}

std::optional<Domain::UserRouteInfo> TransportRouter::GetUserRouteInfo(std::string_view stop_name_from,
        std::string_view stop_name_to) const {
    auto stop_from = catalogue_.FindStop(stop_name_from);
    auto stop_to = catalogue_.FindStop(stop_name_to);
    if (stop_from.has_value() && stop_to.has_value()) {
        return TransportRouter::GetUserRouteInfo(stop_from.value(), stop_to.value());
    }
    else {
        return std::nullopt;
    }
}

void TransportRouter::InitGraph() {
    size_t vertex_count = catalogue_.GetStops().size() * 2;
    graph_ = graph::DirectedWeightedGraph<Domain::TimeMinuts>(vertex_count);
    
    for (size_t i = 0; i < vertex_count; i+=2) {
        
        graph::EdgeId id = graph_.AddEdge({i, i + 1, routing_settings_.bus_wait_time});
        const Domain::Stop* stop_ptr = &catalogue_.GetStops().at(i / 2);
        graph_stop_to_vertex_id_catalog_[stop_ptr] = i;
        graph_edge_id_to_info_catalog_[id] = {.time = routing_settings_.bus_wait_time, .span_count = 0, .entity = stop_ptr};
    }
}

void TransportRouter::AddBusesToGraph() {
    static const double MINUTES_PER_HOUR = 60.;
    static const double METERS_PER_KMETERS = 1000.;
    
    for (const Domain::Bus& bus : catalogue_.GetBuses()) {
        std::vector<std::pair<graph::VertexId, Domain::TimeMinuts>> traveled_stops;
        for (auto it = bus.route.begin(), it_end = std::prev(bus.route.end()); it != it_end; std::advance(it, 1)) {
            auto it_next = std::next(it);
            
            graph::VertexId from = graph_stop_to_vertex_id_catalog_.at(*it) + 1;
            graph::VertexId to = graph_stop_to_vertex_id_catalog_.at(*it_next);
            
            //Время движения по секции маршрута
            double track_section_distance = catalogue_.GetDistance({*it, *it_next});
            const Domain::TimeMinuts time_drive = track_section_distance / (routing_settings_.bus_velocity * METERS_PER_KMETERS / MINUTES_PER_HOUR);
            
            AddTrackSectionToGraph(from, to, time_drive, 1,&bus);
            for (size_t i = 0, traveled_stops_size = traveled_stops.size(); i < traveled_stops_size; ++i) {
                auto& [old_from, old_time] = traveled_stops[i];
                old_time += time_drive;
                AddTrackSectionToGraph(old_from, to, old_time, traveled_stops_size + 1 - i ,&bus);
            }
            traveled_stops.emplace_back(from, time_drive);
        }
    }
}

void TransportRouter::AddTrackSectionToGraph(graph::VertexId from, graph::VertexId to, Domain::TimeMinuts time, size_t span_count, const Domain::RouteEntity& entity) {
    graph::EdgeId id = graph_.AddEdge({.from = from, .to = to, .weight = time});
    graph_edge_id_to_info_catalog_.insert({id, {.time =  time, .span_count = span_count, .entity = entity}});
}

Domain::RoutingSettings TransportRouter::GetRoutingSettings() const {
    return routing_settings_;
}


} // TransportGuide::BusinessLogic