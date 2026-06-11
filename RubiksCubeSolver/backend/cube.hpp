#ifndef RUBIK_CUBE_HPP
#define RUBIK_CUBE_HPP

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace rubik {

const std::string SOLVED_STATE =
    "WWWWWWWWW"
    "RRRRRRRRR"
    "GGGGGGGGG"
    "YYYYYYYYY"
    "OOOOOOOOO"
    "BBBBBBBBB";

const std::array<char, 6> FACE_COLORS = {'W', 'R', 'G', 'Y', 'O', 'B'};
const std::array<int, 6> CENTER_INDEXES = {4, 13, 22, 31, 40, 49};
const std::vector<std::string> MOVES = {
    "U", "U'", "U2", "R", "R'", "R2", "F", "F'", "F2",
    "D", "D'", "D2", "L", "L'", "L2", "B", "B'", "B2"};

struct Vec3 {
    int x;
    int y;
    int z;
};

struct Sticker {
    Vec3 pos;
    Vec3 normal;
    char color;
};

struct MoveSpec {
    char face;
    char axis;
    int layer;
    int sign;
    int turns;
};

struct ValidationResult {
    bool valid;
    std::string message;
};

struct SearchOptions {
    int maxDepth = 7;
    std::size_t maxNodes = 120000;
};

struct SolveResult {
    bool solved = false;
    std::string algorithm;
    std::vector<std::string> moves;
    std::size_t nodesExpanded = 0;
    double timeMs = 0.0;
    int depthReached = 0;
    std::string message;
};

struct ComplexityInfo {
    std::string time;
    std::string space;
};

struct ComparisonRow {
    std::string name;
    std::string timeComplexity;
    std::string spaceComplexity;
    std::size_t nodes;
    double timeMs;
    int moves;
};

inline bool isSolved(const std::string& state) {
    return state == SOLVED_STATE;
}

inline bool isKnownMove(const std::string& move) {
    return std::find(MOVES.begin(), MOVES.end(), move) != MOVES.end();
}

inline bool isSameFace(const std::string& a, const std::string& b) {
    return !a.empty() && !b.empty() && a[0] == b[0];
}

inline Vec3 rotateQuarter(Vec3 v, char axis, int sign) {
    if (axis == 'x') {
        return sign > 0 ? Vec3{v.x, -v.z, v.y} : Vec3{v.x, v.z, -v.y};
    }
    if (axis == 'y') {
        return sign > 0 ? Vec3{v.z, v.y, -v.x} : Vec3{-v.z, v.y, v.x};
    }
    return sign > 0 ? Vec3{-v.y, v.x, v.z} : Vec3{v.y, -v.x, v.z};
}

inline MoveSpec parseMove(const std::string& move) {
    if (move.empty()) {
        return {'?', '?', 0, 1, 0};
    }

    MoveSpec spec{move[0], '?', 0, 1, 1};
    switch (move[0]) {
        case 'U':
            spec.axis = 'y';
            spec.layer = 1;
            spec.sign = -1;
            break;
        case 'D':
            spec.axis = 'y';
            spec.layer = -1;
            spec.sign = 1;
            break;
        case 'R':
            spec.axis = 'x';
            spec.layer = 1;
            spec.sign = -1;
            break;
        case 'L':
            spec.axis = 'x';
            spec.layer = -1;
            spec.sign = 1;
            break;
        case 'F':
            spec.axis = 'z';
            spec.layer = 1;
            spec.sign = -1;
            break;
        case 'B':
            spec.axis = 'z';
            spec.layer = -1;
            spec.sign = 1;
            break;
        default:
            return {'?', '?', 0, 1, 0};
    }

    if (move.size() > 1) {
        if (move[1] == '\'') {
            spec.sign *= -1;
        } else if (move[1] == '2') {
            spec.turns = 2;
        }
    }
    return spec;
}

inline int layerCoordinate(const Vec3& pos, char axis) {
    if (axis == 'x') return pos.x;
    if (axis == 'y') return pos.y;
    return pos.z;
}

inline Sticker indexToSticker(int index, char color) {
    int face = index / 9;
    int local = index % 9;
    int r = local / 3;
    int c = local % 3;

    switch (face) {
        case 0:  // Up
            return {{c - 1, 1, r - 1}, {0, 1, 0}, color};
        case 1:  // Right
            return {{1, 1 - r, 1 - c}, {1, 0, 0}, color};
        case 2:  // Front
            return {{c - 1, 1 - r, 1}, {0, 0, 1}, color};
        case 3:  // Down
            return {{c - 1, -1, 1 - r}, {0, -1, 0}, color};
        case 4:  // Left
            return {{-1, 1 - r, c - 1}, {-1, 0, 0}, color};
        default:  // Back
            return {{1 - c, 1 - r, -1}, {0, 0, -1}, color};
    }
}

inline int stickerToIndex(const Sticker& sticker) {
    const Vec3& p = sticker.pos;
    const Vec3& n = sticker.normal;
    int face = 0;
    int r = 0;
    int c = 0;

    if (n.y == 1) {
        face = 0;
        r = p.z + 1;
        c = p.x + 1;
    } else if (n.x == 1) {
        face = 1;
        r = 1 - p.y;
        c = 1 - p.z;
    } else if (n.z == 1) {
        face = 2;
        r = 1 - p.y;
        c = p.x + 1;
    } else if (n.y == -1) {
        face = 3;
        r = 1 - p.z;
        c = p.x + 1;
    } else if (n.x == -1) {
        face = 4;
        r = 1 - p.y;
        c = p.z + 1;
    } else {
        face = 5;
        r = 1 - p.y;
        c = 1 - p.x;
    }

    return face * 9 + r * 3 + c;
}

inline std::vector<Sticker> stateToStickers(const std::string& state) {
    std::vector<Sticker> stickers;
    stickers.reserve(54);
    for (int i = 0; i < 54; ++i) {
        stickers.push_back(indexToSticker(i, state[i]));
    }
    return stickers;
}

inline std::string stickersToState(const std::vector<Sticker>& stickers) {
    std::string state(54, '?');
    for (const Sticker& sticker : stickers) {
        int index = stickerToIndex(sticker);
        if (index >= 0 && index < 54) {
            state[index] = sticker.color;
        }
    }
    return state;
}

inline std::string applyMove(const std::string& state, const std::string& move) {
    if (state.size() != 54 || !isKnownMove(move)) {
        return state;
    }

    std::vector<Sticker> stickers = stateToStickers(state);
    MoveSpec spec = parseMove(move);
    for (int t = 0; t < spec.turns; ++t) {
        for (Sticker& sticker : stickers) {
            if (layerCoordinate(sticker.pos, spec.axis) == spec.layer) {
                sticker.pos = rotateQuarter(sticker.pos, spec.axis, spec.sign);
                sticker.normal = rotateQuarter(sticker.normal, spec.axis, spec.sign);
            }
        }
    }
    return stickersToState(stickers);
}

inline std::string applyMoves(std::string state, const std::vector<std::string>& moves) {
    for (const std::string& move : moves) {
        if (isKnownMove(move)) {
            state = applyMove(state, move);
        }
    }
    return state;
}

inline std::string inverseMove(const std::string& move) {
    if (move.size() == 2 && move[1] == '2') {
        return move;
    }
    if (move.size() == 2 && move[1] == '\'') {
        return std::string(1, move[0]);
    }
    return move + "'";
}

inline std::vector<std::string> inverseSequence(const std::vector<std::string>& moves) {
    std::vector<std::string> inverse;
    inverse.reserve(moves.size());
    for (auto it = moves.rbegin(); it != moves.rend(); ++it) {
        inverse.push_back(inverseMove(*it));
    }
    return inverse;
}

inline ValidationResult validateCubeState(const std::string& state) {
    if (state.size() != 54) {
        return {false, "Cube state must contain exactly 54 stickers."};
    }

    std::unordered_map<char, int> counts;
    for (char color : state) {
        bool known = std::find(FACE_COLORS.begin(), FACE_COLORS.end(), color) != FACE_COLORS.end();
        if (!known) {
            return {false, "Cube state contains an unknown sticker color."};
        }
        counts[color]++;
    }

    for (char color : FACE_COLORS) {
        if (counts[color] != 9) {
            return {false, "Each cube color must appear exactly 9 times."};
        }
    }

    for (std::size_t i = 0; i < CENTER_INDEXES.size(); ++i) {
        if (state[CENTER_INDEXES[i]] != FACE_COLORS[i]) {
            return {false, "Center stickers must stay fixed: W, R, G, Y, O, B."};
        }
    }

    return {true, "Cube state is valid."};
}

inline int misplacedCount(const std::string& state) {
    int misplaced = 0;
    for (std::size_t i = 0; i < state.size() && i < SOLVED_STATE.size(); ++i) {
        if (state[i] != SOLVED_STATE[i]) {
            ++misplaced;
        }
    }
    return misplaced;
}

inline int heuristicCost(const std::string& state) {
    int misplaced = misplacedCount(state);
    if (misplaced == 0) {
        return 0;
    }
    // A face turn can reposition many stickers, so this conservative lower
    // bound keeps A* and branch-and-bound useful without over-pruning.
    return std::max(1, static_cast<int>(std::ceil(misplaced / 20.0)));
}

inline std::vector<std::pair<std::string, std::string>> neighbors(const std::string& state) {
    std::vector<std::pair<std::string, std::string>> result;
    result.reserve(MOVES.size());
    for (const std::string& move : MOVES) {
        result.push_back({applyMove(state, move), move});
    }
    return result;
}

inline double elapsedMs(std::chrono::steady_clock::time_point start) {
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

inline ComplexityInfo complexityFor(const std::string& algorithm) {
    if (algorithm == "BFS") return {"O(b^d)", "O(b^d)"};
    if (algorithm == "DFS") return {"O(b^m)", "O(bm)"};
    if (algorithm == "Backtracking") return {"O(b^d) with pruning", "O(d)"};
    if (algorithm == "Branch and Bound") return {"O(b^d), reduced by bounds", "O(d)"};
    if (algorithm == "Greedy") return {"O(bd)", "O(d)"};
    if (algorithm == "Heuristic Search") return {"O(b^d), heuristic-guided", "O(b^d)"};
    if (algorithm == "IDDFS") return {"O(b^d)", "O(d)"};
    if (algorithm == "A*") return {"O(b^d), heuristic-dependent", "O(b^d)"};
    return {"O(b^d)", "O(b^d)"};
}

}  // namespace rubik

#endif
