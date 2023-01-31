void arbitration::migrate(uint16_t counter, uint16_t max_number_rows) {

    //authenticate
    check(has_auth(get_self()), "Missing authority");

    migration_table migrations(get_self(), get_self().value);

    if(counter == 0) {
        //Create migration table to keep track of the process. 
        migration _migration;
        _migration.in_process = 1;
        _migration.migrating = true;
        migrations.set(_migration, get_self());
    }
    
    uint16_t i = 0; //Migrated rows counter

    //Open tables that you are intending to migrate 
    //and their corresponding support table HERE
    /* BEGIN MODIFY */
    casefiles_table casefiles(get_self(), get_self().value);
    /* END MODIFY */

    
    for(auto cf_itr = casefiles.begin(); cf_itr != casefiles.end(); ++cf_itr) {
        claims_table claims(get_self(), cf_itr->case_id);
        support_claims_table supportclaims(get_self(), cf_itr->case_id);

        auto claim_itr = claims.begin();
        while(claim_itr != claims.end()) {
            supportclaims.emplace(get_self(), [&](auto& col) {
                col.claim_id = claim_itr->claim_id;
                col.claim_summary = claim_itr->claim_summary;
                col.decision_link = claim_itr->decision_link;
                col.response_link = claim_itr->response_link;
                col.status = claim_itr->status;
                col.claim_category = claim_itr->claim_category;
                col.claimant_limit_time = claim_itr->claimant_limit_time;
                col.respondant_limit_time = claim_itr->respondant_limit_time;
                col.claim_info_needed = claim_itr->claim_info_needed;
                col.response_info_needed = claim_itr->response_info_needed;
            });

            claim_itr = claims.erase(claim_itr);
        }
    } 
    
    //Update the migration table by setting that the migration process has ended.       
    migration _updated_migration;
    _updated_migration.in_process = 2;
    _updated_migration.migrating = true;
    migrations.set(_updated_migration, get_self());
}

