#include "solver_algorithms.hpp"

#include <chrono>
#include <functional>
#include <unordered_set>

namespace rubik {

SolveResult branchAndBoundSearch(const std::string& start, const SearchOptions& options) {
    auto clockStart = std::chrono::steady_clock::now();
    SolveResult result;
    result.algorithm = "Branch and Bound";

    std::vector<std::string> path;
    std::vector<std::string> bestPath;
    std::unordered_set<std::string> activePath;
    activePath.insert(start);

    int bestCost = options.maxDepth + 1;

    std::function<void(const std::string&, int)> dfs = [&](const std::string& state, int depth) {
        ++result.nodesExpanded;
        result.depthReached = std::max(result.depthReached, depth);

        if (result.nodesExpanded >= options.maxNodes) {
            return;
        }

        int optimisticCost = depth + heuristicCost(state);
        if (optimisticCost >= bestCost) {
            return;
        }

        if (isSolved(state)) {
            bestCost = depth;
            bestPath = path;
            return;
        }

        if (depth == options.maxDepth) {
            return;
        }

        std::vector<std::pair<std::string, std::string>> ordered = neighbors(state);
        std::sort(ordered.begin(), ordered.end(), [](const auto& a, const auto& b) {
            return heuristicCost(a.first) < heuristicCost(b.first);
        });

        for (const auto& candidate : ordered) {
            const std::string& nextState = candidate.first;
            const std::string& move = candidate.second;
            if (!path.empty() && isSameFace(path.back(), move)) {
                continue;
            }
            if (activePath.find(nextState) != activePath.end()) {
                continue;
            }

            activePath.insert(nextState);
            path.push_back(move);
            dfs(nextState, depth + 1);
            path.pop_back();
            activePath.erase(nextState);
        }
    };

    dfs(start, 0);
    result.solved = !bestPath.empty() || isSolved(start);
    result.moves = bestPath;
    result.message = result.solved
                         ? "Branch and bound found the best path after pruning expensive branches."
                         : "Branch and bound pruned the tree but no solution was found in the limit.";
    result.timeMs = elapsedMs(clockStart);
    return result;
}

}  // namespace rubik
