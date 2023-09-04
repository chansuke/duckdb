#include "duckdb/planner/logical_operator.hpp"

#include "duckdb/common/printer.hpp"
#include "duckdb/common/serializer/buffered_deserializer.hpp"
#include "duckdb/common/string_util.hpp"
#include "duckdb/common/tree_renderer.hpp"
#include "duckdb/parser/parser.hpp"
#include "duckdb/planner/operator/list.hpp"
#include "duckdb/planner/operator/logical_extension_operator.hpp"
#include "duckdb/planner/operator/logical_dependent_join.hpp"
#include "duckdb/common/serializer/binary_serializer.hpp"
#include "duckdb/common/serializer/binary_deserializer.hpp"

namespace duckdb {

const uint64_t PLAN_SERIALIZATION_VERSION = 1;

LogicalOperator::LogicalOperator(LogicalOperatorType type)
    : type(type), estimated_cardinality(0), has_estimated_cardinality(false) {
}

LogicalOperator::LogicalOperator(LogicalOperatorType type, vector<unique_ptr<Expression>> expressions)
    : type(type), expressions(std::move(expressions)), estimated_cardinality(0), has_estimated_cardinality(false) {
}

LogicalOperator::~LogicalOperator() {
}

vector<ColumnBinding> LogicalOperator::GetColumnBindings() {
	return {ColumnBinding(0, 0)};
}

string LogicalOperator::GetName() const {
	return LogicalOperatorToString(type);
}

string LogicalOperator::ParamsToString() const {
	string result;
	for (idx_t i = 0; i < expressions.size(); i++) {
		if (i > 0) {
			result += "\n";
		}
		result += expressions[i]->GetName();
	}
	return result;
}

void LogicalOperator::ResolveOperatorTypes() {

	types.clear();
	// first resolve child types
	for (auto &child : children) {
		child->ResolveOperatorTypes();
	}
	// now resolve the types for this operator
	ResolveTypes();
	D_ASSERT(types.size() == GetColumnBindings().size());
}

vector<ColumnBinding> LogicalOperator::GenerateColumnBindings(idx_t table_idx, idx_t column_count) {
	vector<ColumnBinding> result;
	result.reserve(column_count);
	for (idx_t i = 0; i < column_count; i++) {
		result.emplace_back(table_idx, i);
	}
	return result;
}

vector<LogicalType> LogicalOperator::MapTypes(const vector<LogicalType> &types, const vector<idx_t> &projection_map) {
	if (projection_map.empty()) {
		return types;
	} else {
		vector<LogicalType> result_types;
		result_types.reserve(projection_map.size());
		for (auto index : projection_map) {
			result_types.push_back(types[index]);
		}
		return result_types;
	}
}

vector<ColumnBinding> LogicalOperator::MapBindings(const vector<ColumnBinding> &bindings,
                                                   const vector<idx_t> &projection_map) {
	if (projection_map.empty()) {
		return bindings;
	} else {
		vector<ColumnBinding> result_bindings;
		result_bindings.reserve(projection_map.size());
		for (auto index : projection_map) {
			D_ASSERT(index < bindings.size());
			result_bindings.push_back(bindings[index]);
		}
		return result_bindings;
	}
}

string LogicalOperator::ToString() const {
	TreeRenderer renderer;
	return renderer.ToString(*this);
}

void LogicalOperator::Verify(ClientContext &context) {
#ifdef DEBUG
	// verify expressions
	for (idx_t expr_idx = 0; expr_idx < expressions.size(); expr_idx++) {
		auto str = expressions[expr_idx]->ToString();
		// verify that we can (correctly) copy this expression
		auto copy = expressions[expr_idx]->Copy();
		auto original_hash = expressions[expr_idx]->Hash();
		auto copy_hash = copy->Hash();
		// copy should be identical to original
		D_ASSERT(expressions[expr_idx]->ToString() == copy->ToString());
		D_ASSERT(original_hash == copy_hash);
		D_ASSERT(Expression::Equals(expressions[expr_idx], copy));

		for (idx_t other_idx = 0; other_idx < expr_idx; other_idx++) {
			// comparison with other expressions
			auto other_hash = expressions[other_idx]->Hash();
			bool expr_equal = Expression::Equals(expressions[expr_idx], expressions[other_idx]);
			if (original_hash != other_hash) {
				// if the hashes are not equal the expressions should not be equal either
				D_ASSERT(!expr_equal);
			}
		}
		D_ASSERT(!str.empty());

		// verify that serialization + deserialization round-trips correctly
		if (expressions[expr_idx]->HasParameter()) {
			continue;
		}
		throw InternalException("TODO verification");
//		BufferedSerializer serializer;
//		// We are serializing a query plan
//		try {
//			expressions[expr_idx]->Serialize(serializer);
//		} catch (NotImplementedException &ex) {
//			// ignore for now (FIXME)
//			continue;
//		}
//
//		auto data = serializer.GetData();
//		auto deserializer = BufferedContextDeserializer(context, data.data.get(), data.size);
//
//		PlanDeserializationState state(context);
//		auto deserialized_expression = Expression::Deserialize(deserializer, state);
//
//		// format (de)serialization of expressions
//		try {
//			auto blob = BinarySerializer::Serialize(*expressions[expr_idx], true);
//			bound_parameter_map_t parameters;
//			auto result = BinaryDeserializer::Deserialize<Expression>(context, parameters, blob.data(), blob.size());
//			result->Hash();
//		} catch (SerializationException &ex) {
//			// pass
//		}
		// FIXME: expressions might not be equal yet because of statistics propagation
		continue;
//		D_ASSERT(Expression::Equals(expressions[expr_idx], deserialized_expression));
//		D_ASSERT(expressions[expr_idx]->Hash() == deserialized_expression->Hash());
	}
	D_ASSERT(!ToString().empty());
	for (auto &child : children) {
		child->Verify(context);
	}
#endif
}

void LogicalOperator::AddChild(unique_ptr<LogicalOperator> child) {
	D_ASSERT(child);
	children.push_back(std::move(child));
}

idx_t LogicalOperator::EstimateCardinality(ClientContext &context) {
	// simple estimator, just take the max of the children
	if (has_estimated_cardinality) {
		return estimated_cardinality;
	}
	idx_t max_cardinality = 0;
	for (auto &child : children) {
		max_cardinality = MaxValue(child->EstimateCardinality(context), max_cardinality);
	}
	has_estimated_cardinality = true;
	estimated_cardinality = max_cardinality;
	return estimated_cardinality;
}

void LogicalOperator::Print() {
	Printer::Print(ToString());
}

vector<idx_t> LogicalOperator::GetTableIndex() const {
	return vector<idx_t> {};
}

unique_ptr<LogicalOperator> LogicalOperator::Copy(ClientContext &context) const {
	throw InternalException("FIXME: use format serializer");
//	BufferedSerializer logical_op_serializer;
//	try {
//		this->Serialize(logical_op_serializer);
//	} catch (NotImplementedException &ex) {
//		throw NotImplementedException("Logical Operator Copy requires the logical operator and all of its children to "
//		                              "be serializable: " +
//		                              std::string(ex.what()));
//	}
//	auto data = logical_op_serializer.GetData();
//	auto logical_op_deserializer = BufferedContextDeserializer(context, data.data.get(), data.size);
//	PlanDeserializationState state(context);
//	auto op_copy = LogicalOperator::Deserialize(logical_op_deserializer, state);
//	return op_copy;
}

} // namespace duckdb
