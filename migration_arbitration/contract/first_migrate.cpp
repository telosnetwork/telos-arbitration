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
    support_casefiles_table supportcasefiles(get_self(), get_self().value);

    arbitrators_table arbitrators(get_self(), get_self().value);
    support_arbitrators_table supportarbs(get_self(), get_self().value);

    config_singleton configs(get_self(), get_self().value);
    support_config_singleton support_configs(get_self(), get_self().value);
    /* END MODIFY */

    bool migration_ended = false;
    
    while (i < max_number_rows && !migration_ended) {
        /* BEGIN MODIFY */

        auto cf_itr = casefiles.begin();
        auto arbs_itr = arbitrators.begin();
        if(cf_itr != casefiles.end()) {
            supportcasefiles.emplace(get_self(), [&](auto& col){
                col.case_id = cf_itr->case_id;
                col.case_status = cf_itr->case_status;
                col.claimant = cf_itr->claimant;
                col.respondant = cf_itr->respondant;
                col.arbitrators = cf_itr->arbitrators;
                col.approvals = cf_itr->approvals;
                col.number_claims = cf_itr->number_claims;
                col.required_langs = cf_itr->required_langs;
                col.case_ruling = cf_itr->case_ruling;
                col.update_ts = cf_itr->update_ts;
            });

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
                    col.decision_class = claim_itr->decision_class;
                });

                claim_itr = claims.erase(claim_itr);
            }

            cf_itr = casefiles.erase(cf_itr);

            //If multiple tables need to be migrating, add different else if conditions
            //unless that the second table depends on the first one.

        /* END MODIFY */
        } else if(arbs_itr != arbitrators.end()) {
            supportarbs.emplace(get_self(), [&](auto& col) {
                col.arb = arbs_itr->arb;
                col.arb_status = arbs_itr->arb_status;
                col.open_case_ids = arbs_itr->open_case_ids;
                col.closed_case_ids = arbs_itr->closed_case_ids;
                col.credentials_link = arbs_itr->credentials_link;
                col.elected_time = arbs_itr->elected_time;
                col.term_expiration = arbs_itr->term_expiration;
                col.languages = arbs_itr->languages;
            });

            arbs_itr = arbitrators.erase(arbs_itr);
        } else {
            //In the final loop migrate Only singleton Tables
            /* BEGIN MODIFY */
            if(configs.exists()) {

                auto conf = configs.get();
                supportconf _config;
                _config.admin = conf.admin;
                _config.contract_version = conf.contract_version;
                _config.max_elected_arbs = conf.max_elected_arbs;
                _config.election_voting_ts = conf.election_voting_ts;
                _config.runoff_election_voting_ts = conf.runoff_election_voting_ts;
                _config.election_add_candidates_ts = conf.election_add_candidates_ts;
                _config.fee_structure = conf.fee_structure;
                _config.arb_term_length = conf.arb_term_length;
                _config.current_election_id = conf.current_election_id;
                _config.available_funds = conf.available_funds;

                support_configs.set(_config, get_self());
                configs.remove();

            }
            /* END MODIFY */

            //Update the migration table by setting that the migration process has ended.       
            migration _updated_migration;
            _updated_migration.in_process = 2;
            _updated_migration.migrating = true;
            migrations.set(_updated_migration, get_self());

            migration_ended = true;
        }

        ++i;
    }
}

