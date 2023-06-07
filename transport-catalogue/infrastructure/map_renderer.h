#pragma once
#include "../domain/geo.h"
#include "../external/svg.h"
#include "../business_logic/transport_catalogue.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>
#include <set>

namespace TransportGuide::renderer {

inline const double EPSILON = 1e-6;
bool IsZero(double value);

class SphereProjector {
public:
    // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
    template<typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                    double max_width, double max_height, double padding)
        : padding_(padding) //
    {
        // Если точки поверхности сферы не заданы, вычислять нечего
        if (points_begin == points_end) {
            return;
        }
        
        // Находим точки с минимальной и максимальной долготой
        const auto [left_it, right_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;
        
        // Находим точки с минимальной и максимальной широтой
        const auto [bottom_it, top_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;
        
        // Вычисляем коэффициент масштабирования вдоль координаты x
        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }
        
        // Вычисляем коэффициент масштабирования вдоль координаты y
        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }
        
        if (width_zoom && height_zoom) {
            // Коэффициенты масштабирования по ширине и высоте ненулевые,
            // берём минимальный из них
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        } else if (width_zoom) {
            // Коэффициент масштабирования по ширине ненулевой, используем его
            zoom_coeff_ = *width_zoom;
        } else if (height_zoom) {
            // Коэффициент масштабирования по высоте ненулевой, используем его
            zoom_coeff_ = *height_zoom;
        }
    }
    
    // Проецирует широту и долготу в координаты внутри SVG-изображения
    svg::Point operator()(Domain::geo::Coordinates coords) const;

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

class MapRenderer {
public:
    MapRenderer(const TransportGuide::BusinessLogic::TransportCatalogue& catalogue);
    
    void CreateDocument(const Domain::RenderSettings& settings);
    void CreateDocument();
    void Render(std::ostream& out);
    void SetRenderSettings(Domain::RenderSettings render_settings);
    Domain::RenderSettings GetRenderSettings() const;

private:
    const BusinessLogic::TransportCatalogue& catalogue_;
    Domain::RenderSettings render_settings_;
    svg::Document document_;
    
private:
    void CreatePolyline(svg::Document& document, const Domain::RenderSettings& settings, const std::set<std::string_view>& buses,
            const SphereProjector& sphere_projector);
    void CreateNameRoute(svg::Document& document, const Domain::RenderSettings& settings,
            const std::set<std::string_view>& buses, const SphereProjector& sphere_projector);
    void CreateCircle(svg::Document& document, const Domain::RenderSettings& settings,
            const std::vector<Domain::geo::Coordinates>& stop_coordinates, const SphereProjector& sphere_projector);
    void CreateNameStop(svg::Document& document, const Domain::RenderSettings& settings,
            const std::set<std::string_view>& stops, const SphereProjector& sphere_projector);
};

template<typename Container>
class GetFollowingIt {
public:
    GetFollowingIt(const Container& container) : container_(container) {
        position_ = 0;
    }
    
    auto operator()() {
        typename Container::const_iterator it = std::next(container_.begin(), position_);
        if (it == container_.end()) {
            position_ = 0;
            it = std::next(container_.begin(), position_);
        }
        ++position_;
        return *it;
    }

private:
    const Container& container_;
    int position_;
};
}