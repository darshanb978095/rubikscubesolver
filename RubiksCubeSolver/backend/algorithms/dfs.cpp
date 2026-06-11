#include "solver_algorithms.hpp"

#include <algorithm>
#include <chrono>
#include <stack>
#include <unordered_set>

namespace rubik {

SolveResult depthFirstSearch(const std::string& start, const SearchOptions& options) {
    auto clockStart = std::chrono::steady_clock::now();
    SolveResult result;
    result.algorithm = "DFS";

    if (isSolved(start)) {
        result.solved = true;
        result.message = "Cube is already solved.";
        result.timeMs = elapsedMs(clockStart);
        return result;
    }

    struct Node {
        std::string state;
        std::vector<std::string> path;
    };

    std::stack<Node> frontier;
    std::unordered_set<std::string> visited;
    frontier.push({start, {}});
    visited.insert(start);

    while (!frontier.empty()) {
        Node current = frontier.top();
        frontier.pop();
        ++result.nodesExpanded;
        result.depthReached = std::max(result.depthReached, static_cast<int>(current.path.size()));

        if (result.nodesExpanded >= options.maxNodes) {
            break;
        }
        if (static_cast<int>(current.path.size()) >= options.maxDepth) {
            continue;
        }

        for (auto it = MOVES.rbegin(); it != MOVES.rend(); ++it) {
            const std::string& move = *it;
            if (!current.path.empty() && isSameFace(current.path.back(), move)) {
                continue;
            }

            std::string next = applyMove(current.state, move);
            if (visited.find(next) != visited.end()) {
                continue;
            }

            std::vector<std::string> nextPath = current.path;
            nextPath.push_back(move);

            if (isSolved(next)) {
                result.solved = true;
                result.moves = nextPath;
                result.depthReached = static_cast<int>(nextPath.size());
                result.message = "DFS found a valid solution path inside the depth limit.";
                result.timeMs = elapsedMs(clockStart);
                return result;
            }

            visited.insert(next);
            frontier.push({next, nextPath});
        }
    }

    result.message = "DFS traversal finished without finding a solution inside the limit.";
    result.timeMs = elapsedMs(clockStart);
    return result;
}

}  // namespace rubik
