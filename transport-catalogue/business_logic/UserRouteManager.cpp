#include <numeric>
#include "UserRouteManager.h"
#include "transport_catalogue.h"

namespace TransportGuide::BusinessLogic {

using namespace std::literals;

UserRouteManager::UserRouteManager(const TransportCatalogue& catalogue,
        Domain::RoutingSettings routing_settings) : catalogue_(catalogue), routing_settings_(routing_settings) {
    ConstructGraph();
}

UserRouteManager& UserRouteManager::SetRoutingSettings(const Domain::RoutingSettings& routing_settings) {
    routing_settings_ = routing_settings;
    return *this;
}

void UserRouteManager::ConstructGraph() {
    InitGraph();
    AddBusesToGraph();
    router_.emplace(graph_);
}

std::optional<Domain::UserRouteInfo> UserRouteManager::GetUserRouteInfo(const Domain::Stop* stop_from,
        const Domain::Stop* stop_to) const {
    graph::VertexId id_from = graph_stop_to_vertex_id_catalog_.at(stop_from).front();
    graph::VertexId id_to = graph_stop_to_vertex_id_catalog_.at(stop_to).front();
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

Domain::UserRouteInfo::RouteItems UserRouteManager::GetRouteItems(const graph::Router<Domain::TimeMinuts>::RouteInfo& route_info) const {
    Domain::UserRouteInfo::RouteItems items;
    
    std::optional<Domain::TrackSectionInfo> track_section_info_prev;
    
    for (graph::EdgeId id : route_info.edges) {
        if (graph_edge_id_to_info_catalog_.count(id)) {
            Domain::TrackSectionInfo track_section_info = graph_edge_id_to_info_catalog_.at(id);
            
            if (!track_section_info_prev.has_value() || std::holds_alternative<const Domain::Stop*>(track_section_info.entity)) {
                items.emplace_back(Domain::UserRouteInfo::UserWait{ .stop = std::get<const Domain::Stop*>(track_section_info.entity),
                                                                    .time = track_section_info.time});
            }
            else if (std::holds_alternative<const Domain::Bus*>(track_section_info_prev->entity) &&
                    std::get<const Domain::Bus*>(track_section_info_prev->entity) == std::get<const Domain::Bus*>(track_section_info.entity)) {
                auto& user_bus = std::get<Domain::UserRouteInfo::UserBus>(items.back());
                user_bus.span_count += 1;
                user_bus.time += track_section_info.time;
            }
            else {
                items.emplace_back(Domain::UserRouteInfo::UserBus{.bus = std::get<const Domain::Bus*>(track_section_info.entity),
                                                                  .span_count = 1,
                                                                  .time = track_section_info.time});
            }
            
            track_section_info_prev = track_section_info;
        }
        else {
            throw std::range_error(
                    "EdgeId "s + std::to_string(id) + " is not count in graph_edge_id_to_info_catalog."s);
        }
    }
    return items;
}

std::optional<Domain::UserRouteInfo> UserRouteManager::GetUserRouteInfo(std::string_view stop_name_from,
        std::string_view stop_name_to) const {
    auto stop_from = catalogue_.FindStop(stop_name_from);
    auto stop_to = catalogue_.FindStop(stop_name_to);
    if (stop_from.has_value() && stop_to.has_value()) {
        return UserRouteManager::GetUserRouteInfo(stop_from.value(), stop_to.value());
    }
    else {
        return std::nullopt;
    }
}

void UserRouteManager::InitGraph() {
    for (const Domain::Stop& stop : catalogue_.GetStops()) {
        AddStopToGraph(&stop);
    }
}

void UserRouteManager::AddBusesToGraph() {
    for (const Domain::Bus& bus : catalogue_.GetBuses()) {
        std::optional<graph::VertexId> prev_imaginary;
        Domain::TimeMinuts prev_time_drive;
        
        for (auto it = bus.route.begin(), it_end = std::prev(bus.route.end()); it != it_end; std::advance(it, 1)) {
            auto it_next = std::next(it);
            const Domain::TrackSection track_section {*it, *it_next};
            //Время движения по секции маршрута
            double track_section_distance = catalogue_.GetDistance(track_section);
            static const double MINUTES_PER_HOUR = 60.;
            static const double METERS_PER_KMETERS = 1000.;
            const Domain::TimeMinuts time_drive = track_section_distance / (routing_settings_.bus_velocity * METERS_PER_KMETERS / MINUTES_PER_HOUR);
            
            graph::VertexId from = graph_stop_to_vertex_id_catalog_.at(*it).front();
            graph::VertexId imaginary = AddStopToGraph(*it); //добавляем мнимую остановку
            AddTrackSectionToGraph(from, imaginary, track_section, routing_settings_.bus_wait_time, *it);
            
            graph::VertexId to = graph_stop_to_vertex_id_catalog_.at(*it_next).front();
            AddTrackSectionToGraph(imaginary, to, track_section, time_drive, &bus);
            
            if (prev_imaginary) { AddTrackSectionToGraph(*prev_imaginary, imaginary, track_section, prev_time_drive, &bus); }
            
            prev_imaginary = imaginary;
            prev_time_drive = time_drive;
        }
    }
}

graph::VertexId UserRouteManager::AddStopToGraph(const Domain::Stop* stop) {
    graph::VertexId id = graph_.AddVertex();
    graph_stop_to_vertex_id_catalog_[stop].push_back(id);
    return id;
}

void UserRouteManager::AddTrackSectionToGraph(graph::VertexId from, graph::VertexId to, const Domain::TrackSection& track_section, Domain::TimeMinuts time, const Domain::RouteEntity& entity) {
    graph::EdgeId id = graph_.AddEdge({.from = from, .to = to, .weight = time});
    graph_edge_id_to_info_catalog_.insert({id, {.track_section = track_section, .time =  time, .entity = entity}});
}

} // TransportGuide::BusinessLogic