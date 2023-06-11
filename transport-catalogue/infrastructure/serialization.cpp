#include <transport_catalogue.pb.h>
#include "serialization.h"

TransportGuide::IoRequests::ProtoSerialization::ProtoSerialization(
        BusinessLogic::TransportCatalogue& transport_catalogue,
        renderer::MapRenderer& map_renderer) : transport_catalogue_(transport_catalogue), map_renderer_(map_renderer) {}

void TransportGuide::IoRequests::ProtoSerialization::Serialize([[maybe_unused]]std::ostream& output) {
    Serialization::TransportCatalogue result_catalogue;
    
    BusinessLogic::SerializerTransportCatalogue serializer_catalogue(transport_catalogue_);
    // Сериализация каталога остановок
    for (const auto& stop : serializer_catalogue.GetStopCatalog()) {
        Serialization::Stop ser_stop;
        ser_stop.set_id(reinterpret_cast<uint64_t>(&stop));
        ser_stop.set_name(stop.name);
        ser_stop.set_latitude(stop.latitude);
        ser_stop.set_longitude(stop.longitude);
        ser_stop.set_is_fill(stop.is_fill);
        result_catalogue.add_stops()->CopyFrom(ser_stop);
    }
    // Сериализация каталога маршрутов
    for (const auto& bus : serializer_catalogue.GetBusCatalog()) {
        Serialization::Bus ser_bus;
        ser_bus.set_id(reinterpret_cast<uint64_t>(&bus));
        ser_bus.set_name(bus.name);
        for (const auto& stop_ptr : bus.route) {
            auto ser_stop_id = reinterpret_cast<uint64_t>(stop_ptr);
            ser_bus.add_route(ser_stop_id);
        }
        ser_bus.set_unique_stops_count(bus.unique_stops_count);
        ser_bus.set_calc_length(bus.calc_length);
        ser_bus.set_real_length(bus.real_length);
        ser_bus.set_number_final_stop_(bus.number_final_stop_);
        result_catalogue.add_buses()->CopyFrom(ser_bus);
    }
    // Сериализация каталога посчитанных расстояний
    for (const auto& [sector, distance] : serializer_catalogue.GetCalculatedDistanceCatalog()) {
        Serialization::CalculatedDistance ser_sector;
        ser_sector.set_from_stop_id(reinterpret_cast<uint64_t>(sector.first));
        ser_sector.set_to_stop_id(reinterpret_cast<uint64_t>(sector.second));
        ser_sector.set_distance(distance);
        result_catalogue.add_calculated_distance_catalog()->CopyFrom(ser_sector);
    }
    // Сериализация каталога реальных расстояний
    for (const auto& [sector, distance] : serializer_catalogue.GetRealDistanceCatalog()) {
        Serialization::RealDistance ser_sector;
        ser_sector.set_from_stop_id(reinterpret_cast<uint64_t>(sector.first));
        ser_sector.set_to_stop_id(reinterpret_cast<uint64_t>(sector.second));
        ser_sector.set_distance(distance);
        result_catalogue.add_real_distance_catalog()->CopyFrom(ser_sector);
    }
    // Сериализация настроек рендера
    {
        Serialization::RenderSettings ser_render_settings;
        const Domain::RenderSettings render_settings = map_renderer_.GetRenderSettings();
        ser_render_settings.set_width(render_settings.width);
        ser_render_settings.set_height(render_settings.height);
        ser_render_settings.set_padding(render_settings.padding);
        ser_render_settings.set_line_width( render_settings.line_width);
        ser_render_settings.set_stop_radius(render_settings.stop_radius);
        ser_render_settings.set_bus_label_font_size(render_settings.bus_label_font_size);
        ser_render_settings.mutable_bus_label_offset()->set_x(render_settings.bus_label_offset.first);
        ser_render_settings.mutable_bus_label_offset()->set_y(render_settings.bus_label_offset.second);
        ser_render_settings.set_stop_label_font_size(render_settings.stop_label_font_size);
        ser_render_settings.mutable_stop_label_offset()->set_x(render_settings.stop_label_offset.first);
        ser_render_settings.mutable_stop_label_offset()->set_y(render_settings.stop_label_offset.second);
        ser_render_settings.set_underlayer_color(render_settings.underlayer_color);
        ser_render_settings.set_underlayer_width(render_settings.underlayer_width);
        for (const auto& color : render_settings.color_palette) {
            ser_render_settings.add_color_palette(color);
        }
        result_catalogue.mutable_render_settings()->CopyFrom(ser_render_settings);
    }
    
    //Если есть UserRouteManager
    if (serializer_catalogue.GetUserRouteManager().has_value()) {
        Serialization::TransportRouter result_user_route_manager;
        BusinessLogic::SerializerTransportRouter serializer_transport_router(
                serializer_catalogue.GetUserRouteManager().value());
        
        //Сериализуем настройки маршрутов
        {
            Serialization::RoutingSettings ser_router;
            Domain::RoutingSettings routing_settings = serializer_transport_router.GetRoutingSettings();
            ser_router.set_bus_velocity(routing_settings.bus_velocity);
            ser_router.set_bus_wait_time(routing_settings.bus_wait_time);
            result_user_route_manager.mutable_routing_settings()->CopyFrom(ser_router);
        }
        //Сериализуем граф
        {
            Serialization::Graph ser_graph;
            TransportGuide::graph::DirectedWeightedGraph<double>::SerializerDirectedWeightedGraph serializer_graph(
                    serializer_transport_router.GetGraph());
            for (const auto& edge : serializer_graph.GetEdges()) {
                Serialization::Edge ser_edge;
                ser_edge.set_from(reinterpret_cast<uint64_t>(edge.from));
                ser_edge.set_to(reinterpret_cast<uint64_t>(edge.to));
                ser_edge.set_weight(edge.weight);
                ser_graph.add_edges()->CopyFrom(ser_edge);
            }
            for (const auto& incidence_list : serializer_graph.GetIncidenceLists()) {
                Serialization::IncidenceList ser_incidence_list;
                for (const auto& edge_id : incidence_list) {
                    ser_incidence_list.add_edge_id(edge_id);
                }
                ser_graph.add_incidence_lists()->CopyFrom(ser_incidence_list);
            }
            result_user_route_manager.mutable_graph()->CopyFrom(ser_graph);
        }
        //Если есть Router серриализуем роутер (отключено в связи с требованиями тренажера)
/*
        if (serializer_transport_router.GetRouter().has_value()) {
            Serialization::Router ser_router;
            TransportGuide::graph::Router<double>::SerializerRouter serializer_router(
                    serializer_transport_router.GetRouter().value());
            for (const auto& items : serializer_router.GetRoutesInternalData()) {
                Serialization::RepeatedRouteInternalData ser_repeated_route_internal_data;
                for (const auto& route_internal_data : items) {
                    Serialization::OptionalRouteInternalData ser_optional_route_internal_data;
                    if (route_internal_data.has_value()) {
                        ser_optional_route_internal_data.mutable_route_internal_data()->set_weight(route_internal_data->weight);
                        if (route_internal_data->prev_edge.has_value()) {
                            ser_optional_route_internal_data.mutable_route_internal_data()->mutable_optional_prev_edge()->set_prev_edge(route_internal_data->prev_edge.value());
                        }
                    }
                    ser_repeated_route_internal_data.add_route_internal_data()->CopyFrom(ser_optional_route_internal_data);
                }
                ser_router.add_repeated_route_internal_data()->CopyFrom(ser_repeated_route_internal_data);
            }
            result_user_route_manager.mutable_router()->CopyFrom(ser_router);
        }
*/
        // Сериализуем карту остановок и их ID (graph_stop_to_vertex_id_catalog)
        {
            for (const auto& [stop_id, vertex_id] : serializer_transport_router.GetGraphStopToVertexIdCatalog()) {
                result_user_route_manager.mutable_graph_stop_to_vertex_id_catalog()
                                         ->insert({reinterpret_cast<uint64_t>(stop_id), vertex_id});
            }
            
        }
        // Сериализуем карту ребер и Информацию о них (graph_edge_id_to_info_catalog)
        {
            for (const auto& [edge_id, track_section_info] : serializer_transport_router.GetGraphEdgeIdToInfoCatalog()) {
                Serialization::TrackSectionInfo ser_track_section_info;
                ser_track_section_info.set_time(track_section_info.time);
                ser_track_section_info.set_span_count(track_section_info.span_count);
                //сериализуем один из вариантов
                if (std::holds_alternative<const Domain::Stop*>(track_section_info.entity)) {
                    ser_track_section_info.set_stop_id(
                            reinterpret_cast<uint64_t>(std::get<const Domain::Stop*>(track_section_info.entity)));
                } else if (std::holds_alternative<const Domain::Bus*>(track_section_info.entity)) {
                    ser_track_section_info.set_bus_id(
                            reinterpret_cast<uint64_t>(std::get<const Domain::Bus*>(track_section_info.entity)));
                }
                
                result_user_route_manager.mutable_graph_edge_id_to_info_catalog_()
                                         ->insert({edge_id, ser_track_section_info});
            }
        }
        result_catalogue.mutable_user_route_manager()->CopyFrom(result_user_route_manager);
    }
    
    result_catalogue.SerializeToOstream(&output);
}

void TransportGuide::IoRequests::ProtoSerialization::Deserialize([[maybe_unused]]std::istream& input) {
    Serialization::TransportCatalogue parsed_catalog;
    
    BusinessLogic::SerializerTransportCatalogue serializer_catalogue(transport_catalogue_);
    
    std::map<uint64_t, const Domain::Stop*> temp_stops_catalog;
    std::map<uint64_t, const Domain::Bus*> temp_buses_catalog;
    
    parsed_catalog.ParseFromIstream(&input);
    
    // Заполняем каталог остановок
    for (const auto& stop : parsed_catalog.stops()) {
        Domain::Stop* s_ptr = &serializer_catalogue.GetStopCatalog()
                                                   .emplace_back(stop.name(), stop.latitude(), stop.longitude());
        //создаем каталог имен и ссылок на остановки
        serializer_catalogue.GetStopNameCatalog().emplace(s_ptr->name, s_ptr);
        temp_stops_catalog.emplace(stop.id(), s_ptr);
        
    }
    //Заполняем каталог маршрутов
    for (const auto& bus : parsed_catalog.buses()) {
        std::vector<const Domain::Stop*> route;
        for (const auto& stop_id : bus.route()) {
            route.emplace_back(temp_stops_catalog.at(stop_id));
        }
        // создаем маршрут
        Domain::Bus* b_ptr = &serializer_catalogue.GetBusCatalog()
                                                  .emplace_back(bus.name(), route, bus.number_final_stop_(),
                                                          bus.calc_length(), bus.real_length());
        
        //создаем список маршрутов у остановок
        for (const Domain::Stop* stop_ptr : route) {
            serializer_catalogue.GetStopBusesCatalog()[stop_ptr].emplace(b_ptr);
        }
        //создаем каталог имен и ссылок на маршруты
        serializer_catalogue.GetBusNameCatalog().emplace(b_ptr->name, b_ptr);
        
        temp_buses_catalog.emplace(bus.id(), b_ptr);
    }
    //Заполняем каталог посчитанных расстояний
    for (const auto& entity : parsed_catalog.calculated_distance_catalog()) {
        serializer_catalogue.GetCalculatedDistanceCatalog()
                            .emplace(std::make_pair(temp_stops_catalog.at(entity.from_stop_id()),
                                     temp_stops_catalog.at(entity.to_stop_id())), entity.distance());
    }
    //Заполняем каталог реальных расстояний
    for (const auto& entity : parsed_catalog.real_distance_catalog()) {
        serializer_catalogue.GetRealDistanceCatalog()
                            .emplace(std::make_pair(temp_stops_catalog.at(entity.from_stop_id()),
                                     temp_stops_catalog.at(entity.to_stop_id())), entity.distance());
    }
    
    //Если есть настройки маршрутов, зполняем менеджер маршрутов
    if (parsed_catalog.has_user_route_manager()) {
        const Serialization::TransportRouter& parsed_user_route_manager = parsed_catalog.user_route_manager();
        
        {
            BusinessLogic::TransportRouter transport_router = BusinessLogic::SerializerTransportRouter::ConstructTransportRouter(transport_catalogue_);
            serializer_catalogue.GetUserRouteManager().emplace(std::move(transport_router));
        }
        BusinessLogic::SerializerTransportRouter serializer_transport_router (*serializer_catalogue.GetUserRouteManager());
        
        {
            graph::DirectedWeightedGraph<Domain::TimeMinuts> graph;
            serializer_transport_router.GetGraph() = std::move(graph);
        }
        graph::DirectedWeightedGraph<Domain::TimeMinuts>::SerializerDirectedWeightedGraph serializer_graph(serializer_transport_router.GetGraph());
        
        using SerializerRouter = graph::Router<Domain::TimeMinuts>::SerializerRouter;
        {
            graph::Router<Domain::TimeMinuts> router = SerializerRouter::Construct(serializer_transport_router.GetGraph(), {});
            serializer_transport_router.GetRouter().emplace(std::move(router));
        }
        SerializerRouter serializer_router (*serializer_transport_router.GetRouter());
        
        //заполняем настройки маршрутов
        {
            serializer_transport_router.GetRoutingSettings().bus_wait_time = parsed_user_route_manager.routing_settings().bus_wait_time();
            serializer_transport_router.GetRoutingSettings().bus_velocity = parsed_user_route_manager.routing_settings().bus_velocity();
        }
        //Заполняем граф
        {
            for (const auto& edge : parsed_user_route_manager.graph().edges()) {
                serializer_graph.GetEdges().push_back({edge.from(), edge.to(), edge.weight()});
            }
            for (const auto& parsed_incidence_list : parsed_user_route_manager.graph().incidence_lists()) {
                std::vector<graph::EdgeId> incidence_list_edges(parsed_incidence_list.edge_id().begin(),parsed_incidence_list.edge_id().end());
                serializer_graph.GetIncidenceLists().push_back(std::move(incidence_list_edges));
            }
        }
        //Заполняем маршрутирезатор (Router), после заполнения графа (в зависимости от настроек сериализации роутер может заполняться или рассчитываться)
        //Заполняем
        if (parsed_user_route_manager.has_router()) {
            SerializerRouter::RoutesInternalData& routes_internal_data = serializer_router.GetRoutesInternalData();
            for (const auto& parsed_rep_r_inter_data : parsed_user_route_manager.router().repeated_route_internal_data()) {
                std::vector<std::optional<SerializerRouter::RouteInternalData>> route_internal_data_list;
                for(const auto& parsed_opt_r_inter_data : parsed_rep_r_inter_data.route_internal_data()) {
                    if (parsed_opt_r_inter_data.has_route_internal_data()) {
                        SerializerRouter::RouteInternalData r_inter_data;
                        r_inter_data.weight = parsed_opt_r_inter_data.route_internal_data().weight();
                        if (parsed_opt_r_inter_data.route_internal_data().has_optional_prev_edge()){
                            r_inter_data.prev_edge = parsed_opt_r_inter_data.route_internal_data().optional_prev_edge().prev_edge();
                        }
                        route_internal_data_list.emplace_back(r_inter_data);
                    }
                    else {
                        route_internal_data_list.emplace_back(std::nullopt);
                    }
                }
                routes_internal_data.push_back(std::move(route_internal_data_list));
            }
        }
        //Рассчитываем в конструкторе
        else {
            serializer_transport_router.GetRouter().emplace(serializer_transport_router.GetGraph());
        }
        //Заполняем каталог вертексов по остановке
        {
            for(const auto& [stop_id,vertex_id] : parsed_user_route_manager.graph_stop_to_vertex_id_catalog()){
                serializer_transport_router.GetGraphStopToVertexIdCatalog().emplace(temp_stops_catalog.at(stop_id),vertex_id);
            }
        }
        //Заполняем каталог информации по ребрам
        {
            for(const auto& [edge_id, parsed_track_section_info] : parsed_user_route_manager.graph_edge_id_to_info_catalog_()){
                Domain::TrackSectionInfo track_section_info;
                track_section_info.time = parsed_track_section_info.time();
                track_section_info.span_count = parsed_track_section_info.span_count();
                if (parsed_track_section_info.RouteEntity_case() == Serialization::TrackSectionInfo::kStopId) {
                    track_section_info.entity = temp_stops_catalog.at(parsed_track_section_info.stop_id());
                }
                else if (parsed_track_section_info.RouteEntity_case() == Serialization::TrackSectionInfo::kBusId) {
                    track_section_info.entity = temp_buses_catalog.at(parsed_track_section_info.bus_id());
                }
                else {
                    throw std::logic_error("Неизвестный тип ребра");
                }
                serializer_transport_router.GetGraphEdgeIdToInfoCatalog().emplace(edge_id, track_section_info);
            }
        }
    }
    
    //Заполняем настройки визуализации
    if (parsed_catalog.has_render_settings()) {
        Domain::RenderSettings render_settings;
        render_settings.width = parsed_catalog.render_settings().width();
        render_settings.height = parsed_catalog.render_settings().height();
        render_settings.padding = parsed_catalog.render_settings().padding();
        render_settings.line_width = parsed_catalog.render_settings().line_width();
        render_settings.stop_radius = parsed_catalog.render_settings().stop_radius();
        render_settings.bus_label_font_size = parsed_catalog.render_settings().bus_label_font_size();
        render_settings.bus_label_offset = {parsed_catalog.render_settings().bus_label_offset().x(),
                                            parsed_catalog.render_settings().bus_label_offset().y()};
        render_settings.stop_label_font_size = parsed_catalog.render_settings().stop_label_font_size();
        render_settings.stop_label_offset = {parsed_catalog.render_settings().stop_label_offset().x(),
                                             parsed_catalog.render_settings().stop_label_offset().y()};
        render_settings.underlayer_color = parsed_catalog.render_settings().underlayer_color();;
        render_settings.underlayer_width = parsed_catalog.render_settings().underlayer_width();
        {
            std::vector<std::string> color_palette;
            for (const std::string& parsed_color : parsed_catalog.render_settings().color_palette()) {
                color_palette.emplace_back(parsed_color);
            }
            render_settings.color_palette = std::move(color_palette);
        }
        map_renderer_.SetRenderSettings(std::move(render_settings));
    }
    
}
