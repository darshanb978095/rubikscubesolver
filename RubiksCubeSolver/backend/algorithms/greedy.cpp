#include "solver_algorithms.hpp"

#include <chrono>
#include <limits>
#include <unordered_set>

namespace rubik {

SolveResult greedySearch(const std::string& start, const SearchOptions& options) {
    auto clockStart = std::chrono::steady_clock::now();
    SolveResult result;
    result.algorithm = "Greedy";

    std::string current = start;
    std::unordered_set<std::string> visited;
    visited.insert(current);

    for (int depth = 0; depth < options.maxDepth; ++depth) {
        ++result.nodesExpanded;
        result.depthReached = depth;

        if (isSolved(current)) {
            result.solved = true;
            result.message = "Greedy solved the cube by repeatedly selecting the best local rotation.";
            result.timeMs = elapsedMs(clockStart);
            return result;
        }

        std::string bestMove;
        std::string bestState;
        int bestScore = std::numeric_limits<int>::max();

        for (const std::string& move : MOVES) {
            if (!result.moves.empty() && isSameFace(result.moves.back(), move)) {
                continue;
            }

            std::string next = applyMove(current, move);
            if (visited.find(next) != visited.end()) {
                continue;
            }

            int score = misplacedCount(next);
            if (score < bestScore) {
                bestScore = score;
                bestMove = move;
                bestState = next;
            }
        }

        if (bestMove.empty()) {
            break;
        }

        current = bestState;
        visited.insert(current);
        result.moves.push_back(bestMove);

        if (result.nodesExpanded >= options.maxNodes) {
            break;
        }
    }

    if (isSolved(current)) {
        result.solved = true;
        result.message = "Greedy solved the cube by choosing locally optimal rotations.";
    } else {
        result.moves.clear();
        result.message = "Greedy got trapped in a local minimum for this cube state.";
    }

    result.timeMs = elapsedMs(clockStart);
    return result;
}

}  // namespace rubik
