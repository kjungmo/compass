// topo_class.cpp
#include "compass_core/topo_class.hpp"
namespace compass {
int TopoClass::hamming(const TopoClass & a, const TopoClass & b) {
  int d = 0;
  for (const auto & [id, s] : a.pairs_) {
    auto bs = b.side(id);
    if (!bs.has_value() || bs.value() != s) d++;
  }
  for (const auto & [id, s] : b.pairs_) { if (!a.side(id).has_value()) d++; }
  return d;
}
bool TopoClass::lex_less(const TopoClass & a, const TopoClass & b) {
  auto ia = a.pairs_.begin(); auto ib = b.pairs_.begin();
  for (; ia != a.pairs_.end() && ib != b.pairs_.end(); ++ia, ++ib) {
    if (ia->first != ib->first) return ia->first < ib->first;
    if (ia->second != ib->second) return ia->second < ib->second;  // L(0) < R(1)
  }
  return a.pairs_.size() < b.pairs_.size();
}
TopoClass TopoClass::restrict(const std::set<uint64_t> & removed) const {
  TopoClass r;
  for (const auto & [id, s] : pairs_) if (!removed.count(id)) r.set(id, s);
  return r;
}
}  // namespace compass
