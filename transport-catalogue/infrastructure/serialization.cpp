#include <transport_catalogue.pb.h>
#include "serialization.h"

TransportGuide::IoRequests::ProtoSerialization::ProtoSerialization(
        TransportGuide::BusinessLogic::SerializerTransportCatalogue& serializer_transport_catalogue, renderer::MapRenderer& map_renderer) :
        SerializerBase(serializer_transport_catalogue), map_renderer_(map_renderer) {}

void TransportGuide::IoRequests::ProtoSerialization::Serialize([[maybe_unused]]std::ostream& output) {
    Serialization::TransportCatalogue result_catalogue;
    for (const auto& stop : serializer_catalogue_.GetStopCatalog()) {
        Serialization::Stop ser_stop;
        ser_stop.set_id(reinterpret_cast<uint64_t>(&stop));
        ser_stop.set_name(stop.name);
        ser_stop.set_latitude(stop.latitude);
        ser_stop.set_longitude(stop.longitude);
        ser_stop.set_is_fill(stop.is_fill);
        result_catalogue.add_stops()->CopyFrom(ser_stop);
    }
    for (const auto& bus : serializer_catalogue_.GetBusCatalog()) {
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
    for (const auto& [sector, distance] : serializer_catalogue_.GetCalculatedDistanceCatalog()) {
        Serialization::CalculatedDistance ser_sector;
        ser_sector.set_from_stop_id(reinterpret_cast<uint64_t>(sector.first));
        ser_sector.set_to_stop_id(reinterpret_cast<uint64_t>(sector.second));
        ser_sector.set_distance(distance);
        result_catalogue.add_calculated_distance_catalog()->CopyFrom(ser_sector);
    }
    for (const auto& [sector, distance] : serializer_catalogue_.GetRealDistanceCatalog()) {
        Serialization::RealDistance ser_sector;
        ser_sector.set_from_stop_id(reinterpret_cast<uint64_t>(sector.first));
        ser_sector.set_to_stop_id(reinterpret_cast<uint64_t>(sector.second));
        ser_sector.set_distance(distance);
        result_catalogue.add_real_distance_catalog()->CopyFrom(ser_sector);
    }
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
    if (serializer_catalogue_.GetUserRouteManager().has_value()) {
        Serialization::RoutingSettings ser_router;
        Domain::RoutingSettings routing_settings = serializer_catalogue_.GetUserRouteManager()->GetRoutingSettings();
        ser_router.set_bus_velocity(routing_settings.bus_velocity);
        ser_router.set_bus_wait_time(routing_settings.bus_wait_time);
        result_catalogue.mutable_routing_settings()->CopyFrom(ser_router);
    }
    result_catalogue.SerializeToOstream(&output);
}

void TransportGuide::IoRequests::ProtoSerialization::Deserialize([[maybe_unused]]std::istream& input) {
    Serialization::TransportCatalogue parsed_catalog;
    std::map<uint64_t, const Domain::Stop*> temp_stops_catalog;
    std::map<uint64_t, const Domain::Bus*> temp_buses_catalog;
    parsed_catalog.ParseFromIstream(&input);
    
    // Заполняем каталог остановок
    for (const auto& stop : parsed_catalog.stops()) {
        Domain::Stop* s_ptr = &serializer_catalogue_.GetStopCatalog()
                                            .emplace_back(stop.name(), stop.latitude(), stop.longitude());
        //создаем каталог имен и ссылок на остановки
        serializer_catalogue_.GetStopNameCatalog().emplace(s_ptr->name, s_ptr);
        temp_stops_catalog.emplace(stop.id(), s_ptr);
        
    }
    //Заполняем каталог маршрутов
    for (const auto& bus : parsed_catalog.buses()) {
        std::vector<const Domain::Stop*> route;
        for (const auto& stop_id : bus.route()) {
            route.emplace_back(temp_stops_catalog.at(stop_id));
        }
        // создаем маршрут
        Domain::Bus* b_ptr = &serializer_catalogue_.GetBusCatalog().emplace_back(bus.name(), route,
                bus.number_final_stop_(), bus.calc_length(), bus.real_length());
        
        //создаем список маршрутов у остановок
        for (const Domain::Stop* stop_ptr : route) {
            serializer_catalogue_.GetStopBusesCatalog()[stop_ptr].emplace(b_ptr);
        }
        //создаем каталог имен и ссылок на маршруты
        serializer_catalogue_.GetBusNameCatalog().emplace(b_ptr->name, b_ptr);
        
        temp_buses_catalog.emplace(bus.id(), b_ptr);
    }
    //Заполняем каталог посчитанных расстояний
    for (const auto& entity : parsed_catalog.calculated_distance_catalog()) {
        serializer_catalogue_.GetCalculatedDistanceCatalog()
                             .emplace(std::make_pair(temp_stops_catalog.at(entity.from_stop_id()),
                                     temp_stops_catalog.at(entity.to_stop_id())), entity.distance());
    }
    //Заполняем каталог реальных расстояний
    for (const auto& entity : parsed_catalog.real_distance_catalog()) {
        serializer_catalogue_.GetRealDistanceCatalog()
                             .emplace(std::make_pair(temp_stops_catalog.at(entity.from_stop_id()),
                                     temp_stops_catalog.at(entity.to_stop_id())), entity.distance());
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
    //Если есть настройки маршрутов, зполняем менеджер маршрутов
    if (parsed_catalog.has_routing_settings()) {
        serializer_catalogue_.GetCatalogue().ConstructUserRouteManager({parsed_catalog.routing_settings().bus_wait_time(),
                                                                        parsed_catalog.routing_settings().bus_velocity()});
    }
}
