//===----------------------------------------------------------------------===//
// This file is automatically generated by scripts/generate_serialization.py
// Do not edit this file manually, your changes will be overwritten
//===----------------------------------------------------------------------===//

#include "duckdb/common/serializer/serializer.hpp"
#include "duckdb/common/serializer/deserializer.hpp"
#include "duckdb/parser/result_modifier.hpp"
#include "duckdb/planner/bound_result_modifier.hpp"

namespace duckdb {

void ResultModifier::FormatSerialize(FormatSerializer &serializer) const {
	serializer.WriteProperty(100, "type", type);
}

unique_ptr<ResultModifier> ResultModifier::FormatDeserialize(FormatDeserializer &deserializer) {
	auto type = deserializer.ReadProperty<ResultModifierType>(100, "type");
	unique_ptr<ResultModifier> result;
	switch (type) {
	case ResultModifierType::DISTINCT_MODIFIER:
		result = DistinctModifier::FormatDeserialize(deserializer);
		break;
	case ResultModifierType::LIMIT_MODIFIER:
		result = LimitModifier::FormatDeserialize(deserializer);
		break;
	case ResultModifierType::LIMIT_PERCENT_MODIFIER:
		result = LimitPercentModifier::FormatDeserialize(deserializer);
		break;
	case ResultModifierType::ORDER_MODIFIER:
		result = OrderModifier::FormatDeserialize(deserializer);
		break;
	default:
		throw SerializationException("Unsupported type for deserialization of ResultModifier!");
	}
	return result;
}

void BoundOrderModifier::FormatSerialize(FormatSerializer &serializer) const {
	serializer.WriteProperty(100, "orders", orders);
}

unique_ptr<BoundOrderModifier> BoundOrderModifier::FormatDeserialize(FormatDeserializer &deserializer) {
	auto result = duckdb::unique_ptr<BoundOrderModifier>(new BoundOrderModifier());
	deserializer.ReadProperty(100, "orders", result->orders);
	return result;
}

void DistinctModifier::FormatSerialize(FormatSerializer &serializer) const {
	ResultModifier::FormatSerialize(serializer);
	serializer.WriteProperty(200, "distinct_on_targets", distinct_on_targets);
}

unique_ptr<ResultModifier> DistinctModifier::FormatDeserialize(FormatDeserializer &deserializer) {
	auto result = duckdb::unique_ptr<DistinctModifier>(new DistinctModifier());
	deserializer.ReadProperty(200, "distinct_on_targets", result->distinct_on_targets);
	return std::move(result);
}

void LimitModifier::FormatSerialize(FormatSerializer &serializer) const {
	ResultModifier::FormatSerialize(serializer);
	serializer.WritePropertyWithDefault(200, "limit", limit, unique_ptr<ParsedExpression>());
	serializer.WritePropertyWithDefault(201, "offset", offset, unique_ptr<ParsedExpression>());
}

unique_ptr<ResultModifier> LimitModifier::FormatDeserialize(FormatDeserializer &deserializer) {
	auto result = duckdb::unique_ptr<LimitModifier>(new LimitModifier());
	deserializer.ReadPropertyWithDefault(200, "limit", result->limit, unique_ptr<ParsedExpression>());
	deserializer.ReadPropertyWithDefault(201, "offset", result->offset, unique_ptr<ParsedExpression>());
	return std::move(result);
}

void LimitPercentModifier::FormatSerialize(FormatSerializer &serializer) const {
	ResultModifier::FormatSerialize(serializer);
	serializer.WritePropertyWithDefault(200, "limit", limit, unique_ptr<ParsedExpression>());
	serializer.WritePropertyWithDefault(201, "offset", offset, unique_ptr<ParsedExpression>());
}

unique_ptr<ResultModifier> LimitPercentModifier::FormatDeserialize(FormatDeserializer &deserializer) {
	auto result = duckdb::unique_ptr<LimitPercentModifier>(new LimitPercentModifier());
	deserializer.ReadPropertyWithDefault(200, "limit", result->limit, unique_ptr<ParsedExpression>());
	deserializer.ReadPropertyWithDefault(201, "offset", result->offset, unique_ptr<ParsedExpression>());
	return std::move(result);
}

void OrderModifier::FormatSerialize(FormatSerializer &serializer) const {
	ResultModifier::FormatSerialize(serializer);
	serializer.WriteProperty(200, "orders", orders);
}

unique_ptr<ResultModifier> OrderModifier::FormatDeserialize(FormatDeserializer &deserializer) {
	auto result = duckdb::unique_ptr<OrderModifier>(new OrderModifier());
	deserializer.ReadProperty(200, "orders", result->orders);
	return std::move(result);
}

} // namespace duckdb
