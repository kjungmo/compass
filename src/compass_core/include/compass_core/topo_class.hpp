// topo_class.hpp
#pragma once
#include <map>
#include <set>
#include <optional>
#include <cstdint>
namespace compass {
enum class Side { L = 0, R = 1 };
class TopoClass {
public:
  void set(uint64_t id, Side s) { pairs_[id] = s; }
  std::optional<Side> side(uint64_t id) const {
    auto it = pairs_.find(id);
    return it == pairs_.end() ? std::nullopt : std::optional<Side>(it->second);
  }
  size_t size() const { return pairs_.size(); }
  bool equals(const TopoClass & o) const { return pairs_ == o.pairs_; }
  static int hamming(const TopoClass & a, const TopoClass & b);
  static bool lex_less(const TopoClass & a, const TopoClass & b);
  TopoClass restrict(const std::set<uint64_t> & removed) const;
  const std::map<uint64_t, Side> & pairs() const { return pairs_; }
private:
  std::map<uint64_t, Side> pairs_;   // sorted by id
};
}  // namespace compass
