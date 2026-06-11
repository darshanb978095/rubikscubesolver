#include "solver_algorithms.hpp"

#include <chrono>
#include <queue>
#include <unordered_map>

namespace rubik {

SolveResult aStarSearch(const std::string& start, const SearchOptions& options) {
    auto clockStart = std::chrono::steady_clock::now();
    SolveResult result;
    result.algorithm = "A*";

    if (isSolved(start)) {
        result.solved = true;
        result.message = "Cube is already solved.";
        result.timeMs = elapsedMs(clockStart);
        return result;
    }

    struct Node {
        std::string state;
        std::vector<std::string> path;
        int g;
        int h;
        int serial;
    };

    struct Compare {
        bool operator()(const Node& a, const Node& b) const {
            int fa = a.g + a.h;
            int fb = b.g + b.h;
            if (fa != fb) return fa > fb;
            if (a.h != b.h) return a.h > b.h;
            return a.serial > b.serial;
        }
    };

    std::priority_queue<Node, std::vector<Node>, Compare> open;
    std::unordered_map<std::string, int> bestDepth;
    int serial = 0;

    open.push({start, {}, 0, heuristicCost(start), serial++});
    bestDepth[start] = 0;

    while (!open.empty()) {
        Node current = open.top();
        open.pop();

        auto bestKnown = bestDepth.find(current.state);
        if (bestKnown != bestDepth.end() && current.g > bestKnown->second) {
            continue;
        }

        ++result.nodesExpanded;
        result.depthReached = std::max(result.depthReached, current.g);

        if (isSolved(current.state)) {
            result.solved = true;
            result.moves = current.path;
            result.message = "A* solved the cube using g(n) + h(n) priority ordering.";
            result.timeMs = elapsedMs(clockStart);
            return result;
        }

        if (result.nodesExpanded >= options.maxNodes || current.g >= options.maxDepth) {
            continue;
        }

        for (const std::string& move : MOVES) {
            if (!current.path.empty() && isSameFace(current.path.back(), move)) {
                continue;
            }

            std::string next = applyMove(current.state, move);
            int nextG = current.g + 1;
            auto found = bestDepth.find(next);
            if (found != bestDepth.end() && found->second <= nextG) {
                continue;
            }

            std::vector<std::string> nextPath = current.path;
            nextPath.push_back(move);
            bestDepth[next] = nextG;
            open.push({next, nextPath, nextG, heuristicCost(next), serial++});
        }
    }

    result.message = "A* could not reach a solved state inside the search limit.";
    result.timeMs = elapsedMs(clockStart);
    return result;
}

}  // namespace rubik
