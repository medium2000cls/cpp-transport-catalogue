#include "json_builder.h"

#include <utility>

namespace TransportGuide::json {

Builder::Builder() {
    node_ = Node{};
    stack_action_info_.push_back(ActionInfo{BuildActionType::BUILD, &node_});
}

BuilderKeyContext Builder::Key(std::string key) {
    if (stack_action_info_.empty()) { throw std::logic_error("stack_action_info size must be more zero."); }
    ActionInfo& info = stack_action_info_.back();
    
    if (!info.node_ptr) {return *this;}
    
    auto action = [&](ActionInfo& action_info, Node& action_last_node) {
        if (action_info.type != BuildActionType::DICT) {
            throw std::logic_error("\'"s + key + "\' must be key in Dictionary"s);
        }
        auto [it, _] = std::get<Dict>(action_last_node).emplace(key, Dict{});
        stack_action_info_.push_back(ActionInfo{BuildActionType::KEY, &(it->second)});
    };
    if (!actions_condition_info_.empty() && actions_condition_info_.back().first == info.node_ptr && actions_condition_info_.back().second == ActionCondition::IF_FALSE) {
        return {*this};
    }
    action(info, *info.node_ptr);
    return {*this};
}

Builder& Builder::Value(Node node) {
    if (stack_action_info_.empty()) { throw std::logic_error("stack_action_info size must be more zero."); }
    ActionInfo& info = stack_action_info_.back();
    
    if (!info.node_ptr) {return *this;}
    
    auto action = [&](ActionInfo& action_info, Node& action_last_node) {
        if (action_info.type == BuildActionType::KEY) {
            action_last_node = node;
            stack_action_info_.pop_back();
        }
        else if (info.type == BuildActionType::ARRAY) {
            std::get<Array>(action_last_node).push_back(node);
        }
        else if (info.type == BuildActionType::BUILD) {
            action_last_node = node;
            stack_action_info_.back() = ActionInfo{BuildActionType::VALUE, &action_last_node};
        }
        else {
            throw std::logic_error("Value must be added in pure Node or Array or Dict");
        }
        
    };
    
    if (!actions_condition_info_.empty() && actions_condition_info_.back().first == info.node_ptr && actions_condition_info_.back().second == ActionCondition::IF_FALSE) {
        return *this;
    }
    action(info, *info.node_ptr);
    return *this;
}

BuilderDictValueContext Builder::StartDict() {
    if (stack_action_info_.empty()) { throw std::logic_error("stack_action_info size must be more zero."); }
    ActionInfo& info = stack_action_info_.back();
    
    auto action = [&](ActionInfo& action_info, Node& action_last_node) {
        if (action_info.type == BuildActionType::KEY) {
            action_last_node = Dict{};
            stack_action_info_.pop_back();
            stack_action_info_.push_back(ActionInfo{BuildActionType::DICT, &action_last_node});
        }
        else if (action_info.type == BuildActionType::ARRAY) {
            auto ptr = &std::get<Array>(action_last_node).emplace_back(Dict{});
            stack_action_info_.push_back(ActionInfo{BuildActionType::DICT, ptr});
        }
        else if (action_info.type == BuildActionType::BUILD) {
            action_last_node = Dict{};
            stack_action_info_.back() = ActionInfo{BuildActionType::DICT, &action_last_node};
        }
        else {
            throw std::logic_error("StartDict must be added in pure Node or Array or Dict");
        }
    };
    
    if (!info.node_ptr || (!actions_condition_info_.empty() && actions_condition_info_.back().first == info.node_ptr && actions_condition_info_.back().second == ActionCondition::IF_FALSE)){
        stack_action_info_.push_back(ActionInfo{BuildActionType::DICT, nullptr});
        return {*this};
    }
    
    action(info, *info.node_ptr);
    
    return *this;
}

BuilderArrayValueContext Builder::StartArray() {
    if (stack_action_info_.empty()) { throw std::logic_error("stack_action_info size must be more zero."); }
    ActionInfo& info = stack_action_info_.back();
    
    auto action = [&](ActionInfo& action_info, Node& action_last_node) {
        if (action_info.type == BuildActionType::KEY) {
            action_last_node = Array{};
            stack_action_info_.pop_back();
            stack_action_info_.push_back(ActionInfo{BuildActionType::ARRAY, &action_last_node});
        }
        else if (action_info.type == BuildActionType::ARRAY) {
            auto ptr = &std::get<Array>(action_last_node).emplace_back(Array{});
            stack_action_info_.push_back(ActionInfo{BuildActionType::ARRAY, ptr});
        }
        else if (action_info.type == BuildActionType::BUILD) {
            action_last_node = Array{};
            stack_action_info_.back() = ActionInfo{BuildActionType::ARRAY, &action_last_node};
        }
        else {
            throw std::logic_error("StartArray must be added in pure Node or Array or Dict");
        }
    };
    
    if (!info.node_ptr || (!actions_condition_info_.empty() && actions_condition_info_.back().first == info.node_ptr && actions_condition_info_.back().second == ActionCondition::IF_FALSE)){
        stack_action_info_.push_back(ActionInfo{BuildActionType::ARRAY, nullptr});
        return {*this};
    }
    action(info, *info.node_ptr);
    
    return *this;
    
}

Builder& Builder::EndDict() {
    if (stack_action_info_.empty()) { throw std::logic_error("stack_action_info size must be more zero."); }
    ActionInfo& info = stack_action_info_.back();
    auto action = [&](ActionInfo& action_info) {
        if (action_info.type == BuildActionType::DICT) {
            stack_action_info_.pop_back();
        }
        else {
            throw std::logic_error("EndDict must be added in Dict");
        }
    };
    
    if (info.node_ptr && !actions_condition_info_.empty() && actions_condition_info_.back().first == info.node_ptr && actions_condition_info_.back().second == ActionCondition::IF_FALSE) {
        return *this;
    }
    action(info);
    
    return *this;
}

Builder& Builder::EndArray() {
    if (stack_action_info_.empty()) { throw std::logic_error("stack_action_info size must be more zero."); }
    ActionInfo& info = stack_action_info_.back();
    
    auto action = [&](ActionInfo& action_info) {
        if (action_info.type == BuildActionType::ARRAY) {
            stack_action_info_.pop_back();
        }
        else {
            throw std::logic_error("EndArray must be added in Array");
        }
    };
    
    if (info.node_ptr && !actions_condition_info_.empty() && actions_condition_info_.back().first == info.node_ptr && actions_condition_info_.back().second == ActionCondition::IF_FALSE) {
        return *this;
    }
    action(info);
    
    return *this;
}

Node Builder::Build() {
    if (actions_condition_info_.empty() && stack_action_info_.empty()) {
        return node_;
    }
    else if (actions_condition_info_.empty() && stack_action_info_.size() == 1 && stack_action_info_.back().type == BuildActionType::VALUE) {
        return node_;
    }
    else {
        throw std::logic_error("Build mast be added after all End or Value");
    }
}

Builder& Builder::If(bool condition) {
    if (stack_action_info_.empty()) { throw std::logic_error("stack_action_info size must be more zero."); }
    ActionInfo& info = stack_action_info_.back();
    
    if (condition) {
        actions_condition_info_.emplace_back(info.node_ptr, ActionCondition::IF_TRUE);
    }
    else {
        actions_condition_info_.emplace_back(info.node_ptr, ActionCondition::IF_FALSE);
    }
    return *this;
}

Builder& Builder::Else() {
    if (stack_action_info_.empty()) { throw std::logic_error("stack_action_info size must be more zero."); }
    ActionInfo& info = stack_action_info_.back();
    
    if (!actions_condition_info_.empty() && actions_condition_info_.back().first == info.node_ptr && actions_condition_info_.back().second == ActionCondition::IF_FALSE) {
        actions_condition_info_.back() = {info.node_ptr, ActionCondition::IF_TRUE};
    }
    else if (!actions_condition_info_.empty() && actions_condition_info_.back().first == info.node_ptr && actions_condition_info_.back().second == ActionCondition::IF_TRUE) {
        actions_condition_info_.back() = {info.node_ptr, ActionCondition::IF_FALSE};
    }
    else {
        throw std::logic_error("Else expression must be after If expression.");
    }
    return *this;
}

Builder& Builder::EndIf() {
    if (stack_action_info_.empty()) { throw std::logic_error("stack_action_info size must be more zero."); }
    ActionInfo& info = stack_action_info_.back();
    
    if (!actions_condition_info_.empty() && actions_condition_info_.back().first == info.node_ptr) {
        actions_condition_info_.pop_back();
    }
    else {
        throw std::logic_error("EndIf expression must be after If expression or Else expression.");
    }
    return *this;
}

BuilderKeyContext::BuilderKeyContext(Builder& builder) : builder_(builder) {}

BuilderDictValueContext BuilderKeyContext::Value(Node node) {
    return {builder_.Value(std::move(node))};
}

BuilderDictValueContext BuilderKeyContext::StartDict() {
    return {builder_.StartDict()};
}

BuilderArrayValueContext BuilderKeyContext::StartArray() {
    return {builder_.StartArray()};
}

BuilderDictValueContext::BuilderDictValueContext(Builder& builder) : builder_(builder) {}

BuilderKeyContext BuilderDictValueContext::Key(std::string str) {
    return {builder_.Key(std::move(str))};
}

Builder& BuilderDictValueContext::EndDict() {
    return builder_.EndDict();
}

BuilderDictValueContext BuilderDictValueContext::If(bool expr) {
    return {builder_.If(expr)};
}

BuilderDictValueContext BuilderDictValueContext::Else() {
    return {builder_.Else()};
}

BuilderDictValueContext BuilderDictValueContext::EndIf() {
    return {builder_.EndIf()};
}

BuilderArrayValueContext BuilderArrayValueContext::If(bool expr) {
    return {builder_.If(expr)};
}

BuilderArrayValueContext BuilderArrayValueContext::Else() {
    return {builder_.Else()};
}

BuilderArrayValueContext BuilderArrayValueContext::EndIf() {
    return {builder_.EndIf()};
}

BuilderArrayValueContext::BuilderArrayValueContext(Builder& builder) : builder_(builder) {}

BuilderArrayValueContext BuilderArrayValueContext::Value(Node node) {
    return {builder_.Value(std::move(node))};
}

BuilderDictValueContext BuilderArrayValueContext::StartDict() {
    return {builder_.StartDict()};
}

BuilderArrayValueContext BuilderArrayValueContext::StartArray() {
    return {builder_.StartArray()};
}

Builder& BuilderArrayValueContext::EndArray() {
    return {builder_.EndArray()};
}

}