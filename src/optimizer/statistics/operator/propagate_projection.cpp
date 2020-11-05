#include "duckdb/optimizer/statistics_propagator.hpp"
#include "duckdb/planner/operator/logical_projection.hpp"

namespace duckdb {

bool StatisticsPropagator::PropagateStatistics(LogicalProjection &proj) {
	// first propagate to the child
	if (PropagateStatistics(*proj.children[0])) {
		return true;
	}
	// then propagate to each of the expressions
	for(idx_t i = 0; i < proj.expressions.size(); i++) {
		auto stats = PropagateExpression(*proj.expressions[i]);
		if (stats) {
			ColumnBinding binding(proj.table_index, i);
			statistics_map.insert(make_pair(binding, move(stats)));
		}
	}
	return false;
}

}
