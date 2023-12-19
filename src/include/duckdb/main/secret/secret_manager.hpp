//===----------------------------------------------------------------------===//
//                         DuckDB
//
// duckdb/main/secret/duck_secret_manager.hpp
//
//
//===----------------------------------------------------------------------===//

#pragma once

#include "duckdb/catalog/default/default_generator.hpp"
#include "duckdb/common/common.hpp"
#include "duckdb/main/secret/secret.hpp"
#include "duckdb/main/secret/secret_storage.hpp"
#include "duckdb/main/secret/secret_manager.hpp"
#include "duckdb/parser/parsed_data/create_secret_info.hpp"

namespace duckdb {
class SecretManager;
struct DBConfig;
class SchemaCatalogEntry;

//! Return value of a Secret Lookup
struct SecretMatch {
public:
	SecretMatch() : secret_entry(nullptr), score(-1) {
	}
	SecretMatch(SecretEntry &secret_entry, int64_t score) : secret_entry(&secret_entry), score(score) {
	}

	//! Get the secret
	const BaseSecret &GetSecret();

	bool HasMatch() {
		return secret_entry;
	}

	optional_ptr<SecretEntry> secret_entry;
	int64_t score;
};

//! A Secret Entry in the secret manager
struct SecretEntry : public InCatalogEntry {
public:
	SecretEntry(unique_ptr<const BaseSecret> secret, Catalog &catalog, string name)
	    : InCatalogEntry(CatalogType::SECRET_ENTRY, catalog, name), secret(std::move(secret)) {
		internal = true;
	}

	//! Whether the secret is persistent
	SecretPersistType persist_type;
	//! The storage backend of the secret
	string storage_mode;
	//! The secret pointer
	unique_ptr<const BaseSecret> secret;
};

struct SecretManagerConfig {
	static constexpr const bool DEFAULT_ALLOW_PERSISTENT_SECRETS = true;
	//! The default persistence type for secrets
	SecretPersistType default_persist_type = SecretPersistType::TEMPORARY;
	//! Secret Path can be changed by until the secret manager is initialized, after that it will be set automatically
	string secret_path = "";
	//! The default secret path is determined on startup and can be used to reset the secret path
	string default_secret_path = "";
	//! The storage type for persistent secrets when none is specified;
	string default_persistent_storage = "";
	//! Persistent secrets are enabled by default
	bool allow_persistent_secrets = DEFAULT_ALLOW_PERSISTENT_SECRETS;
};

//! The Secret Manager for DuckDB. Can handle both temporary and persistent secrets
class SecretManager {
	friend struct SecretEntry;

public:
	explicit SecretManager() = default;
	virtual ~SecretManager() = default;

	//! The default storage backends
	static constexpr const char *TEMPORARY_STORAGE_NAME = "memory";
	static constexpr const char *LOCAL_FILE_STORAGE_NAME = "local_file";

	//! Static Helper Functions
	DUCKDB_API static SecretManager &Get(ClientContext &context);
	DUCKDB_API static SecretManager &Get(DatabaseInstance &db);

	// Initialize the secret manager with the DB instance
	DUCKDB_API void Initialize(DatabaseInstance &db);
	//! Load a secret storage
	DUCKDB_API void LoadSecretStorage(unique_ptr<SecretStorage> storage);

	//! Deserialize a secret by automatically selecting the correct deserializer
	DUCKDB_API unique_ptr<BaseSecret> DeserializeSecret(CatalogTransaction transaction, Deserializer &deserializer);
	//! Register a new SecretType
	DUCKDB_API void RegisterSecretType(CatalogTransaction transaction, SecretType &type);
	//! Lookup a SecretType
	DUCKDB_API SecretType LookupType(CatalogTransaction transaction, const string &type);
	//! Register a Secret Function i.e. a secret provider for a secret type
	DUCKDB_API void RegisterSecretFunction(CatalogTransaction transaction, CreateSecretFunction function,
	                                       OnCreateConflict on_conflict);
	//! Register a secret by providing a secret manually
	DUCKDB_API optional_ptr<SecretEntry> RegisterSecret(CatalogTransaction transaction,
	                                                    unique_ptr<const BaseSecret> secret,
	                                                    OnCreateConflict on_conflict, SecretPersistType persist_type,
	                                                    const string &storage = "");
	//! Create a secret from a CreateSecretInfo
	DUCKDB_API optional_ptr<SecretEntry> CreateSecret(ClientContext &context, const CreateSecretInfo &info);
	//! The Bind for create secret is done by the secret manager
	DUCKDB_API BoundStatement BindCreateSecret(CatalogTransaction transaction, CreateSecretInfo &info);
	//! Lookup the best matching secret by matching the secret scopes to the path
	DUCKDB_API SecretMatch LookupSecret(CatalogTransaction transaction, const string &path, const string &type);
	//! Get a secret by name, optionally from a specific storage
	DUCKDB_API optional_ptr<SecretEntry> GetSecretByName(CatalogTransaction transaction, const string &name,
	                                                     const string &storage = "");
	//! Delete a secret by name, optionally by providing the storage to drop from
	DUCKDB_API void DropSecretByName(CatalogTransaction transaction, const string &name,
	                                 OnEntryNotFound on_entry_not_found, const string &storage = "");
	//! List all secrets from all secret storages
	DUCKDB_API vector<reference<SecretEntry>> AllSecrets(CatalogTransaction transaction);

	//! Secret Manager settings
	DUCKDB_API virtual void SetEnablePersistentSecrets(bool enabled);
	DUCKDB_API virtual void ResetEnablePersistentSecrets();
	DUCKDB_API virtual bool PersistentSecretsEnabled();

	DUCKDB_API virtual void SetDefaultStorage(string storage);
	DUCKDB_API virtual void ResetDefaultStorage();
	DUCKDB_API virtual string DefaultStorage();

	DUCKDB_API virtual void SetPersistentSecretPath(const string &path);
	DUCKDB_API virtual void ResetPersistentSecretPath();
	DUCKDB_API virtual string PersistentSecretPath();

	//! Utility functions
	DUCKDB_API void DropSecretByName(ClientContext &context, const string &name, OnEntryNotFound on_entry_not_found,
	                                 const string &storage = "");

private:
	//! Deserialize a secret
	unique_ptr<BaseSecret> DeserializeSecretInternal(CatalogTransaction transaction, Deserializer &deserializer);
	//! Lookup a SecretType
	SecretType LookupTypeInternal(CatalogTransaction transaction, const string &type);
	//! Lookup a CreateSecretFunction
	optional_ptr<CreateSecretFunction> LookupFunctionInternal(CatalogTransaction transaction, const string &type,
	                                                          const string &provider);
	//! Register a new Secret
	optional_ptr<SecretEntry> RegisterSecretInternal(CatalogTransaction transaction,
	                                                 unique_ptr<const BaseSecret> secret, OnCreateConflict on_conflict,
	                                                 SecretPersistType persist_type, const string &storage = "");
	//! Initialize the secret catalog_set and persistent secrets (lazily)
	void InitializeSecrets(CatalogTransaction transaction);

	//! Autoload extension for specific secret type
	void AutoloadExtensionForType(ClientContext &context, const string &type);
	//! Autoload extension for specific secret function
	void AutoloadExtensionForFunction(ClientContext &context, const string &type, const string &provider);

	//! Thread-safe accessors for secret_storages
	vector<reference<SecretStorage>> GetSecretStorages();
	optional_ptr<SecretStorage> GetSecretStorage(const string &name);

	//! Throw an exception if the secret manager is initialized
	void ThrowOnSettingChangeIfInitialized();

	//! Secret CatalogSets (thread-safe)
	unique_ptr<CatalogSet> secret_types;
	unique_ptr<CatalogSet> secret_functions;

	//! Lock for settings and storages
	mutex manager_lock;
	//! Map of all registered SecretStorages
	case_insensitive_map_t<unique_ptr<SecretStorage>> secret_storages;
	//! While false, secret manager settings can still be changed
	atomic<bool> initialized {false};
	//! Configuration for secret manager
	SecretManagerConfig config;
};

//! The DefaultGenerator for persistent secrets. This is used to store lazy loaded secrets in the catalog
class DefaultSecretGenerator : public DefaultGenerator {
public:
	DefaultSecretGenerator(Catalog &catalog, SecretManager &secret_manager, case_insensitive_set_t &persistent_secrets);

public:
	unique_ptr<CatalogEntry> CreateDefaultEntry(ClientContext &context, const string &entry_name) override;
	vector<string> GetDefaultEntries() override;

protected:
	SecretManager &secret_manager;
	case_insensitive_set_t persistent_secrets;
};

} // namespace duckdb
