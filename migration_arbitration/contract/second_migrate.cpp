void arbitration::migrate(uint16_t counter, uint16_t max_number_rows) {

    //authenticate
    check(has_auth(get_self()), "Missing authority");
    
    migration_table migrations(get_self(), get_self().value);

    if(counter == 0) {
        //Create migration table to keep track of the process. 
        migration _migration;
        _migration.in_process = 2;
        _migration.migrating = true;
        migrations.set(_migration, get_self());
    }

    uint16_t i = 0; //Migrated rows counter


    //Open tables that you are intending to migrate 
    //and their corresponding support table HERE
    /* BEGIN MODIFY */
    casefiles_table casefiles(get_self(), get_self().value);

    /* END MODIFY */

    bool migration_ended = false;

    while(i < max_number_rows && !migration_ended) {
        /* BEGIN MODIFY */

        auto cf_itr = casefiles.begin();
        if(cf_itr != casefiles.end()) {
            claims_table claims(get_self(), cf_itr->case_id);
            support_claims_table supportclaims(get_self(), cf_itr->case_id);

            auto claim_itr = supportclaims.begin();
            while(claim_itr != supportclaims.end()) {
                claims.emplace(get_self(), [&](auto& col) {
                    col.claim_id = claim_itr->claim_id;
                    col.claim_summary = claim_itr->claim_summary;
                    col.decision_link = claim_itr->decision_link;
                    col.response_link = claim_itr->response_link;
                    col.status = claim_itr->status;
                    col.claim_category = claim_itr->claim_category;
                    col.claimant_limit_time = claim_itr->claimant_limit_time;
                    col.respondant_limit_time = claim_itr->respondant_limit_time;
                    col.claim_info_required = string("");
                    col.response_info_required = string("");
                    col.claim_info_needed = claim_itr->claim_info_needed;
                    col.response_info_needed = claim_itr->response_info_needed;
                });

                claim_itr = supportclaims.erase(claim_itr);
            }

        /* END MODIFY */
        } else {
            //In the final loop migrate Only singleton Tables
            /* BEGIN MODIFY */
            /* END MODIFY */

            //Update the migration table by setting that the migration process has ended.       
            migration _updated_migration;
            _updated_migration.in_process = 0;
            _updated_migration.migrating = false;
            migrations.set(_updated_migration, get_self());

            migration_ended = true;
        }
        
        ++i;
    }
}


