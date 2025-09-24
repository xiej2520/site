// https://arxiv.org/pdf/cs/0011047.pdf
// https://garethrees.org/2007/06/10/zendoku-generation/#section-4.2
// DLX 10ms vs Min Branch 57ms on https://lightoj.com/problem/sudoku-solver
#include <array>
#include <emscripten.h>
#include <cstdint>
#include <stdint.h>
#include <vector>

using namespace std;

using INT_T = int16_t;
struct Node {
  INT_T up;
  INT_T down;
  INT_T left;
  INT_T right;
  INT_T col;
};

struct Column {
  INT_T count = 0;
};

struct DLX {
  vector<Column> columns;
  vector<Node> nodes;

  DLX(INT_T n) {
    nodes.reserve(1 + 9 * 9 * 4 + (9 * 9 * 9) * 4);
    columns.reserve(n + 1);
    nodes.push_back(Node{0, 0, n, 1, 0}); // root
    columns.push_back(Column{0});         // root
    for (INT_T i = 0; i < n; i++) {
      // 1st column node at index 1, left is 0, right is 2, column i -> index i
      nodes.push_back(Node{static_cast<INT_T>(i + 1), static_cast<INT_T>(i + 1),
                           static_cast<INT_T>(i), static_cast<INT_T>(i + 2),
                           static_cast<INT_T>(i + 1)});
      columns.push_back(Column{0});
    }
    nodes.back().right = 0; // link back to root
  }
  void add_row(INT_T c1, INT_T c2, INT_T c3, INT_T c4) {
    c1++, c2++, c3++, c4++; // increment by one for root
    INT_T i1 = nodes.size();
    INT_T i2 = i1 + 1;
    INT_T i3 = i2 + 1;
    INT_T i4 = i3 + 1;
    nodes.push_back({Node{nodes[c1].up, c1, i4, i2, c1}});
    nodes[nodes[c1].up].down = i1;
    nodes[c1].up = i1;
    columns[c1].count++;

    nodes.push_back({Node{nodes[c2].up, c2, i1, i3, c2}});
    nodes[nodes[c2].up].down = i2;
    nodes[c2].up = i2;
    columns[c2].count++;

    nodes.push_back({Node{nodes[c3].up, c3, i2, i4, c3}});
    nodes[nodes[c3].up].down = i3;
    nodes[c3].up = i3;
    columns[c3].count++;

    nodes.push_back({Node{nodes[c4].up, c4, i3, i1, c4}});
    nodes[nodes[c4].up].down = i4;
    nodes[c4].up = i4;
    columns[c4].count++;
  }
};

DLX dlx{81 * 4};
vector<INT_T> O;

#define U(x) dlx.nodes[x].up
#define D(x) dlx.nodes[x].down
#define L(x) dlx.nodes[x].left
#define R(x) dlx.nodes[x].right
#define C(x) dlx.columns[dlx.nodes[x].col]
#define foreach(i, x, F) for (INT_T i = F(x); i != x; i = F(i))

void cover(INT_T col) {
  // printf("covering col %d\n",col);
  L(R(col)) = L(col);
  R(L(col)) = R(col);
  foreach (i, col, D) {
    foreach (j, i, R) {
      U(D(j)) = U(j);
      D(U(j)) = D(j);
      C(j).count--;
      // printf("%d\n",j);
    }
  }
  // printf("done covering col %d\n",col);
};
void uncover(INT_T col) {
  // printf("uncovering col %d\n",col);
  foreach (i, col, U) {
    foreach (j, i, L) {
      C(j).count++;
      U(D(j)) = j;
      D(U(j)) = j;
    }
  }
  L(R(col)) = col;
  R(L(col)) = col;
  // printf("done uncovering col %d\n",col);
};

bool dfs() {
  // Knuth
  if (R(0) == 0)
    return true;

  // choose column
  INT_T c = R(0);
  INT_T s = C(c).count;
  foreach (j, 0, R) {
    if (C(j).count < s) {
      c = j;
      s = C(j).count;
    }
  }
  cover(c);
  foreach (r, c, D) {
    foreach (j, r, R) {
      cover(dlx.nodes[j].col);
    }
    if (dfs()) {
      O.push_back(r);
      return true;
    }
    foreach (j, r, L) {
      uncover(dlx.nodes[j].col);
    }
  }
  uncover(c);
  return false;
}

extern "C" {

// input is 9 lines, '.' for empty space
EMSCRIPTEN_KEEPALIVE
char *solve(char *input) {
  for (INT_T r = 0; r < 9; r++) {
    for (INT_T c = 0; c < 9; c++) {
      for (INT_T d = 1; d <= 9; d++) {
        INT_T d_offset = 81 + 27 * (d - 1);
        dlx.add_row(9 * r + c, d_offset + r, d_offset + 9 + c,
                    d_offset + 18 + r / 3 * 3 + c / 3);
      }
    }
  }
  O.reserve(81);

  array<array<char, 9>, 9> board{};

  for (INT_T i = 0; i < 9; i++) {
    for (INT_T j = 0; j < 9; j++) {
      char c = input[10 * i + j];
      if (c == '.') {
        board[i][j] = '.';
        continue;
      }
      board[i][j] = c;

      INT_T col = 1 + 9 * i + j;
      cover(col);
      foreach (r, col, D) {
        INT_T d = (dlx.nodes[R(r)].col - 81 - 1) / 27;
        if (d + 1 == c - '0') {
          O.push_back(r);
          foreach (j, r, R) {
            cover(dlx.nodes[j].col);
          }
          break;
        }
      }
    }
  }
  dfs();

  for (INT_T i = 0; i < 81; i++) {
    while (dlx.nodes[O[i]].col > 81) {
      O[i] = dlx.nodes[O[i]].left;
    }
    INT_T col = dlx.nodes[O[i]].col - 1; // root
    INT_T r = col / 9;
    INT_T c = col % 9;
    INT_T d = (dlx.nodes[dlx.nodes[O[i]].right].col - 81 - 1) / 27;
    board[r][c] = d + 1 + '0';
  }

  char *res = new char[9 * 10 + 1]();
  for (INT_T i = 0; i < 9; i++) {
    for (INT_T j = 0; j < 9; j++) {
      res[10 * i + j] = board[i][j];
    }
    res[10 * i + 9] = '\n';
  }
  return res;
}
}

int main() { }
