
TABLE migration {
    uint8_t in_process;
    bool migrating;
   
    EOSLIB_SERIALIZE(migration, (in_process)(migrating));
};
typedef singleton<name("migration"), migration> migration_table;
