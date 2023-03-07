#include <algorithm>
#include <regex>
#include <iomanip>
#include "svg.h"

namespace TransportGuide::svg {

using namespace std::literals;

//region --------- Color ---------
Rgb::Rgb(uint8_t red, uint8_t green, uint8_t blue) : red(red), green(green), blue(blue) {}
Rgba::Rgba(uint8_t red, uint8_t green, uint8_t blue, double opacity): red(red), green(green), blue(blue), opacity(opacity) {}

std::string RenderColorAttribute::operator()(std::monostate) const {
    return "none";
}
std::string RenderColorAttribute::operator()(std::string str) const {
    return str;
}
std::string RenderColorAttribute::operator()(Rgb rgb) const {
    return "rgb("s + std::to_string(rgb.red) + ","s + std::to_string(rgb.green) + ","s + std::to_string(rgb.blue) + ")"s;
}
std::string RenderColorAttribute::operator()(Rgba rgba) const {
    std::ostringstream o_str;
    //o_str.precision(15);
    o_str << rgba.opacity;
    return "rgba("s + std::to_string(rgba.red) + ","s + std::to_string(rgba.green) + ","s + std::to_string(rgba.blue) + ","s + o_str.str() + ")"s;
}

std::ostream& operator<<(std::ostream &out, const Color& color) {
    out << std::visit(RenderColorAttribute {}, color);
    return out;
}

//endregion

//region --------- Enums Stroke ---------

std::ostream& operator<<(std::ostream& out, const StrokeLineCap& stroke_line_cap) {
    std::string str_out;
    if (stroke_line_cap == StrokeLineCap::BUTT) { str_out = "butt"; }
    else if (stroke_line_cap == StrokeLineCap::ROUND) { str_out = "round"; }
    else if (stroke_line_cap == StrokeLineCap::SQUARE) { str_out = "square"; }
    
    if (!str_out.empty()) { out << str_out; }
    return out;
}

std::ostream& operator<<(std::ostream& out, const StrokeLineJoin& stroke_line_join) {
    std::string str_out;
    if (stroke_line_join == StrokeLineJoin::ARCS) { str_out = "arcs"; }
    else if (stroke_line_join == StrokeLineJoin::BEVEL) { str_out = "bevel"; }
    else if (stroke_line_join == StrokeLineJoin::MITER) { str_out = "miter"; }
    else if (stroke_line_join == StrokeLineJoin::MITER_CLIP) { str_out = "miter-clip"; }
    else if (stroke_line_join == StrokeLineJoin::ROUND) { str_out = "round"; }
    
    if (!str_out.empty()) { out << str_out; }
    return out;
}

//endregion

//region --------- Object ---------

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();
    
    // Делегируем вывод тега своим подклассам
    RenderObject(context);
    
    context.out << std::endl;
}

//endregion

//region ---------- Circle ----------

Circle& Circle::SetCenter(Point center)  {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius)  {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
    out << "r=\""sv << radius_ << "\""sv;
    RenderAttrs(out);
    out << "/>"sv;
}
//endregion

//region ---------- Polyline ----------

Polyline& Polyline::AddPoint(Point point) {
    pointers_.push_back(point);
    return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<polyline points=\""sv;
    bool is_not_first_point = false;
    for (const auto& p : pointers_) {
        if (is_not_first_point) {
            out << " ";
        }
        is_not_first_point = true;
        out << p.x << "," << p.y;
    }
    out << "\"";
    RenderAttrs(out);
    out << "/>"sv;
}

//endregion

//region ---------- Text ----------

Text& Text::SetPosition(Point pos) {
    pos_ = pos;
    return *this;
}

Text& Text::SetOffset(Point offset) {
    offset_ = offset;
    return *this;
}

Text& Text::SetFontSize(uint32_t size) {
    size_ = size;
    return *this;
}

Text& Text::SetFontFamily(std::string font_family) {
    font_family_ = std::forward<std::string>(font_family);
    return *this;
}

Text& Text::SetFontWeight(std::string font_weight) {
    font_weight_ = std::forward<std::string>(font_weight);
    return *this;
}

Text& Text::SetData(std::string data) {
    data = std::regex_replace(data, std::regex("&"), "&amp;");
    data = std::regex_replace(data, std::regex("\""), "&quot;");
    data = std::regex_replace(data, std::regex("\'"), "&apos;");
    data = std::regex_replace(data, std::regex("<"), "&lt;");
    data = std::regex_replace(data, std::regex(">"), "&gt;");
    data_ = std::forward<std::string>(data);
    return *this;
}

void Text::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<text";
    RenderAttrs(out);
    out << " x=\"" << pos_.x << "\" y=\"" << pos_.y << "\" dx=\"" << offset_.x << "\" dy=\"" << offset_.y
            << "\" font-size=\"" << size_ << "\"";
    if (!font_family_.empty()) { out << " font-family=\"" << font_family_ << "\""; }
    if (!font_weight_.empty()) { out << " font-weight=\"" << font_weight_ << "\""; }
    out << ">" << data_ << "</text>"sv;
}

//endregion

//region --------- Document ---------

void Document::AddPtr(std::unique_ptr<Object>&& obj) {
    objects_.push_back(std::move(obj));
}

void Document::Render(std::ostream& out) const {
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n";
    RenderContext ctx(out, 2, 2);
    for (auto& ptr : objects_) {
        ptr->Render(ctx);
    }
    out << "</svg>";
}

//endregion

}  // namespace svg