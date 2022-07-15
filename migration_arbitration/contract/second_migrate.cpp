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
    support_casefiles_table supportcasefiles(get_self(), get_self().value);

    arbitrators_table arbitrators(get_self(), get_self().value);
    support_arbitrators_table supportarbs(get_self(), get_self().value);

    config_singleton configs(get_self(), get_self().value);
    support_config_singleton support_configs(get_self(), get_self().value);

    /* END MODIFY */

    bool migration_ended = false;

    while(i < max_number_rows && !migration_ended) {
        /* BEGIN MODIFY */

        auto cf_itr = supportcasefiles.begin();
        auto arbs_itr = supportarbs.begin();
        if(cf_itr != supportcasefiles.end()) {
            casefiles.emplace(get_self(), [&](auto& col){
                col.case_id = cf_itr->case_id;
                col.case_status = cf_itr->case_status;
                col.claimant = cf_itr->claimant;
                col.respondant = cf_itr->respondant;
                col.arbitrators = cf_itr->arbitrators;
                col.approvals = cf_itr->approvals;
                col.number_claims = cf_itr->number_claims;
                col.number_offers = 0;
                col.required_langs = cf_itr->required_langs;
                col.case_ruling = cf_itr->case_ruling;
                col.fee_paid_tlos = asset(0, TLOS_SYM);
                col.arbitrator_cost_tlos = asset(0, TLOS_SYM);
                col.update_ts = cf_itr->update_ts;
                col.sending_offers_until_ts = time_point_sec(current_time_point().sec_since_epoch() + 10*86400);
            });

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
                    col.claim_category = claim_itr->decision_class;
                    col.claimant_limit_time = time_point_sec(current_time_point().sec_since_epoch() + 10*86400);
                    col.respondant_limit_time = time_point_sec(current_time_point().sec_since_epoch() + 10*86400);;
                    col.claim_info_needed = false;
                    col.response_info_needed = false;
                });

                claim_itr = supportclaims.erase(claim_itr);
            }

            cf_itr = supportcasefiles.erase(cf_itr);

        /* END MODIFY */
        } else if(arbs_itr != supportarbs.end()) {
            arbitrators.emplace(get_self(), [&](auto& col) {
                col.arb = arbs_itr->arb;
                col.arb_status = arbs_itr->arb_status;
                col.open_case_ids = arbs_itr->open_case_ids;
                col.closed_case_ids = arbs_itr->closed_case_ids;
                col.recused_case_ids = vector<uint64_t>();
                col.credentials_link = arbs_itr->credentials_link;
                col.elected_time = arbs_itr->elected_time;
                col.term_expiration = arbs_itr->term_expiration;
                col.languages = arbs_itr->languages;
            });

            arbs_itr = supportarbs.erase(arbs_itr);
        } else {
            //In the final loop migrate Only singleton Tables
            /* BEGIN MODIFY */

            if(support_configs.exists()) {

                auto conf = support_configs.get();

                 config _config;
                _config.admin = conf.admin;
                _config.contract_version = conf.contract_version;
                _config.max_elected_arbs = conf.max_elected_arbs;
                _config.election_voting_ts = conf.election_voting_ts;
                _config.runoff_election_voting_ts = conf.runoff_election_voting_ts;
                _config.election_add_candidates_ts = conf.election_add_candidates_ts;
                _config.claimant_accepting_offers_ts = 604800; 
                _config.arb_term_length = conf.arb_term_length;
                _config.max_claims_per_case = 21;
                _config.current_election_id = conf.current_election_id;
                _config.fee_usd = asset(100000, USD_SYM);
                _config.available_funds = conf.available_funds;  
                _config.reserved_funds = asset(0, TLOS_SYM);

                configs.set(_config, get_self());    
                support_configs.remove();
            }

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


