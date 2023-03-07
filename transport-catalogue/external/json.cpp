#include <algorithm>
#include <variant>

#include "json.h"

namespace TransportGuide::json {

//region --------Node--------
bool Node::IsInt() const {
    return std::holds_alternative<int>(*this);
}

bool Node::IsDouble() const {
    return std::holds_alternative<double>(*this) || std::holds_alternative<int>(*this);
}

bool Node::IsPureDouble() const {
    return std::holds_alternative<double>(*this);
}

bool Node::IsBool() const {
    return std::holds_alternative<bool>(*this);
}

bool Node::IsString() const {
    return std::holds_alternative<std::string>(*this);
}

bool Node::IsNull() const {
    return std::holds_alternative<std::nullptr_t>(*this);
}

bool Node::IsArray() const {
    return std::holds_alternative<Array>(*this);
}

bool Node::IsMap() const {
    return std::holds_alternative<Dict>(*this);
}

int Node::AsInt() const {
    if (const auto* value_ptr = std::get_if<int>(this)) {
        return *value_ptr;
    }
    else {
        throw std::logic_error("Value is not of this type.");
    }
}

bool Node::AsBool() const {
    if (const auto* value_ptr = std::get_if<bool>(this)) {
        return *value_ptr;
    }
    else {
        throw std::logic_error("Value is not of this type.");
    }
}

double Node::AsDouble() const {
    if (const auto* value_ptr = std::get_if<double>(this)) {
        return *value_ptr;
    }
    else if (const auto* value_point = std::get_if<int>(this)) {
        return static_cast<double>(*value_point);
    }
    else {
        throw std::logic_error("Value is not of this type.");
    }
}

const std::string& Node::AsString() const {
    if (const auto* value_ptr = std::get_if<std::string>(this)) {
        return *value_ptr;
    }
    else {
        throw std::logic_error("Value is not of this type.");
    }
}

const Array& Node::AsArray() const {
    if (const auto* value_ptr = std::get_if<Array>(this)) {
        return *value_ptr;
    }
    else {
        throw std::logic_error("Value is not of this type.");
    }
}

const Dict& Node::AsMap() const {
    if (const auto* value_ptr = std::get_if<Dict>(this)) {
        return *value_ptr;
    }
    else {
        throw std::logic_error("Value is not of this type.");
    }
}

bool Node::operator==(const Node& rhs) const {
    auto node_variant_lhs = static_cast<const NodeVariant&>(*this);
    auto node_variant_rhs = static_cast<const NodeVariant&>(rhs);
    return node_variant_lhs == node_variant_rhs;
}

bool Node::operator!=(const Node& rhs) const {
    return !(*this == rhs);
}

//endregion

namespace {
using namespace std::literals;

inline void SkipEscapeSequence(std::istream& input) {
    const std::string escape_symbol_list = " \n\r\t";
    while (input.peek() != EOF && escape_symbol_list.find(input.peek()) != std::string::npos) {
        char c;
        input.get(c);
    }
}

using Number = std::variant<int, double>;

Number LoadNumber(std::istream& input) {
    using namespace std::literals;
    
    std::string parsed_num;
    
    // Считывает в parsed_num очередной символ из input
    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
    };
    
    // Считывает одну или более цифр в parsed_num из input
    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
            throw ParsingError("A digit is expected"s);
        }
        while (std::isdigit(input.peek())) {
            read_char();
        }
    };
    
    if (input.peek() == '-') {
        read_char();
    }
    // Парсим целую часть числа
    if (input.peek() == '0') {
        read_char();
        // После 0 в JSON не могут идти другие цифры
    }
    else {
        read_digits();
    }
    
    bool is_int = true;
    // Парсим дробную часть числа
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }
    
    // Парсим экспоненциальную часть числа
    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }
    
    try {
        if (is_int) {
            // Сначала пробуем преобразовать строку в int
            try {
                return std::stoi(parsed_num);
            }
            catch (...) {
                // В случае неудачи, например, при переполнении,
                // код ниже попробует преобразовать строку в double
            }
        }
        return std::stod(parsed_num);
    }
    catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

// Считывает содержимое строкового литерала JSON-документа
// Функцию следует использовать после считывания открывающего символа ":
std::string LoadString(std::istream& input) {
    using namespace std::literals;
    
    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string s;
    while (true) {
        if (it == end) {
            // Поток закончился до того, как встретили закрывающую кавычку?
            throw ParsingError("String parsing error");
        }
        const char ch = *it;
        if (ch == '"') {
            // Встретили закрывающую кавычку
            ++it;
            break;
        }
        else if (ch == '\\') {
            // Встретили начало escape-последовательности
            ++it;
            if (it == end) {
                // Поток завершился сразу после символа обратной косой черты
                throw ParsingError("String parsing error");
            }
            const char escaped_char = *(it);
            // Обрабатываем одну из последовательностей: \\, \n, \t, \r, \"
            switch (escaped_char) {
                case 'n':
                    s.push_back('\n');
                    break;
                case 't':
                    s.push_back('\t');
                    break;
                case 'r':
                    s.push_back('\r');
                    break;
                case '"':
                    s.push_back('"');
                    break;
                case '\\':
                    s.push_back('\\');
                    break;
                default:
                    // Встретили неизвестную escape-последовательность
                    throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
            }
        }
        else if (ch == '\n' || ch == '\r') {
            // Строковый литерал внутри- JSON не может прерываться символами \r или \n
            throw ParsingError("Unexpected end of line"s);
        }
        else {
            // Просто считываем очередной символ и помещаем его в результирующую строку
            s.push_back(ch);
        }
        ++it;
    }
    
    return s;
}

Node LoadNode(std::istream& input) {
    const std::string end_word_symbol_list = " \n\r\t\\:,}]";
    SkipEscapeSequence(input);
    char c = input.peek();
    if (c == '\"') {
        input.get(c);
        std::string str = LoadString(input);
        return Node(std::move(str));
    }
    else if (c == '{') {
        input.get(c);
        Dict map;
        while (true) {
            SkipEscapeSequence(input);
            if (input.peek() == '}') {
                input.get(c);
                break;
            }
            std::string str_key;
            
            input.get(c);
            if (c == '\"') {
                str_key = LoadString(input);
            }
            else {
                throw json::ParsingError("Key must be in an environment symbol \'\"\'"s);
            }
            
            SkipEscapeSequence(input);
            input.get(c);
            
            if (c != ':') {
                throw json::ParsingError("After key must be symbol \':\', now \'"s + c + "\'"s);
            }
            
            Node node = LoadNode(input);
            map.emplace(std::move(str_key), std::move(node));
            
            SkipEscapeSequence(input);
            input.get(c);
            
            if (c == ',') {
                continue;
            }
            else if (c == '}') {
                break;
            }
            else {
                throw json::ParsingError("After value must be symbol \',\' or \'}\'\n"s + c);
            }
        }
        return Node(std::move(map));
    }
    else if (c == '[') {
        input.get(c);
        Array arr;
        while (true) {
            SkipEscapeSequence(input);
            if (input.peek() == ']') {
                input.get(c);
                break;
            }
            
            Node node = LoadNode(input);
            arr.emplace_back(std::move(node));
            
            SkipEscapeSequence(input);
            input.get(c);
            
            if (c == ',') {
                continue;
            }
            else if (c == ']') {
                break;
            }
            else {
                throw json::ParsingError("After value must be symbol \',\' or \']\'\n"s + c);
            }
        }
        return Node(std::move(arr));
        
    }
    else if (c == 't' || c == 'f' || c == 'n') {
        std::string str;
        input.get(c);
        str += c;
        while (input.get(c)) {
            str += c;
            if (end_word_symbol_list.find(input.peek()) != std::string::npos) {
                break;
            }
        }
        if (str == "true") {
            return Node(true);
        }
        else if (str == "false") {
            return Node(false);
        }
        else if (str == "null") {
            return Node(nullptr);
        }
        else {
            throw json::ParsingError("Description with an error");
        }
    }
    else if (std::isdigit(c) || c == '+' || c == '-') {
        Number number = LoadNumber(input);
        if (std::holds_alternative<int>(number)) {
            int num = std::get<int>(number);
            return Node(num);
        }
        else if (std::holds_alternative<double>(number)) {
            double num = std::get<double>(number);
            return Node(num);
        }
    }
    else {
        throw json::ParsingError("Invalid character \'"s + c + "\'");
    }
    return Node(nullptr);
}


struct PrintContext {
    int indent_step = 4;
    int indent = 0;
    bool from_map = false;
    
    std::string PrintIndent() const {
        return std::string(indent, ' ');
    }
    
    // Возвращает новый контекст вывода с увеличенным смещением
    PrintContext Indented(bool is_map = false) const {
        return {indent_step, indent_step + indent, is_map};
    }
};


std::ostream& PrintNode(std::ostream& out, const Node& node, PrintContext context = {}) {
    PrintContext context_indented = context.Indented();
    if (node.IsInt()) {
        if (!context.from_map) { out << context.PrintIndent(); }
        out << node.AsInt();
    }
    else if (node.IsDouble()) {
        if (!context.from_map) { out << context.PrintIndent(); }
        out << node.AsDouble();
    }
    else if (node.IsBool()) {
        if (!context.from_map) { out << context.PrintIndent(); }
        out << (node.AsBool() ? "true" : "false");
    }
    else if (node.IsString()) {
        const std::string& str = node.AsString();
        std::string new_str;
        std::for_each(str.begin(), str.end(), [&new_str](char c) {
            if (c == '\r') {
                new_str += R"(\r)";
            }
            else if (c == '\n') {
                new_str += R"(\n)";
            }
            else if (c == '\"') {
                new_str += R"(\")";
            }
            else if (c == '\\') {
                new_str += R"(\\)";
            }
            else {
                new_str += c;
            }
        });
        if (!context.from_map) { out << context.PrintIndent(); }
        out <<  "\"" << new_str << "\"";
    }
    else if (node.IsNull()) {
        if (!context.from_map) { out << context.PrintIndent(); }
        out <<  "null";
    }
    else if (node.IsArray()) {
        if (!context.from_map) { out << context.PrintIndent(); }
        out << "[";
        bool is_first = true;
        for (auto internal_node : node.AsArray()) {
            if (is_first) {
                out << "\n";
                is_first = false;
            }
            else {
                out << ",\n";
            }
            PrintNode(out, internal_node, context_indented);
        }
        if(!node.AsArray().empty()) out << "\n";
        out << context.PrintIndent();
        out <<  "]";
    }
    else if (node.IsMap()) {
        if (!context.from_map) { out << context.PrintIndent(); }
        out << "{";
        bool is_first = true;
        for (auto [key, internal_node] : node.AsMap()) {
            if (is_first) {
                out << "\n";
                is_first = false;
            }
            else {
                out << ",\n";
            }
            out << context_indented.PrintIndent();
            out << "\"" << key << "\": ";
            PrintNode(out, internal_node, context.Indented(true));
        }
        out << "\n";
        out << context.PrintIndent();
        out <<  "}";
    }
    return out;
}

}  // namespace


//region --------Document--------

Document::Document(Node root) : root_(std::move(root)) {
}

const Node& Document::GetRoot() const {
    return root_;
}

bool Document::operator==(const Document& rhs) const {
    return root_ == rhs.root_;
}

bool Document::operator!=(const Document& rhs) const {
    return !(rhs == *this);
}

Document Load(std::istream& input) {
    return Document(LoadNode(input));
}

//endregion


void Print(const Document& doc, std::ostream& output) {
    PrintNode(output, doc.GetRoot(), PrintContext());
}

}  // namespace json