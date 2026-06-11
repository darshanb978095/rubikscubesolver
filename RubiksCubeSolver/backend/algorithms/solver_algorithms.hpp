#ifndef RUBIK_SOLVER_ALGORITHMS_HPP
#define RUBIK_SOLVER_ALGORITHMS_HPP

#include "../cube.hpp"

namespace rubik {

SolveResult breadthFirstSearch(const std::string& start, const SearchOptions& options);
SolveResult depthFirstSearch(const std::string& start, const SearchOptions& options);
SolveResult backtrackingSearch(const std::string& start, const SearchOptions& options);
SolveResult branchAndBoundSearch(const std::string& start, const SearchOptions& options);
SolveResult greedySearch(const std::string& start, const SearchOptions& options);
SolveResult heuristicBestFirstSearch(const std::string& start, const SearchOptions& options);
SolveResult iterativeDeepeningDfs(const std::string& start, const SearchOptions& options);
SolveResult aStarSearch(const std::string& start, const SearchOptions& options);

}  // namespace rubik

#endif
