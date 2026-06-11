#include "solver_algorithms.hpp"

#include <chrono>
#include <queue>
#include <unordered_set>

namespace rubik {

SolveResult heuristicBestFirstSearch(const std::string& start, const SearchOptions& options) {
    auto clockStart = std::chrono::steady_clock::now();
    SolveResult result;
    result.algorithm = "Heuristic Search";

    if (isSolved(start)) {
        result.solved = true;
        result.message = "Cube is already solved.";
        result.timeMs = elapsedMs(clockStart);
        return result;
    }

    struct Node {
        std::string state;
        std::vector<std::string> path;
        int h;
        int serial;
    };

    struct Compare {
        bool operator()(const Node& a, const Node& b) const {
            if (a.h != b.h) return a.h > b.h;
            return a.serial > b.serial;
        }
    };

    std::priority_queue<Node, std::vector<Node>, Compare> frontier;
    std::unordered_set<std::string> visited;
    int serial = 0;
    frontier.push({start, {}, misplacedCount(start), serial++});

    while (!frontier.empty()) {
        Node current = frontier.top();
        frontier.pop();

        if (visited.find(current.state) != visited.end()) {
            continue;
        }
        visited.insert(current.state);

        ++result.nodesExpanded;
        result.depthReached = std::max(result.depthReached, static_cast<int>(current.path.size()));

        if (isSolved(current.state)) {
            result.solved = true;
            result.moves = current.path;
            result.message = "Heuristic best-first search reached the solved state.";
            result.timeMs = elapsedMs(clockStart);
            return result;
        }

        if (result.nodesExpanded >= options.maxNodes ||
            static_cast<int>(current.path.size()) >= options.maxDepth) {
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
            frontier.push({next, nextPath, misplacedCount(next), serial++});
        }
    }

    result.message = "Heuristic search did not reach the solved state inside the limit.";
    result.timeMs = elapsedMs(clockStart);
    return result;
}

}  // namespace rubik
