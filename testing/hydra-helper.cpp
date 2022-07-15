TABLE config
{
   name admin;
    string contract_version;
    uint16_t max_elected_arbs = 21;
    uint32_t election_voting_ts = 2505600; //29 days
    uint32_t runoff_election_voting_ts = 604800;
    uint32_t election_add_candidates_ts = 604800; //7 days
    uint32_t arb_term_length = 31536000; //365 days
    uint32_t claimant_accepting_offers_ts = 604800; //7 days
    uint64_t current_election_id;
    uint8_t max_claims_per_case = 21;
    asset fee_usd = asset(100000, USD_SYM);
    asset available_funds = asset(0, TLOS_SYM);
    asset reserved_funds = asset(0, TLOS_SYM);
    
    int64_t primary_key() const { return name("config").value; };

    EOSLIB_SERIALIZE(config, (admin)(contract_version)(max_elected_arbs)(election_voting_ts)
        (runoff_election_voting_ts)(election_add_candidates_ts)(arb_term_length)
        (claimant_accepting_offers_ts)(current_election_id)
        (max_claims_per_case)(fee_usd)(available_funds)(reserved_funds))
};

typedef singleton<name("config"), config> config_singleton;
typedef multi_index<name("config"), config>config_for_abi;

HYDRA_FIXTURE_ACTION(
    ((nominees)(nominee)(nominees_table))
    ((arbitrators)(arbitrator)(arbitrators_table))
    ((claims)(claim)(claims_table))
    ((casefiles)(casefile)(casefiles_table))
    ((joinedcases)(joinder)(joinders_table))
    ((accounts)(account)(accounts_table))
    ((offers)(offer)(offers_table))
    ((elections)(election)(elections_table))
    ((config)(config)(config_for_abi)) //Convert singleton into multi_index table
)
