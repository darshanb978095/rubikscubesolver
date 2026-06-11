#include "solver_algorithms.hpp"

#include <chrono>
#include <functional>
#include <unordered_set>

namespace rubik {

SolveResult backtrackingSearch(const std::string& start, const SearchOptions& options) {
    auto clockStart = std::chrono::steady_clock::now();
    SolveResult result;
    result.algorithm = "Backtracking";

    std::string current = start;
    std::vector<std::string> path;
    std::unordered_set<std::string> activePath;
    activePath.insert(start);

    std::function<bool(int)> dfs = [&](int depth) -> bool {
        ++result.nodesExpanded;
        result.depthReached = std::max(result.depthReached, depth);

        if (isSolved(current)) {
            result.moves = path;
            return true;
        }
        if (depth == options.maxDepth || result.nodesExpanded >= options.maxNodes) {
            return false;
        }

        for (const std::string& move : MOVES) {
            if (!path.empty() && isSameFace(path.back(), move)) {
                continue;
            }

            current = applyMove(current, move);
            if (activePath.find(current) == activePath.end()) {
                activePath.insert(current);
                path.push_back(move);

                if (dfs(depth + 1)) {
                    return true;
                }

                path.pop_back();
                activePath.erase(current);
            }

            // Backtracking step: undo the rotation before trying a sibling path.
            current = applyMove(current, inverseMove(move));
        }
        return false;
    };

    result.solved = dfs(0);
    result.message = result.solved
                         ? "Backtracking solved the cube by recursively choosing and undoing moves."
                         : "Backtracking could not solve this state within the configured limit.";
    result.timeMs = elapsedMs(clockStart);
    return result;
}

}  // namespace rubik
