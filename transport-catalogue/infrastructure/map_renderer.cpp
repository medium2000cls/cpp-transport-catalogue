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
    return {
            (coords.lng - min_lon_) * zoom_coeff_ + padding_,
            (max_lat_ - coords.lat) * zoom_coeff_ + padding_
    };
}

MapRenderer::MapRenderer(const TransportGuide::BusinessLogic::TransportCatalogue& catalogue, std::ostream& out) : catalogue_(catalogue)
        , out(out) {}


void MapRenderer::CreateDocument(RenderSettings settings) {
    const std::unordered_map<std::string_view, Domain::Bus*>& bus_catalog = catalogue_.GetBusNameCatalog();
    //Получаем все указатели на маршруты
    std::set<std::string_view> buses;
    std::transform(bus_catalog.begin(), bus_catalog.end(), std::inserter(buses, buses.begin()),
            [](const std::pair<std::string_view, Domain::Bus*>& bus) { return bus.first; });
    //Получаем координаты всех остановок.
    std::vector<Domain::geo::Coordinates> stop_coordinates;
    std::for_each(buses.begin(), buses.end(), [&](const std::string_view bus_name){
        const Domain::Bus* bus_ptr = bus_catalog.at(bus_name);
        for (auto stop : bus_ptr->route) {
            stop_coordinates.push_back(Domain::geo::Coordinates{stop->latitude, stop->longitude});
        }
    });
    //Создаем интерпретатор координат из геоцентрической системы в прямоугольную
    SphereProjector sphere_projector(stop_coordinates.begin(), stop_coordinates.end(), settings.width,
            settings.height, settings.padding);
    GetFollowingIt get_following_color(settings.color_palette);
    
    //создаем документ svg
    svg::Document document;
    for (const auto& bus_name : buses) {
        const Domain::Bus* bus_ptr = bus_catalog.at(bus_name);
        if (bus_ptr->route.empty()) { continue; }
        document.AddPtr(CreateRoute(bus_ptr, settings, sphere_projector, get_following_color()));
    }
    document_ = std::move(document);
}


void MapRenderer::Render() {
    document_.Render(out);
}


std::unique_ptr<svg::Object> MapRenderer::CreateRoute(const Domain::Bus* bus_ptr, const RenderSettings& settings, const SphereProjector& sphere_projector,
        const svg::Color& color_route) const {
    std::unique_ptr<svg::Polyline> polyline = std::make_unique<svg::Polyline>();
    std::vector<const Domain::Stop*> unique_route = bus_ptr->route;
    unique_route.erase(std::unique(unique_route.begin(), unique_route.end()), unique_route.end());
    for (auto stop : unique_route) {
        svg::Point point = sphere_projector(Domain::geo::Coordinates{stop->latitude, stop->longitude});
        polyline->AddPoint(point);
    }
    polyline->SetStrokeColor(color_route)
            .SetFillColor(svg::NoneColor)
            .SetStrokeWidth(settings.line_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    return polyline;
}


} //namespace TransportGuide::renderer
