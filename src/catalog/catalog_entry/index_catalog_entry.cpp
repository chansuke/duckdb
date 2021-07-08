#include "duckdb/catalog/catalog_entry/index_catalog_entry.hpp"
#include "duckdb/storage/data_table.hpp"

namespace duckdb {

IndexCatalogEntry::IndexCatalogEntry(Catalog *catalog, SchemaCatalogEntry *schema, CreateIndexInfo *info)
    : StandardEntry(CatalogType::INDEX_ENTRY, schema, catalog, info->index_name), index(nullptr), sql(info->sql) {
}

IndexCatalogEntry::~IndexCatalogEntry() {
	// remove the associated index from the info
	if (!info || !index) {
		return;
	}
	info->indexes.RemoveIndex(index);
}

string IndexCatalogEntry::ToSQL() {
	if (sql.empty()) { // LCOV_EXCL_START
		throw InternalException("Cannot convert INDEX to SQL because it was not created with a SQL statement");
	} // LCOV_EXCL_STOP
	return sql;
}

} // namespace duckdb
