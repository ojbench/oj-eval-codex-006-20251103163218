#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <utility>
#include <vector>
#include <queue>
#include <cstring>
#include <random>
#include <algorithm>
#include <set>

extern int rows;
extern int columns;
extern int total_mines;

namespace client_ns {
  constexpr int MAXN = 35;
  char map[MAXN][MAXN];
  int unknown_count[MAXN][MAXN];
  int marked_count[MAXN][MAXN];
  std::mt19937 rng;
  
  // Track probability estimates
  double mine_prob[MAXN][MAXN];
}

void Execute(int r, int c, int type);

void InitGame() {
  memset(client_ns::map, '?', sizeof(client_ns::map));
  memset(client_ns::unknown_count, 0, sizeof(client_ns::unknown_count));
  memset(client_ns::marked_count, 0, sizeof(client_ns::marked_count));
  for (int i = 0; i < client_ns::MAXN; i++) {
    for (int j = 0; j < client_ns::MAXN; j++) {
      client_ns::mine_prob[i][j] = 0.5;
    }
  }
  client_ns::rng.seed(std::random_device{}());
  
  int first_row, first_column;
  std::cin >> first_row >> first_column;
  Execute(first_row, first_column, 0);
}

void ReadMap() {
  for (int i = 0; i < rows; i++) {
    std::string line;
    std::cin >> line;
    for (int j = 0; j < columns; j++) {
      client_ns::map[i][j] = line[j];
    }
  }
  
  // Update neighbor counts
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < columns; j++) {
      client_ns::unknown_count[i][j] = 0;
      client_ns::marked_count[i][j] = 0;
      
      if (client_ns::map[i][j] >= '0' && client_ns::map[i][j] <= '8') {
        for (int di = -1; di <= 1; di++) {
          for (int dj = -1; dj <= 1; dj++) {
            if (di == 0 && dj == 0) continue;
            int ni = i + di, nj = j + dj;
            if (ni >= 0 && ni < rows && nj >= 0 && nj < columns) {
              if (client_ns::map[ni][nj] == '?') {
                client_ns::unknown_count[i][j]++;
              } else if (client_ns::map[ni][nj] == '@') {
                client_ns::marked_count[i][j]++;
              }
            }
          }
        }
      }
    }
  }
  
  // Update probabilities
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < columns; j++) {
      client_ns::mine_prob[i][j] = 0.5;
    }
  }
  
  // Calculate mine probabilities from constraints
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < columns; j++) {
      if (client_ns::map[i][j] >= '0' && client_ns::map[i][j] <= '8') {
        int mc = client_ns::map[i][j] - '0';
        int rem = mc - client_ns::marked_count[i][j];
        int unk = client_ns::unknown_count[i][j];
        
        if (unk > 0) {
          double prob = (double)rem / unk;
          for (int di = -1; di <= 1; di++) {
            for (int dj = -1; dj <= 1; dj++) {
              if (di == 0 && dj == 0) continue;
              int ni = i + di, nj = j + dj;
              if (ni >= 0 && ni < rows && nj >= 0 && nj < columns) {
                if (client_ns::map[ni][nj] == '?') {
                  client_ns::mine_prob[ni][nj] = std::max(client_ns::mine_prob[ni][nj], prob);
                }
              }
            }
          }
        }
      }
    }
  }
}

void Decide() {
  using namespace client_ns;
  
  // Phase 1: Mark certain mines and auto-explore
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < columns; j++) {
      if (map[i][j] >= '0' && map[i][j] <= '8') {
        int mine_count = map[i][j] - '0';
        int remaining = mine_count - marked_count[i][j];
        
        if (remaining > 0 && remaining == unknown_count[i][j]) {
          for (int di = -1; di <= 1; di++) {
            for (int dj = -1; dj <= 1; dj++) {
              if (di == 0 && dj == 0) continue;
              int ni = i + di, nj = j + dj;
              if (ni >= 0 && ni < rows && nj >= 0 && nj < columns && map[ni][nj] == '?') {
                Execute(ni, nj, 1);
                return;
              }
            }
          }
        }
        
        if (marked_count[i][j] == mine_count && unknown_count[i][j] > 0) {
          Execute(i, j, 2);
          return;
        }
      }
    }
  }
  
  // Phase 2: Advanced constraint satisfaction
  std::vector<std::vector<std::pair<int,int>>> constraints;
  std::vector<int> constraint_counts;
  
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < columns; j++) {
      if (map[i][j] < '0' || map[i][j] > '8') continue;
      
      int mc = map[i][j] - '0';
      int rem = mc - marked_count[i][j];
      if (rem == 0 || unknown_count[i][j] == 0) continue;
      
      std::vector<std::pair<int,int>> unknowns;
      for (int di = -1; di <= 1; di++) {
        for (int dj = -1; dj <= 1; dj++) {
          if (di == 0 && dj == 0) continue;
          int ni = i + di, nj = j + dj;
          if (ni >= 0 && ni < rows && nj >= 0 && nj < columns && map[ni][nj] == '?') {
            unknowns.push_back({ni, nj});
          }
        }
      }
      
      constraints.push_back(unknowns);
      constraint_counts.push_back(rem);
    }
  }
  
  // Try to find safe cells via constraint satisfaction
  for (size_t idx1 = 0; idx1 < constraints.size(); idx1++) {
    for (size_t idx2 = idx1 + 1; idx2 < constraints.size(); idx2++) {
      auto& c1 = constraints[idx1];
      auto& c2 = constraints[idx2];
      int cnt1 = constraint_counts[idx1];
      int cnt2 = constraint_counts[idx2];
      
      // Find intersection and differences
      std::vector<std::pair<int,int>> diff1, diff2, inter;
      
      for (auto& p : c1) {
        if (std::find(c2.begin(), c2.end(), p) != c2.end()) {
          inter.push_back(p);
        } else {
          diff1.push_back(p);
        }
      }
      
      for (auto& p : c2) {
        if (std::find(c1.begin(), c1.end(), p) == c1.end()) {
          diff2.push_back(p);
        }
      }
      
      // Apply subset reasoning
      if (diff1.empty() && !diff2.empty()) {
        // c1 is subset of c2
        if (cnt2 == cnt1) {
          // diff2 is safe
          Execute(diff2[0].first, diff2[0].second, 0);
          return;
        }
        if (cnt2 == cnt1 + (int)diff2.size()) {
          // diff2 is all mines
          Execute(diff2[0].first, diff2[0].second, 1);
          return;
        }
      }
      
      if (diff2.empty() && !diff1.empty()) {
        // c2 is subset of c1
        if (cnt1 == cnt2) {
          // diff1 is safe
          Execute(diff1[0].first, diff1[0].second, 0);
          return;
        }
        if (cnt1 == cnt2 + (int)diff1.size()) {
          // diff1 is all mines
          Execute(diff1[0].first, diff1[0].second, 1);
          return;
        }
      }
    }
  }
  
  // Phase 3: Pick the cell with lowest mine probability
  double best_prob = 2.0;
  std::vector<std::pair<int,int>> best_cells;
  
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < columns; j++) {
      if (map[i][j] != '?') continue;
      
      double prob = mine_prob[i][j];
      
      // Prefer cells with more revealed neighbors
      int revealed = 0;
      for (int di = -1; di <= 1; di++) {
        for (int dj = -1; dj <= 1; dj++) {
          if (di == 0 && dj == 0) continue;
          int ni = i + di, nj = j + dj;
          if (ni >= 0 && ni < rows && nj >= 0 && nj < columns) {
            if (map[ni][nj] >= '0' && map[ni][nj] <= '8') revealed++;
          }
        }
      }
      
      // Bonus for having more revealed neighbors (lower effective prob)
      double eff_prob = prob - revealed * 0.01;
      
      if (eff_prob < best_prob) {
        best_prob = eff_prob;
        best_cells.clear();
        best_cells.push_back({i, j});
      } else if (std::abs(eff_prob - best_prob) < 0.001) {
        best_cells.push_back({i, j});
      }
    }
  }
  
  if (!best_cells.empty()) {
    std::uniform_int_distribution<> dist(0, best_cells.size() - 1);
    auto cell = best_cells[dist(rng)];
    Execute(cell.first, cell.second, 0);
    return;
  }
  
  // Fallback
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < columns; j++) {
      if (map[i][j] == '?') {
        Execute(i, j, 0);
        return;
      }
    }
  }
}

#endif
