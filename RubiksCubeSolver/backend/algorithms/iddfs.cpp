#include "solver_algorithms.hpp"

#include <chrono>
#include <functional>
#include <unordered_set>

namespace rubik {

SolveResult iterativeDeepeningDfs(const std::string& start, const SearchOptions& options) {
    auto clockStart = std::chrono::steady_clock::now();
    SolveResult result;
    result.algorithm = "IDDFS";

    if (isSolved(start)) {
        result.solved = true;
        result.message = "Cube is already solved.";
        result.timeMs = elapsedMs(clockStart);
        return result;
    }

    std::vector<std::string> path;

    std::function<bool(const std::string&, int, int, std::unordered_set<std::string>&)> dls;
    dls = [&](const std::string& state, int depth, int limit, std::unordered_set<std::string>& activePath) -> bool {
        ++result.nodesExpanded;
        result.depthReached = std::max(result.depthReached, depth);

        if (isSolved(state)) {
            return true;
        }
        if (depth == limit || result.nodesExpanded >= options.maxNodes) {
            return false;
        }

        for (const std::string& move : MOVES) {
            if (!path.empty() && isSameFace(path.back(), move)) {
                continue;
            }

            std::string next = applyMove(state, move);
            if (activePath.find(next) != activePath.end()) {
                continue;
            }

            activePath.insert(next);
            path.push_back(move);
            if (dls(next, depth + 1, limit, activePath)) {
                return true;
            }
            path.pop_back();
            activePath.erase(next);
        }
        return false;
    };

    for (int limit = 0; limit <= options.maxDepth; ++limit) {
        std::unordered_set<std::string> activePath;
        activePath.insert(start);
        path.clear();

        if (dls(start, 0, limit, activePath)) {
            result.solved = true;
            result.moves = path;
            result.message = "IDDFS found the shallowest solution by increasing depth one level at a time.";
            result.timeMs = elapsedMs(clockStart);
            return result;
        }

        if (result.nodesExpanded >= options.maxNodes) {
            break;
        }
    }

    result.message = "IDDFS could not find a solution within the current depth/node limit.";
    result.timeMs = elapsedMs(clockStart);
    return result;
}

}  // namespace rubik
