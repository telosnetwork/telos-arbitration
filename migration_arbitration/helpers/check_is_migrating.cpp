
migration_table migrations(get_self(), get_self().value);
auto migrate = migrations.get();

if( migrate.migrating == 1 ) {
    check(false, "The contract " + _self.to_string() + " is migrating");
}
