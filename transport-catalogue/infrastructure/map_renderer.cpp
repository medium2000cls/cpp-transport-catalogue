#include <set>
#include <utility>
#include <vector>
#include "map_renderer.h"

/*
 * В этом файле вы можете разместить код, отвечающий за визуализацию карты маршрутов в формате SVG.
 * Визуализация маршрутов вам понадобится во второй части итогового проекта.
 * Пока можете оставить файл пустым.
 */

namespace TransportGuide::renderer {

bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

svg::Point SphereProjector::operator()(Domain::geo::Coordinates coords) const {
    return {(coords.lng - min_lon_) * zoom_coeff_ + padding_, (max_lat_ - coords.lat) * zoom_coeff_ + padding_};
}

MapRenderer::MapRenderer(const TransportGuide::BusinessLogic::TransportCatalogue& catalogue,
        std::ostream& out) : catalogue_(catalogue), out(out) {}


void MapRenderer::CreateDocument(RenderSettings settings) {
    const std::unordered_map<std::string_view, Domain::Bus*>& bus_catalog = catalogue_.GetBusNameCatalog();
    const std::unordered_map<std::string_view, Domain::Stop*>& stop_catalog = catalogue_.GetStopNameCatalog();
    //Получаем все имена маршрутов
    std::set<std::string_view> buses;
    for (const auto& [bus_name, bus_ptr] : bus_catalog) {
        if (!bus_ptr->route.empty()) { buses.insert(bus_name); }
    }
    //Получаем все остановки
    std::set<std::string_view> stops;
    for(const  auto& bus_name : buses) {
        const Domain::Bus* bus_ptr = bus_catalog.at(bus_name);
        for(const auto& stop : bus_ptr->route){
            stops.insert(stop->name);
        }
    }
    //Получаем координаты всех остановок.
    std::vector<Domain::geo::Coordinates> stop_coordinates;
    for (auto stop_name : stops) {
        const auto& stop = stop_catalog.at(stop_name);
        stop_coordinates.push_back(Domain::geo::Coordinates{stop->latitude, stop->longitude});
    }
    
    //Создаем интерпретатор координат из геоцентрической системы в прямоугольную
    SphereProjector sphere_projector(stop_coordinates.begin(), stop_coordinates.end(), settings.width, settings.height,
            settings.padding);
    
    //создаем документ svg
    svg::Document document;
    
    CreatePolyline(document, settings, buses, sphere_projector);
    CreateNameRoute(document, settings, buses, sphere_projector);
    CreateCircle(document, settings, stop_coordinates, sphere_projector);
    CreateNameStop(document, settings, stops, sphere_projector);
    document_ = std::move(document);
}

void MapRenderer::CreatePolyline(svg::Document& document, const RenderSettings& settings,
        const std::set<std::string_view>& buses, const SphereProjector& sphere_projector) {
    const std::unordered_map<std::string_view, Domain::Bus*>& bus_catalog = catalogue_.GetBusNameCatalog();
    GetFollowingIt get_following_color(settings.color_palette);
    
    for (const auto& bus_name : buses) {
        const Domain::Bus* bus_ptr = bus_catalog.at(bus_name);
        
        std::unique_ptr<svg::Polyline> polyline = std::make_unique<svg::Polyline>();
        std::vector<const Domain::Stop*> unique_route = bus_ptr->route;
        unique_route.erase(std::unique(unique_route.begin(), unique_route.end()), unique_route.end());
        for (auto stop : unique_route) {
            svg::Point point = sphere_projector({stop->latitude, stop->longitude});
            polyline->AddPoint(point);
        }
        polyline->SetStrokeColor(get_following_color())
                .SetFillColor(svg::NoneColor)
                .SetStrokeWidth(settings.line_width)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        
        
        document.AddPtr(std::move(polyline));
    }
}

void MapRenderer::CreateNameRoute(svg::Document& document, const RenderSettings& settings,
        const std::set<std::string_view>& buses, const SphereProjector& sphere_projector) {
    const std::unordered_map<std::string_view, Domain::Bus*>& bus_catalog = catalogue_.GetBusNameCatalog();
    GetFollowingIt get_following_color(settings.color_palette);
    
    auto add_title_to_doc = [&](const Domain::Bus* bus_ptr) {
        //добавляю координаты в пикселях
        std::vector<svg::Point> stops_point;
        std::vector<const Domain::Stop*> route = bus_ptr->GetForwardRoute();
        stops_point.push_back(sphere_projector(
                {route.front()->latitude, route.front()->longitude}));
        if (!bus_ptr->IsRoundtrip() && route.front() != route.back()) {
            stops_point.push_back(sphere_projector(
                    {route.back()->latitude, route.back()->longitude}));
        }
        auto color = get_following_color();
        for (auto point : stops_point) {
            std::unique_ptr<svg::Text> route_name_background = std::make_unique<svg::Text>();
            route_name_background->SetPosition(point)
                               .SetOffset({settings.bus_label_offset.first, settings.bus_label_offset.second})
                               .SetFontSize(settings.bus_label_font_size)
                               .SetFontFamily("Verdana")
                               .SetFontWeight("bold")
                               .SetData(bus_ptr->name)
                               .SetFillColor(settings.underlayer_color)
                               .SetStrokeColor(settings.underlayer_color)
                               .SetStrokeWidth(settings.underlayer_width)
                               .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                               .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
            document.AddPtr(std::move(route_name_background));
            std::unique_ptr<svg::Text> route_name = std::make_unique<svg::Text>();
            route_name->SetPosition(point)
                      .SetOffset({settings.bus_label_offset.first, settings.bus_label_offset.second})
                      .SetFontSize(settings.bus_label_font_size)
                      .SetFontFamily("Verdana")
                      .SetFontWeight("bold")
                      .SetData(bus_ptr->name)
                      .SetFillColor(color);
            document.AddPtr(std::move(route_name));
        }
    };
    
    for (const auto& bus_name : buses) {
        const Domain::Bus* bus_ptr = bus_catalog.at(bus_name);
        add_title_to_doc(bus_ptr);
    }
}

void MapRenderer::CreateCircle(svg::Document& document, const RenderSettings& settings,
        const std::vector<Domain::geo::Coordinates>& stop_coordinates, const SphereProjector& sphere_projector) {
    //добавляю координаты в пикселях
    std::vector<svg::Point> stops_point;
    for (const auto& stop_coord : stop_coordinates) {
        stops_point.push_back(sphere_projector(stop_coord));
    }
    for (auto point : stops_point) {
        std::unique_ptr<svg::Circle> stop_circle = std::make_unique<svg::Circle>();
        stop_circle->SetCenter(point).SetRadius(settings.stop_radius).SetFillColor("white");
        document.AddPtr(std::move(stop_circle));
    }
}

void MapRenderer::CreateNameStop(svg::Document& document, const RenderSettings& settings,
        const std::set<std::string_view>& stops, const SphereProjector& sphere_projector) {
    
    const std::unordered_map<std::string_view, Domain::Stop*>& stop_catalog = catalogue_.GetStopNameCatalog();
    GetFollowingIt get_following_color(settings.color_palette);
    
    auto add_title_to_doc = [&](const Domain::Stop* stop_ptr) {
        //добавляю координаты в пикселях
        svg::Point point = sphere_projector({stop_ptr->latitude, stop_ptr->longitude});
        
        std::unique_ptr<svg::Text> stop_name_background = std::make_unique<svg::Text>();
        stop_name_background->SetPosition(point)
                            .SetOffset({settings.stop_label_offset.first, settings.stop_label_offset.second})
                            .SetFontSize(settings.stop_label_font_size)
                            .SetFontFamily("Verdana")
                            .SetData(stop_ptr->name)
                            .SetFillColor(settings.underlayer_color)
                            .SetStrokeColor(settings.underlayer_color)
                            .SetStrokeWidth(settings.underlayer_width)
                            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        document.AddPtr(std::move(stop_name_background));
        
        std::unique_ptr<svg::Text> stop_name_obj = std::make_unique<svg::Text>();
        stop_name_obj->SetPosition(point)
                     .SetOffset({settings.stop_label_offset.first, settings.stop_label_offset.second})
                     .SetFontSize(settings.stop_label_font_size)
                     .SetFontFamily("Verdana")
                     .SetData(stop_ptr->name)
                     .SetFillColor("black");
        document.AddPtr(std::move(stop_name_obj));
    };
    
    for (const auto& stop_name : stops) {
        const Domain::Stop* stop_ptr = stop_catalog.at(stop_name);
        add_title_to_doc(stop_ptr);
    }
    
}

void MapRenderer::Render() {
    document_.Render(out);
}

} //namespace TransportGuide::renderer
