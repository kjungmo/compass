// tie_breaker.cpp
#include "compass_core/tie_breaker.hpp"
#include <limits>
namespace compass {

namespace {
// R6 사전 순서 비교: a 가 b 보다 우선이면 true.
// 1) prev_c_prime 일치 우선 2) c* 와의 해밍 거리 작은 쪽 3) lex_less.
bool prefer(const TopoClass & a, const TopoClass & b,
            const DecisionState & st) {
  const bool a_prev = st.prev_c_prime.has_value() && a.equals(st.prev_c_prime.value());
  const bool b_prev = st.prev_c_prime.has_value() && b.equals(st.prev_c_prime.value());
  if (a_prev != b_prev) return a_prev;  // prev_c_prime 보유 쪽 우선
  const int ha = TopoClass::hamming(a, st.c_star);
  const int hb = TopoClass::hamming(b, st.c_star);
  if (ha != hb) return ha < hb;          // 해밍 거리 작은 쪽
  return TopoClass::lex_less(a, b);      // 사전식 최소
}
}  // namespace

TopoClass TieBreaker::pick(const std::vector<ClassEval> & challengers,
                           const DecisionState & st, const Knobs & k) {
  if (challengers.empty()) {
    locked_.reset();
    return st.c_star;  // 도전자 없음: 현재 class 유지
  }

  // 최선 J.
  double best_j = std::numeric_limits<double>::infinity();
  for (const auto & c : challengers) best_j = std::min(best_j, c.J);

  // 히스테리시스: 고정 우승자가 여전히 도전자 집합에 있고 그 J 가
  // 최선보다 ε_out 이하로 떨어져 있으면 우승자 유지.
  if (locked_.has_value()) {
    for (const auto & c : challengers) {
      if (c.cls.equals(locked_.value())) {
        if (c.J - best_j <= k.eps_out) {
          return locked_.value();  // 채터링 봉쇄: 유지
        }
        break;
      }
    }
  }

  // 새로 선택: ε_out 이내 동률 집합에서 R6 우선순위 최상위.
  TopoClass winner = challengers.front().cls;
  bool have_winner = false;
  for (const auto & c : challengers) {
    if (c.J - best_j > k.eps_out) continue;  // 동률 집합 밖
    if (!have_winner || prefer(c.cls, winner, st)) {
      winner = c.cls;
      have_winner = true;
    }
  }
  locked_ = winner;
  return winner;
}

}  // namespace compass
