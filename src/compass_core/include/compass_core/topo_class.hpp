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
// Steering side bias of a class: the mean of its pair signs (L = +1, R = -1),
// in [-1, 1]. Empty and balanced (cancelling) classes return 0, i.e. no lateral
// bias. This is the single definition used by the path tracker's side-bias term,
// the candidate rollouts and the planned lateral offset L_plan (issue #8).
double side_bias(const TopoClass & c);
}  // namespace compass
