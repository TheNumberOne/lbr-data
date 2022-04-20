//
// Created by rose on 4/18/22.
//

#ifndef LBR_A_STAR_HPP
#define LBR_A_STAR_HPP

#include <utility>
#include <vector>
#include <functional>
#include <map>
#include <set>
#include <queue>
#include <optional>

template<typename T>
std::vector<T> reconstruct_path(const std::map<T, T> &came_from, T current){
  std::vector<T> path{current};
  
  while (came_from.contains(current)) {
    current = came_from.at(current);
    path.push_back(current);
  }
  
  std::reverse(path.begin(), path.end());
  
  return path;
}

template<typename T>
std::optional<std::pair<std::vector<T>, double>> a_star(T start, const T &end, std::function<std::vector<std::pair<T, double>>(const T&)> neighbors, std::function<double(const T&, const T&)> heuristic, std::function<double(const T&, const T&)> max_heuristic) {
  auto compare = [](const std::pair<T, double> &a, const std::pair<T, double> &b) {
    return a.second > b.second;
  };
  
  auto queue = std::priority_queue(compare, std::vector<std::pair<T, double>>{});
  queue.emplace(start, heuristic(start, end));
  std::map<T, T> came_from;
  std::map<T, double> scores{{start, 0}};
  std::set<T> checked;
  double best_seen = max_heuristic(start, end);
  
  int i = 0;
  
  while (!queue.empty()) {
    auto [current, current_score] = queue.top();
    queue.pop();
    
    if (i % 10000 == 0) {
      printf("%d: %d left, %.3f-%.3f\n", i, queue.size(), current_score, best_seen);
    }
    i += 1;
    
    auto [_, inserted] = checked.insert(current);
    
    if (!inserted || current_score > best_seen) {
      continue;
    }
    
    if (current == end) {
      return std::make_pair(reconstruct_path(came_from, current), current_score);
    }
    
    for (auto [neighbor, distance] : neighbors(current)) {
      double tentative_score = scores[current] + distance;
      double max_score = tentative_score + max_heuristic(neighbor, end);
      double min_score = tentative_score + heuristic(neighbor, end);
      best_seen = std::min(max_score, best_seen);
      if ((!scores.contains(neighbor) || tentative_score < scores.at(neighbor)) && min_score <= best_seen) {
        came_from[neighbor] = current;
        scores[neighbor] = tentative_score;
        queue.emplace(neighbor, min_score);
      }
    }
  }
  
  return std::nullopt;
}

#endif //LBR_A_STAR_HPP
