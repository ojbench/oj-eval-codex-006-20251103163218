#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <utility>
#include <vector>
#include <queue>
#include <cstring>
#include <random>
#include <algorithm>

extern int rows;
extern int columns;
extern int total_mines;

// Global variables for client
namespace client_ns {
  constexpr int MAXN = 35;
  char map[MAXN][MAXN];
  int unknown_count[MAXN][MAXN];
  int marked_count[MAXN][MAXN];
  std::mt19937 rng;
}

void Execute(int r, int c, int type);

void InitGame() {
  memset(client_ns::map, '?', sizeof(client_ns::map));
  memset(client_ns::unknown_count, 0, sizeof(client_ns::unknown_count));
  memset(client_ns::marked_count, 0, sizeof(client_ns::marked_count));
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
}

void Decide() {
  using namespace client_ns;
  
  // Phase 1: Mark certain mines and auto-explore
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < columns; j++) {
      if (map[i][j] >= '0' && map[i][j] <= '8') {
        int mine_count = map[i][j] - '0';
        int remaining = mine_count - marked_count[i][j];
        
        // All unknowns must be mines
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
        
        // All mines marked, auto-explore
        if (marked_count[i][j] == mine_count && unknown_count[i][j] > 0) {
          Execute(i, j, 2);
          return;
        }
      }
    }
  }
  
  // Phase 2: Advanced reasoning
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < columns; j++) {
      if (map[i][j] < '0' || map[i][j] > '8') continue;
      
      int mc1 = map[i][j] - '0';
      int rem1 = mc1 - marked_count[i][j];
      if (rem1 == 0 || unknown_count[i][j] == 0) continue;
      
      std::vector<std::pair<int,int>> unk1;
      for (int di = -1; di <= 1; di++) {
        for (int dj = -1; dj <= 1; dj++) {
          if (di == 0 && dj == 0) continue;
          int ni = i + di, nj = j + dj;
          if (ni >= 0 && ni < rows && nj >= 0 && nj < columns && map[ni][nj] == '?') {
            unk1.push_back({ni, nj});
          }
        }
      }
      
      // Check adjacent cells
      for (int ii = i - 2; ii <= i + 2; ii++) {
        for (int jj = j - 2; jj <= j + 2; jj++) {
          if (ii < 0 || ii >= rows || jj < 0 || jj >= columns) continue;
          if (ii == i && jj == j) continue;
          if (map[ii][jj] < '0' || map[ii][jj] > '8') continue;
          
          int mc2 = map[ii][jj] - '0';
          int rem2 = mc2 - marked_count[ii][jj];
          
          std::vector<std::pair<int,int>> unk2;
          for (int di = -1; di <= 1; di++) {
            for (int dj = -1; dj <= 1; dj++) {
              if (di == 0 && dj == 0) continue;
              int ni = ii + di, nj = jj + dj;
              if (ni >= 0 && ni < rows && nj >= 0 && nj < columns && map[ni][nj] == '?') {
                unk2.push_back({ni, nj});
              }
            }
          }
          
          // Find difference
          std::vector<std::pair<int,int>> diff;
          for (auto& p : unk2) {
            if (std::find(unk1.begin(), unk1.end(), p) == unk1.end()) {
              diff.push_back(p);
            }
          }
          
          if (diff.empty()) continue;
          
          // Check if unk1 is subset of unk2
          bool subset = true;
          for (auto& p : unk1) {
            if (std::find(unk2.begin(), unk2.end(), p) == unk2.end()) {
              subset = false;
              break;
            }
          }
          
          if (!subset) continue;
          
          // If same mine count, diff is safe
          if (rem2 == rem1) {
            Execute(diff[0].first, diff[0].second, 0);
            return;
          }
          
          // If diff size matches remaining, diff is all mines
          if (rem2 == rem1 + (int)diff.size()) {
            Execute(diff[0].first, diff[0].second, 1);
            return;
          }
        }
      }
    }
  }
  
  // Phase 3: Pick safest cell (prefer cells with more revealed neighbors)
  int best_score = -1;
  std::vector<std::pair<int,int>> candidates;
  
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < columns; j++) {
      if (map[i][j] != '?') continue;
      
      int score = 0;
      for (int di = -1; di <= 1; di++) {
        for (int dj = -1; dj <= 1; dj++) {
          if (di == 0 && dj == 0) continue;
          int ni = i + di, nj = j + dj;
          if (ni >= 0 && ni < rows && nj >= 0 && nj < columns) {
            if (map[ni][nj] >= '0' && map[ni][nj] <= '8') score++;
          }
        }
      }
      
      if (score > best_score) {
        best_score = score;
        candidates.clear();
        candidates.push_back({i, j});
      } else if (score == best_score) {
        candidates.push_back({i, j});
      }
    }
  }
  
  if (!candidates.empty()) {
    std::uniform_int_distribution<> dist(0, candidates.size() - 1);
    auto cell = candidates[dist(rng)];
    Execute(cell.first, cell.second, 0);
    return;
  }
  
  // Fallback: random unknown
  std::vector<std::pair<int,int>> all_unknown;
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < columns; j++) {
      if (map[i][j] == '?') all_unknown.push_back({i, j});
    }
  }
  
  if (!all_unknown.empty()) {
    std::uniform_int_distribution<> dist(0, all_unknown.size() - 1);
    auto cell = all_unknown[dist(rng)];
    Execute(cell.first, cell.second, 0);
  }
}

#endif
