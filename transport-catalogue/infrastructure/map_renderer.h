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

using PixelDelta = std::pair<double, double>;

struct RenderSettings {
    double width = 600.0;
    double height = 400.0;
    
    double padding = 50.0;
    
    double line_width = 14.0;
    double stop_radius = 5.0;
    
    int bus_label_font_size = 20;
    PixelDelta bus_label_offset = {7.0, 15.0};
    
    int stop_label_font_size = 20;
    PixelDelta stop_label_offset = {7.0, -3.0};
    
    svg::Color underlayer_color = "rgba(255,255,255,0.85)";
    double underlayer_width = 3.0;
    
    std::vector<std::string> color_palette = {"green", "rgb(255,160,0)", "red"};
};

class MapRenderer {
public:
    MapRenderer(const TransportGuide::BusinessLogic::TransportCatalogue& catalogue);
    
    void CreateDocument(RenderSettings settings);
    void Render(std::ostream& out);
protected:
    svg::Document GetDocument;
private:
    const BusinessLogic::TransportCatalogue& catalogue_;
    svg::Document document_;
    void CreatePolyline(svg::Document& document, const RenderSettings& settings, const std::set<std::string_view>& buses,
            const SphereProjector& sphere_projector);
    void CreateNameRoute(svg::Document& document, const RenderSettings& settings,
            const std::set<std::string_view>& buses, const SphereProjector& sphere_projector);
    void CreateCircle(svg::Document& document, const RenderSettings& settings,
            const std::vector<Domain::geo::Coordinates>& stop_coordinates, const SphereProjector& sphere_projector);
    void CreateNameStop(svg::Document& document, const RenderSettings& settings,
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