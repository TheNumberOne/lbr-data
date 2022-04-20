
#include <cmath>
#include <map>
#include <ranges>
#include <numeric>
#include <iostream>
#include <cassert>
#include "a_star.hpp"

enum Rarity : std::uint8_t
{
  base = 14,
  ancient = 16,
  sacred = 17,
  biotite = 18,
  malachite = 19,
  hematite = 20,
};

const int dark_essence_per_ascension_shard = 100;
const int dark_essence_per_fusion_shard = 50;
const int regular_essence_per_dark_essence = 6;
const double hours_per_witch = 144. / 60 / 60;
const double base_crit_rate = 0.3463;
const int levels = 30;
const int total_quality = 200;
const int wem_shards = 10;
const int crit_shards = 7;
const std::uint8_t max_ascend_level = 10;
const double base_essence_per_witch = 17.9;
const int leaves_per_set = 8;

double get_base_wem_bonus()
{
  static const double base_wem_bonus = std::pow(10, -8.5);
  
  return base_wem_bonus;
}

double get_base_crit_bonus()
{
  static const double base_crit_bonus = std::pow(10, -10);
  
  return base_crit_bonus;
}

const std::map<Rarity, int> &get_fusion_costs()
{
  static const std::map<Rarity, int> fusion_costs{
    {ancient,   0},
    {sacred,    5},
    {biotite,   7},
    {malachite, 11},
    {hematite,  19}
  };
  
  return fusion_costs;
}

std::map<Rarity, int> calculate_total_fusion_costs()
{
  const auto &fusion_costs = get_fusion_costs();
  
  std::map<Rarity, int> result{{ancient, 0}};
  
  for (auto rarity: {sacred, biotite, malachite, hematite}) {
    result[rarity] = result[static_cast<Rarity>(rarity - 1)] * 6 + fusion_costs.at(rarity);
  }
  
  return result;
}

const std::map<Rarity, int> &get_total_fusion_costs()
{
  static const std::map<Rarity, int> total_fusion_costs = calculate_total_fusion_costs();
  
  return total_fusion_costs;
}

int ascension_single_level_cost(Rarity rarity, int next_ascend_level)
{
  if (rarity <= ancient) {
    return 0;
  }
  
  return next_ascend_level * (2 + (int) std::ceil(std::pow(1.5, rarity - base)));
}

struct Leaf
{
  Rarity rarity = hematite;
  std::uint8_t ascend_level = max_ascend_level;
  
  auto operator<=>(const Leaf &) const = default;
};

std::map<Leaf, int> calculate_ascension_costs()
{
  std::map<Leaf, int> result;
  
  for (auto rarity: {ancient, sacred, biotite, malachite, hematite}) {
    int cost = 0;
    result[{rarity, 0}] = 0;
    
    for (std::uint8_t i = 1; i <= max_ascend_level; ++i) {
      cost += ascension_single_level_cost(rarity, i);
      result[{rarity, i}] = cost;
    }
  }
  
  return result;
}

const std::map<Leaf, int> &get_ascension_costs()
{
  static const std::map<Leaf, int> ascension_costs = calculate_ascension_costs();
  
  return ascension_costs;
}

double property_bonus(Leaf leaf, double base_property_bonus, int shards)
{
  int ascend_level = leaf.ascend_level;
  if (ascend_level == 0) {
    ascend_level = -1;
  }
  
  return base_property_bonus *
         (((ascend_level + 1) * 60 + levels) / 5.0 + 1) *
         std::pow((leaf.rarity - base) * 128, 1.6) *
         (1 + shards * 3) *
         total_quality / 20;
}

std::map<Leaf, double> calculate_property_bonuses(double base_property_bonus, int shards)
{
  std::map<Leaf, double> result;
  
  for (auto rarity: {ancient, sacred, biotite, malachite, hematite}) {
    for (std::uint8_t i = 0; i <= max_ascend_level; ++i) {
      Leaf leaf{rarity, i};
      result[leaf] = property_bonus(leaf, base_property_bonus, shards);
    }
  }
  
  return result;
}

const std::map<Leaf, double> &get_wem_bonuses()
{
  static const std::map<Leaf, double> wem_bonuses = calculate_property_bonuses(get_base_wem_bonus(), wem_shards);
  
  return wem_bonuses;
}

const std::map<Leaf, double> &get_crit_bonuses()
{
  static const std::map<Leaf, double> crit_bonuses = calculate_property_bonuses(get_base_crit_bonus(), crit_shards);
  
  return crit_bonuses;
}

int dark_essence_cost(Leaf old_leaf, Leaf new_leaf)
{
  const auto &fuse_costs = get_total_fusion_costs();
  const auto &ascension_costs = get_ascension_costs();
  
  int fusion_shards = fuse_costs.at(new_leaf.rarity) - fuse_costs.at(old_leaf.rarity);
  
  int ascension_shards =
    ascension_costs.at(new_leaf) - (new_leaf.rarity == old_leaf.rarity ? ascension_costs.at(old_leaf) : 0);
  
  return dark_essence_per_ascension_shard * ascension_shards + dark_essence_per_fusion_shard * fusion_shards;
}

typedef std::array<Leaf, leaves_per_set> Leaves;

Leaves full_set_of(Leaf leaf)
{
  Leaves result;
  result.fill(leaf);
  return result;
}

double leaves_factor(const Leaves &leaves)
{
  const auto &wem_bonuses = get_wem_bonuses();
  const auto &crit_bonuses = get_crit_bonuses();
  
  double essence_per_witch = base_essence_per_witch * std::accumulate(
    leaves.begin(), leaves.end(), 1,
    [&wem_bonuses](auto accum, auto leaf)
    {
      return accum + wem_bonuses.at(leaf);
    }
  );
  
  double crit_rate = std::accumulate(
    leaves.begin(), leaves.end(), base_crit_rate,
    [&crit_bonuses](auto accum, auto leaf)
    {
      return accum + crit_bonuses.at(leaf);
    }
  );
  
  return essence_per_witch * std::pow(1 + crit_rate, 2);
}

double time_between(Leaf begin, Leaf end, double leaves_factor)
{
  return dark_essence_cost(begin, end) * regular_essence_per_dark_essence / leaves_factor * hours_per_witch;
}

double time_between(const Leaves &begin, const Leaves &end, const Leaves &through)
{
  double time = 0;
  double lf = leaves_factor(through);
  
  for (int i = 0; i < leaves_per_set; ++i) {
    time += time_between(begin[i], end[i], lf);
  }
  
  return time;
}

int smallest_ascend_level_upgrade(Leaf begin, Rarity new_rarity)
{
  assert(begin.rarity <= new_rarity);
  
  static std::map<std::pair<Leaf, Rarity>, int> cache;
  
  if (begin.rarity == new_rarity) {
    return begin.ascend_level + 1;
  }
  
  if (cache.contains({begin, new_rarity})) {
    return cache.at({begin, new_rarity});
  }
  
  for (std::uint8_t i = 0; i <= max_ascend_level; i++) {
    if (get_wem_bonuses().at({new_rarity, i}) > get_wem_bonuses().at(begin)) {
      cache[{begin, new_rarity}] = i;
      return i;
    }
  }
  
  assert(false);
}

double upper_bound_time_between(Leaves begin, const Leaves &end)
{
  double time = 0;
  for (int i = 0; i < leaves_per_set; i++) {
    if (begin[i] == end[i]) {
      continue;
    }
    std::uint8_t ascend_level = smallest_ascend_level_upgrade(begin[i], end[i].rarity);
    time += time_between(begin[i], {end[i].rarity, ascend_level}, leaves_factor(begin));
    begin[i] = {end[i].rarity, ascend_level};
    
    for (std::uint8_t j = ascend_level + 1; j <= end[i].ascend_level; j++) {
      time += time_between(begin[i], {end[i].rarity, j}, leaves_factor(begin));
      begin[i].ascend_level = j;
    }
  }
  
  return time;
}

std::vector<std::pair<Leaves, double>> before_neighbors(const Leaves &leaves, Leaf smallest_allowed)
{
  std::vector<std::pair<Leaves, double>> result;
  double old_lf = leaves_factor(leaves);
  for (size_t i = 0; i < leaves_per_set; i++) {
    Leaf leaf = leaves[i];
    if (i > 0 && leaves[i - 1] == leaf) {
      continue;
    }
    
    if (leaf != smallest_allowed && leaf.ascend_level != 0) {
      Leaves neighbor = leaves;
      neighbor[i].ascend_level--;
      result.emplace_back(neighbor, time_between(neighbor[i], leaf, leaves_factor(neighbor)));
    }
    
    for (Rarity new_rarity = smallest_allowed.rarity;
         new_rarity < leaf.rarity;
         new_rarity = static_cast<Rarity>(new_rarity + 1)) {
      for (std::uint8_t new_ascend_level = 0; new_ascend_level <= max_ascend_level; new_ascend_level++) {
        Leaf new_leaf{new_rarity, new_ascend_level};
        if (new_leaf < smallest_allowed) {
          continue;
        }
        Leaves neighbor = leaves;
        neighbor[i] = new_leaf;
        double lf = leaves_factor(neighbor);
        if (old_lf <= lf) {
          break;
        }
        std::sort(neighbor.begin(), neighbor.end());
        result.emplace_back(neighbor, time_between(new_leaf, leaf, lf));
      }
    }
  }
  
  return result;
}

std::vector<std::pair<Leaves, double>> after_neighbors(const Leaves &leaves, Leaf largest_allowed)
{
  std::vector<std::pair<Leaves, double>> result;
  double old_lf = leaves_factor(leaves);
  for (size_t i = 0; i < leaves_per_set; i++) {
    Leaf leaf = leaves[i];
    if (i > 0 && leaves[i - 1].rarity == leaf.rarity) {
      continue;
    }
    
    if (leaf != largest_allowed && leaf.ascend_level != max_ascend_level) {
      Leaves neighbor = leaves;
      neighbor[i].ascend_level++;
      std::sort(neighbor.begin(), neighbor.end());
      result.emplace_back(neighbor, time_between(leaf, {leaf.rarity, static_cast<uint8_t>(leaf.ascend_level + 1)}, old_lf));
    }
    
    for (auto new_rarity = static_cast<Rarity>(leaf.rarity + 1);
         new_rarity <= largest_allowed.rarity;
         new_rarity = static_cast<Rarity>(new_rarity + 1)) {
  
      std::uint8_t new_ascend_level = smallest_ascend_level_upgrade(leaf, new_rarity);
      Leaf new_leaf{new_rarity, new_ascend_level};
      if (new_leaf > largest_allowed) {
        continue;
      }
      Leaves neighbor = leaves;
      neighbor[i] = new_leaf;
      std::sort(neighbor.begin(), neighbor.end());
      result.emplace_back(neighbor, time_between(leaf, new_leaf, old_lf));
    }
  }
  
  return result;
}

double min_heuristic(const Leaves &start, const Leaves &end)
{
  return time_between(start, end, end);
}

double max_heuristic(const Leaves &start, const Leaves &end)
{
  return time_between(start, end, start);
}

double reversed_min_heuristic(const Leaves &end, const Leaves &start)
{
  return time_between(start, end, end);
}

double reversed_max_heuristic(const Leaves &end, const Leaves &start)
{
  return time_between(start, end, start);
}

std::string leaves_to_str(const Leaves &leaves)
{
  std::string result;
  bool first = true;
  for (auto leaf: leaves) {
    if (first) {
      first = false;
    } else {
      result += ' ';
    }
    
    auto [rarity, ascend_level] = leaf;
    switch (rarity) {
      case ancient:
        result += 'a';
        break;
      case sacred:
        result += 's';
        break;
      case biotite:
        result += 'b';
        break;
      case malachite:
        result += 'm';
        break;
      case hematite:
        result += 'h';
        break;
      default:
        result += '?';
        break;
    }
    result += std::to_string(ascend_level);
  }
  
  return result;
}

int main()
{
  printf("%.3d\n", get_ascension_costs().at({hematite}));
  printf("%.3d\n", dark_essence_cost({ancient}, {hematite}) * regular_essence_per_dark_essence);
  printf("%.3d\n", dark_essence_cost({ancient}, {hematite}));
  printf("%.3d\n", get_total_fusion_costs().at(hematite));
  
  Leaves begin = full_set_of({ancient});
  Leaves end = full_set_of({hematite});
  printf("%.3f\n", time_between(begin, end, begin));
  printf("%.3f\n", leaves_factor(begin));
  printf("%.3g\n", property_bonus({ancient}, get_base_wem_bonus(), wem_shards));
  printf("%.3g\n", get_base_wem_bonus());
  
  auto result = a_star<int>(
    1,
    10,
    [](int a) { return std::vector{std::make_pair(a + 1, 1.0)}; },
    [](int a, int b) { return (double) (b - a); },
    [](int a, int b) { return 0.0; }
  );
  if (result) {
    auto [path, cost] = *result;
    printf("%.3f\n", cost);
  }
  
  Leaf smallest{ancient};
  begin = full_set_of(smallest);
  
  for (const auto &leaves: before_neighbors(end, smallest)) {
    std::cout << leaves_to_str(leaves.first) << ", " << leaves.second << '\n';
  }
  
  std::cout << "after\n";
  
  for (const auto &leaves: after_neighbors(begin, {hematite})) {
    std::cout << leaves_to_str(leaves.first) << ", " << leaves.second << '\n';
  }
  
  auto solution = a_star<Leaves>(
    begin,
    end,
    [](const auto &leaves) { return after_neighbors(leaves, {hematite}); },
    min_heuristic,
    max_heuristic
  );
  if (solution) {
    auto [path, hours] = *std::move(solution);
    printf("takes %.3f hours\n", hours);
    for (const auto &leaves: path) {
      std::cout << leaves_to_str(leaves) << '\n';
    }
  }
  
  return 0;
}