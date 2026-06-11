#include "solver_algorithms.hpp"

#include <chrono>
#include <queue>
#include <unordered_set>

namespace rubik {

SolveResult breadthFirstSearch(const std::string& start, const SearchOptions& options) {
    auto clockStart = std::chrono::steady_clock::now();
    SolveResult result;
    result.algorithm = "BFS";

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

    std::queue<Node> frontier;
    std::unordered_set<std::string> visited;
    frontier.push({start, {}});
    visited.insert(start);

    while (!frontier.empty()) {
        Node current = frontier.front();
        frontier.pop();
        ++result.nodesExpanded;
        result.depthReached = std::max(result.depthReached, static_cast<int>(current.path.size()));

        if (result.nodesExpanded >= options.maxNodes) {
            break;
        }
        if (static_cast<int>(current.path.size()) >= options.maxDepth) {
            continue;
        }

        for (const std::string& move : MOVES) {
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
                result.message = "BFS found the shortest solution within the configured depth.";
                result.timeMs = elapsedMs(clockStart);
                return result;
            }

            visited.insert(next);
            frontier.push({next, nextPath});
        }
    }

    result.message = "BFS could not find a solution within the depth/node limit.";
    result.timeMs = elapsedMs(clockStart);
    return result;
}

}  // namespace rubik
