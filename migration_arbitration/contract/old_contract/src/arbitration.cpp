/**
 * Arbitration Contract Implementation. See function bodies for further notes.
 * 
 * @author Craig Branscom, Peter Bue, Ed Silva
 * @copyright defined in telos/LICENSE.txt
 */

#include "../include/arbitration.hpp"
#include "randomness_provider.cpp"

#pragma region Config_Actions

void arbitration::init(name initial_admin) {
   
    //authenticate
    require_auth(get_self());

    //open config singleton
    config_singleton configs(get_self(), get_self().value);

    //validate
    check(!configs.exists(), "contract already initialized");
    check(is_account(initial_admin), "initial admin account doesn't exist");

    //initialize
    config initial_conf;
    initial_conf.admin = initial_admin;
	initial_conf.contract_version = "0.1.0";


    //set initial config
    configs.set(initial_conf, get_self());
}

void arbitration::setadmin(name new_admin) {
    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);

    //validate
    check(is_account(new_admin), "new admin account doesn't exist");

    //change admin
    conf.admin = new_admin;

    //set new config
    configs.set(conf, get_self());
}

void arbitration::setversion(string new_version)
{
    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    check(has_auth(conf.admin) || has_auth(get_self()), "Only admin and SC account can change the version");

    //change contract version
    conf.contract_version = new_version;

    //set new config
    configs.set(conf, get_self());
}

void arbitration::setconfig(uint16_t max_elected_arbs, uint32_t election_duration, uint32_t runoff_duration,
	uint32_t election_add_candidates_duration, uint32_t arbitrator_term_length, vector<int64_t> fees/*, uint8_t max_claims_per_case*/)
{
	//open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

	require_auth(conf.admin);

	check(max_elected_arbs > 0, "Arbitrators must be greater than 0");
	//check(max_claims_per_case > 0, "Minimum 1 claim");

	conf.max_elected_arbs = max_elected_arbs;
	conf.election_voting_ts = election_duration;
	conf.election_add_candidates_ts = election_add_candidates_duration;
	conf.arb_term_length = arbitrator_term_length;
	conf.fee_structure = fees;
	conf.runoff_election_voting_ts = runoff_duration;
	//conf.max_claims_per_case = max_claims_per_case;

	//set new config
    configs.set(conf, get_self());
}

void arbitration::setclaimclss(uint8_t claim_class_id, asset initial_deposit, asset fee, uint16_t respondant_days,
	 	uint8_t number_arbitrators){

	//open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

	require_auth(conf.admin);	
	
	claim_class_table claimclasses(get_self(), get_self().value);
	auto claim_itr = claimclasses.find(claim_class_id);

	check(number_arbitrators >= 1, "At least one arbitrator needs to be set");

	if(claim_itr != claimclasses.end()) {
		claimclasses.modify(claim_itr, same_payer, [&](auto& col){
			col.initial_deposit = initial_deposit;
			col.fee = fee;
			col.respondant_days = respondant_days;
			col.number_arbitrators = number_arbitrators;
		});
	} else {
		claimclasses.emplace(get_self(), [&](auto& col){
			col.claim_class_id = claim_class_id;
			col.initial_deposit = initial_deposit;
			col.fee = fee;
			col.respondant_days = respondant_days;
			col.number_arbitrators = number_arbitrators;
		}); 
	}
}

void arbitration::rmvclaimclss(uint8_t claim_class_id) {
	//open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

	require_auth(conf.admin);

	claim_class_table claimclasses(get_self(), get_self().value);
	auto claim_itr = claimclasses.get(claim_class_id, "Claim class not found");

	claimclasses.erase(claim_itr);
}

#pragma endregion Config_Actions

#pragma region Arb_Elections

void arbitration::initelection(string content)
{
	//open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

	require_auth(conf.admin);

	elections_table elections(get_self(), get_self().value);
	auto last_election = elections.find(conf.current_election_id);
	
	check(last_election == elections.end() || last_election->status == election_status::ENDED, "There is an election already running");	

	arbitrators_table arbitrators(get_self(), get_self().value);

	uint8_t available_seats = 0;
	check(has_available_seats(arbitrators, available_seats), "No seats are available!");
	
	start_new_election(available_seats, false, content);
	
}

void arbitration::regarb(name nominee, string credentials_link)
{

	check(is_account(nominee), "New candidate is not an account");

	require_auth(nominee);
	validate_ipfs_url(credentials_link);

	nominees_table nominees(get_self(), get_self().value);
	auto nom_itr = nominees.find(nominee.value);
	check(nom_itr == nominees.end(), "Nominee is already an applicant");

	arbitrators_table arbitrators(get_self(), get_self().value);
	auto arb_itr = arbitrators.find(nominee.value);

	if (arb_itr != arbitrators.end()) {
		check(time_point_sec(current_time_point()) > arb_itr->term_expiration || 
		arb_itr->arb_status == arb_status::SEAT_EXPIRED || arb_itr->arb_status == arb_status::REMOVED,
			"Nominee is already an Arbitrator and the seat hasn't expired or been removed");

		//NOTE: set arb_status to SEAT_EXPIRED until re-election
		arbitrators.modify(arb_itr, same_payer, [&](auto &row) {
			row.arb_status = static_cast<uint8_t>(arb_status::SEAT_EXPIRED);
		});
	}

	nominees.emplace(get_self(), [&](auto &row) {
		row.nominee_name = nominee;
		row.credentials_link = credentials_link;
		row.application_time = time_point_sec(current_time_point());
	});
}

void arbitration::unregnominee(name nominee)
{
	require_auth(nominee);

	//open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto _config = configs.get();

	nominees_table nominees(get_self(), get_self().value);
	auto nom_itr = nominees.find(nominee.value);
	check(nom_itr != nominees.end(), "Nominee isn't an applicant");

	elections_table elections(get_self(), get_self().value);
	auto election = elections.find(_config.current_election_id);

	if(election != elections.end()) {
		bool found = false;

		for (auto i = 0; i < election->candidates.size(); ++i) {
			if (election->candidates[i].name == nominee) {
				found = true;
				break;
			}
		}

   		check(found == false, "Cannot remove a nominee if is a candidate");

	}
	nominees.erase(nom_itr);
}

void arbitration::candaddlead(name nominee)
{
	require_auth(nominee);
	
	//open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto _config = configs.get();

	elections_table elections(get_self(), get_self().value);
	auto& election = elections.get(_config.current_election_id, "Election doesn't exist");

	check(election.status == election_status::CREATED, "Election needs to be in CREATED status to add a new candidate");
	check(time_point_sec(current_time_point()) < election.end_add_candidates_ts, "Cannot add candidates if period has ended");

	nominees_table nominees(get_self(), get_self().value);
	auto nom_itr = nominees.find(nominee.value);
	check(nom_itr != nominees.end(), "Nominee isn't an applicant. Use regarb action to register as a nominee");

    auto existing_candidate = std::find_if(election.candidates.begin(), election.candidates.end(), [&](auto &candidate) {
		return candidate.name == nominee;
	});
	
    check(existing_candidate == election.candidates.end(), "Candidate already added");

    elections.modify(election, same_payer, [&]( auto& col ) {
        col.candidates.push_back(candidate{
        	nominee,
        	asset(0, VOTE_SYM),
		});
    });
}

void arbitration::candrmvlead(name nominee)
{
	require_auth(nominee);

	//open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto _config = configs.get();

	elections_table elections(get_self(), get_self().value);
	auto& election = elections.get(_config.current_election_id, "Election doesn't exist");

	check(election.status == election_status::CREATED, "Election needs to be in CREATED status to remove a new candidate");
	check(time_point_sec(current_time_point()) < election.end_add_candidates_ts, "Cannot remove candidates if period has ended");

    bool found = false;

	auto new_candidates = election.candidates;
    for (auto i = 0; i < new_candidates.size(); ++i) {
        if (new_candidates[i].name == nominee) {
            new_candidates.erase(new_candidates.begin() + i);
            found = true;
            break;
        }
    }

    check(found == true, "Candidate not found in election list");

	elections.modify(election, same_payer, [&]( auto& col ) {
		col.candidates = new_candidates;
	});
}

void arbitration::beginvoting(name ballot_name, bool runoff) {
	
	//open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

	elections_table elections(get_self(), get_self().value);
	auto& election = elections.get(conf.current_election_id, "Election doesn't exist");

    auto elections_by_ballot = elections.get_index<name("byballot")>();
    auto by_ballot_itr = elections_by_ballot.find(ballot_name.value);
    check(by_ballot_itr == elections_by_ballot.end(), "Ballot name already used");	

    //authenticate
    check(has_auth(conf.admin) || has_auth(get_self()), "Only admin and SC account can start a voting process");
	
    //initialize
    time_point_sec now = time_point_sec(current_time_point());
    time_point_sec ballot_end_time = !runoff ? now + conf.election_voting_ts : now + conf.runoff_election_voting_ts;
    
    //Get ballot options
    vector<name> ballot_options = {};
    
    for (auto i = 0; i < election.candidates.size(); ++i) {
       ballot_options.push_back(election.candidates[i].name);
    }

    //validate
    check(election.status == election_status::CREATED, "Election must be in created status");
    check(time_point_sec(current_time_point()) >= election.end_add_candidates_ts, "Can't start voting process until add candidates time has ended.");
    
	if(election.candidates.size() > election.available_seats) {
		telosdecide::config_singleton configdecide(TELOSDECIDE, TELOSDECIDE.value);
		auto confdecide = configdecide.get();
		asset newballot_fee = confdecide.fees.at(name("ballot"));

		//update project
		elections.modify(election, get_self(), [&](auto& col) {
			col.status = static_cast<uint8_t>(election_status::LIVE);
			col.ballot_name = ballot_name;
			col.begin_voting_ts = now;
			col.end_voting_ts = ballot_end_time;
		});

		check(conf.available_funds >= newballot_fee, "The SC doesn't have enough funds to cover the ballot fee");

		//send inline transfer to pay for newballot fee
		action(permission_level{get_self(), name("active")}, name("eosio.token"), name("transfer"), make_tuple(
			get_self(), //from
			TELOSDECIDE, //to
			newballot_fee, //quantity
			string("Decide New Ballot Fee Payment") //memo
		)).send();

		//Remove fee from available funds
		conf.available_funds -= newballot_fee;
		configs.set(conf, get_self());

		//send inline newballot to decide
		action(permission_level{get_self(), name("active")}, TELOSDECIDE, name("newballot"), make_tuple(
			ballot_name, //ballot_name
			name("election"), //category
			get_self(), //publisher
			VOTE_SYM, //treasury_symbol
			name("1tokennvote"), //voting_method  
			ballot_options //initial_options
		)).send();


		string title = "Telos Arbitrator Election";
		if(runoff) title += " Runoff";

		//send inline editdetails to decide
		action(permission_level{get_self(), name("active")}, TELOSDECIDE, name("editdetails"), make_tuple(
			ballot_name, //ballot_name
			string(title), //title
			string(""), //description
			string(election.info_url) //content
		)).send();

		//set min/max voting options to decide
		action(permission_level{get_self(), name("active")}, TELOSDECIDE, name("editminmax"), make_tuple(
			ballot_name, //ballot_name
			uint8_t(1), //min
			uint8_t(election.available_seats) //max options
		)).send();

		//toggle ballot votestake on (default is off)
		action(permission_level{get_self(), name("active")}, TELOSDECIDE, name("togglebal"), make_tuple(
			ballot_name, //ballot_name
			name("votestake") //setting_name
		)).send();

		//send inline openvoting to decide
		action(permission_level{get_self(), name("active")}, TELOSDECIDE, name("openvoting"), make_tuple(
			ballot_name, //ballot_name
			ballot_end_time //end_time
		)).send();
	} else {

		nominees_table nominees(get_self(), get_self().value);
		arbitrators_table arbitrators(get_self(), get_self().value);

		for(int i = 0; i < election.candidates.size(); ++i) {
			name candidate_name = election.candidates[i].name;
			auto& candidate_itr = nominees.get(candidate_name.value);
			auto cand_credential = candidate_itr.credentials_link;

			add_arbitrator(arbitrators, candidate_name, cand_credential);
			nominees.erase(candidate_itr);
			
		}

		auto perms = get_arb_permissions();
		set_permissions(perms);

		//update election
		elections.modify(election, get_self(), [&](auto& col) {
			col.status = static_cast<uint8_t>(election_status::ENDED);
			col.begin_voting_ts = now;
			col.end_voting_ts = now;
		});
	}
    
}

void arbitration::endelection()
{
    //open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //open project table, find project
    elections_table elections(get_self(), get_self().value);
    auto& election = elections.get(conf.current_election_id, "Election not found");

    //authenticate
    check(has_auth(conf.admin) || has_auth(get_self()), "Only admin and SC account can start a voting process");

	telosdecide::config_singleton configdecide(TELOSDECIDE, TELOSDECIDE.value);
    auto confdecide = configdecide.get();
    asset newballot_fee = confdecide.fees.at(name("ballot"));

	//Check that the SC has enough funds to cover a new voting in case of fee
	check(conf.available_funds >= newballot_fee, "Not enough funds to cover ballot fees in case of new election");

    //validate
    check(election.status == election_status::LIVE, "Election status must be in live state");
    check(time_point_sec(current_time_point()) > election.end_voting_ts, "Vote time has not ended");

    //send inline closevoting to decide
    action(permission_level{get_self(), name("active")}, TELOSDECIDE, name("closevoting"), make_tuple(
        election.ballot_name, //ballot_name
        true //broadcast
    )).send();

    //NOTE: results processed in catch_broadcast()
}



#pragma endregion Arb_Elections

#pragma region Claimant_Actions
void arbitration::withdraw(name owner)
{
	require_auth(owner);

	accounts_table accounts(get_self(), owner.value);
	const auto &bal = accounts.get(TLOS_SYM.code().raw(), "balance does not exist");

	action(permission_level{get_self(), name("active")}, name("eosio.token"), name("transfer"),
		   make_tuple(get_self(),
					  owner,
					  bal.balance,
					  std::string("Telos Arbitration withdrawal")))
		.send();

	accounts.erase(bal);
}

void arbitration::filecase(name claimant, string claim_link, vector<uint16_t> lang_codes, std::optional<name> respondant)
{
	require_auth(claimant);
	validate_ipfs_url(claim_link);

	if(respondant) {
		check(is_account(*respondant), "Respondant must be an account");
	}

	casefiles_table casefiles(get_self(), get_self().value);
	uint64_t new_case_id = casefiles.available_primary_key();

	casefiles.emplace(claimant, [&](auto &row) {
		row.case_id = new_case_id;
		row.case_status = static_cast<uint8_t>(case_status::CASE_SETUP);
		row.claimant = claimant;
		row.respondant = *respondant;
		row.arbitrators = {};
		row.approvals = {};
		row.required_langs = lang_codes;
		row.number_claims = 1;
		row.case_ruling = std::string("");
		row.update_ts = time_point_sec(current_time_point());
	});

	claims_table claims(get_self(), new_case_id);
	uint64_t new_claim_id = claims.available_primary_key();

	claims.emplace(get_self(), [&](auto& col){
		col.claim_id = new_claim_id;
		col.claim_summary = claim_link;
		col.status = static_cast<uint8_t>(claim_status::FILED);
	});
}

void arbitration::addclaim(uint64_t case_id, string claim_link, name claimant)
{
	require_auth(claimant);
	validate_ipfs_url(claim_link);

	//open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

	casefiles_table casefiles(get_self(), get_self().value);
	const auto& cf = casefiles.get(case_id, "Case Not Found");

	check(cf.case_status == case_status::CASE_SETUP, "claims cannot be added after CASE_SETUP is complete.");

	//check(cf.number_claims < conf.max_claims_per_case, "case file has reached maximum number of claims");
	check(claimant == cf.claimant, "you are not the claimant of this case.");

	claims_table claims(get_self(), case_id);

	for(auto itr = claims.begin(); itr != claims.end(); ++itr) {
		check(itr->claim_summary != claim_link, "Claim Link already exists in another claim");
	}

	uint64_t new_claim_id = claims.available_primary_key();
	claims.emplace(get_self(), [&](auto& col){
		col.claim_id = new_claim_id;
		col.claim_summary = claim_link;
		col.status = static_cast<uint8_t>(claim_status::FILED);
	});
	
	casefiles.modify(cf, same_payer, [&](auto &row) {
		row.number_claims += 1;
		row.update_ts = time_point_sec(current_time_point());
	});
}

void arbitration::removeclaim(uint64_t case_id, uint64_t claim_id, name claimant)
{
	require_auth(claimant);

	casefiles_table casefiles(get_self(), get_self().value);
	const auto& cf = casefiles.get(case_id, "Case Not Found");
	check(cf.case_status == case_status::CASE_SETUP, "Claims cannot be removed after CASE_SETUP is complete");
	check(claimant == cf.claimant, "you are not the claimant of this case.");
	
	claims_table claims(get_self(), case_id);
	auto claim_it = claims.find(claim_id);

	check(claim_it != claims.end(), "Claim not found");
	claims.erase(claim_it);

	casefiles.modify(cf, same_payer, [&](auto &row) {
		row.update_ts = time_point_sec(current_time_point());
		row.number_claims -= 1;
	});
}

void arbitration::shredcase(uint64_t case_id, name claimant)
{
	require_auth(claimant);

	casefiles_table casefiles(get_self(), get_self().value);
	auto c_itr = casefiles.find(case_id);
	check(c_itr != casefiles.end(), "Case Not Found");
	check(claimant == c_itr->claimant, "you are not the claimant of this case.");
	check(c_itr->case_status == case_status::CASE_SETUP, "cases can only be shredded during CASE_SETUP");

	claims_table claims(get_self(), case_id);
	auto claim_it = claims.begin();
	while(claim_it != claims.end()) {
		claim_it = claims.erase(claim_it);
	}

	casefiles.erase(c_itr);
}

void arbitration::readycase(uint64_t case_id, name claimant)
{	
	//open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

	require_auth(claimant);
	casefiles_table casefiles(get_self(), get_self().value);
	const auto& cf = casefiles.get(case_id, "Case Not Found");
	check(cf.case_status == case_status::CASE_SETUP, "Cases can only be readied during CASE_SETUP");
	check(claimant == cf.claimant, "you are not the claimant of this case.");
	check(cf.number_claims >= 1, "Cases must have at least one claim");
	

	//TODO: CHECK FEE SYSTEM
	//sub_balance(claimant, asset(conf.fee_structure[0], TLOS_SYM));

	casefiles.modify(cf, get_self(), [&](auto &row) {
		row.case_status = static_cast<uint8_t>(case_status::AWAITING_ARBS);
		row.update_ts = time_point_sec(current_time_point());
	});
}

#pragma endregion Claimant_Actions

#pragma region Case_Actions

void arbitration::addarbs(uint64_t case_id, name assigned_arb, uint8_t num_arbs_to_assign)
{
	require_auth(assigned_arb);
	casefiles_table casefiles(get_self(), get_self().value);
	const auto& cf = casefiles.get(case_id, "Case Not Found");

	auto arb_it = std::find(cf.arbitrators.begin(), cf.arbitrators.end(), assigned_arb);
	check(arb_it != cf.arbitrators.end(), "arbitrator isn't assigned to this case_id");

	/* RATIONALE: If the following validations pass, the trx would be committed irreversible.
	Demux service watch from calls to this action, a side effect of this action being call 
	is to assigntocase N times. */
}

void arbitration::assigntocase(uint64_t case_id, name arb_to_assign)
{
 	//open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

    //authenticate
    require_auth(conf.admin);
	arbitrators_table arbitrators(get_self(), get_self().value);
	const auto& arb = arbitrators.get(arb_to_assign.value, "actor is not a registered Arbitrator");
	check(arb.arb_status != arb_status::REMOVED, "Arbitrator has been removed.");
	check(arb.arb_status != arb_status::SEAT_EXPIRED, "Arbitrator term has expired");
	check(arb.arb_status == arb_status::AVAILABLE, "Arb status isn't set to available, Arbitrator is unable to receive new cases");

	vector<uint64_t> new_open_cases = arb.open_case_ids;
	new_open_cases.emplace_back(case_id);

	arbitrators.modify(arb, same_payer, [&](auto &row) {
		row.open_case_ids = new_open_cases;
	});

	casefiles_table casefiles(get_self(), get_self().value);
	const auto& cf = casefiles.get(case_id, "Case not found with given Case ID");
	check(cf.case_status >= case_status::AWAITING_ARBS, "case file is still in CASE_SETUP");
	check(cf.case_status < case_status::RESOLVED, "case file can not be RESOLVED or DISMISSED");

	check(std::find(cf.arbitrators.begin(), cf.arbitrators.end(), arb_to_assign) == cf.arbitrators.end(),
		  "Arbitrator is already assigned to this case");

	vector<name> new_arbs = cf.arbitrators;
	new_arbs.emplace_back(arb.arb);

	casefiles.modify(cf, same_payer, [&](auto &row) {
		row.arbitrators = new_arbs;

		if(cf.case_status == case_status::AWAITING_ARBS) {
			row.case_status = static_cast<uint8_t>(case_status::CASE_INVESTIGATION);
		}
	});
}

void arbitration::respond(uint64_t case_id, uint64_t claim_id, name respondant, string response_link)
{
	require_auth(respondant);
	validate_ipfs_url(response_link);

	casefiles_table casefiles(get_self(), get_self().value);
	const auto& cf = casefiles.get(case_id, "Case Not Found");

	check(cf.respondant != name(0), "case_id does not have a respondant");
	check(cf.respondant == respondant, "must be the respondant of this case_id");
	check(cf.case_status == case_status::CASE_INVESTIGATION, "case status does NOT allow responses at this time");

	claims_table claims(get_self(), case_id);
	auto claim_it = claims.find(claim_id);
    
	check(claim_it != claims.end(), "Claim not found");

	claims.modify(claim_it, get_self(), [&](auto& col){
		col.response_link = response_link;
		col.status = static_cast<uint8_t>(claim_status::RESPONDED);
	});	
}

void arbitration::dismissclaim(uint64_t case_id, name assigned_arb, uint64_t claim_id, string memo)
{
	require_auth(assigned_arb);

	casefiles_table casefiles(get_self(), get_self().value);
	const auto& cf = casefiles.get(case_id, "Case not found");

	check(cf.case_status == case_status::CASE_INVESTIGATION, "to dismiss a claim, case should be in investigation status");

	auto arb_case = std::find(cf.arbitrators.begin(), cf.arbitrators.end(), assigned_arb);
	check(arb_case != cf.arbitrators.end(), "Only an assigned arbitrator can dismiss a claim");

	assert_string(memo, std::string("memo must be greater than 0 and less than 255"));
	
	claims_table claims(get_self(), case_id);
	auto claim_it = claims.find(claim_id);

	check(claim_it != claims.end(), "Claim not found");
	claims.modify(claim_it, get_self(), [&](auto& col){
		col.decision_link = memo;
		col.status = static_cast<uint8_t>(claim_status::DISMISSED);
	});	

	casefiles.modify(cf, same_payer, [&](auto &cf) {
		cf.update_ts = time_point_sec(current_time_point());
	});
}

void arbitration::acceptclaim(uint64_t case_id, name assigned_arb, uint64_t claim_id,
							  string decision_link, uint8_t decision_class)
{
	require_auth(assigned_arb);
	validate_ipfs_url(decision_link);
	
	casefiles_table casefiles(get_self(), get_self().value);
	const auto& cf = casefiles.get(case_id, "Case not found");

	auto arb_case = std::find(cf.arbitrators.begin(), cf.arbitrators.end(), assigned_arb);
	check(arb_case != cf.arbitrators.end(), "Only the assigned arbitrator can accept a claim");

	//check(decision_class != claim_status::UNDECIDED, "decision_class must be valid [2 - 15]");
	check(cf.case_status == case_status::CASE_INVESTIGATION, "to dismiss a claim, case should be in investigation status");
	
	claims_table claims(get_self(), case_id);
	auto claim_it = claims.find(claim_id);

	check(claim_it != claims.end(), "Claim not found");

	claims.modify(claim_it, get_self(), [&](auto& col){
		col.decision_link = decision_link;
		col.decision_class = decision_class;
		col.status = static_cast<uint8_t>(claim_status::ACCEPTED);
	});	

	casefiles.modify(cf, same_payer, [&](auto &row) {
		row.update_ts = time_point_sec(current_time_point());
	});
	
	action(permission_level{get_self(), name("active")}, get_self(), name("execclaim"), make_tuple(
		claim_id, 
		case_id,
		assigned_arb,
		claim_it->claim_summary,
		decision_link,
		decision_class
	)).send();
}

void arbitration::execclaim(uint64_t new_claim_id, uint64_t case_id, name assigned_arb, string claim_hash,
				string decision_link, uint8_t decision_class) {
	require_auth(get_self());
}

void arbitration::setruling(uint64_t case_id, name assigned_arb, string case_ruling) {
	require_auth(assigned_arb);
	casefiles_table casefiles(get_self(), get_self().value);
	const auto& cf = casefiles.get(case_id, "Case not found with given Case ID");
	check(cf.case_status == case_status::ENFORCEMENT, "case_status must be ENFORCEMENT");

	auto arb_it = std::find(cf.arbitrators.begin(), cf.arbitrators.end(), assigned_arb);
	check(arb_it != cf.arbitrators.end(), "arbitrator is not assigned to this case_id");
	validate_ipfs_url(case_ruling);

	casefiles.modify(cf, same_payer, [&](auto& row) {
		row.case_ruling = case_ruling;
	});
}

void arbitration::advancecase(uint64_t case_id, name assigned_arb)
{
	require_auth(assigned_arb);

	casefiles_table casefiles(get_self(), get_self().value);
	const auto& cf = casefiles.get(case_id, "Case not found with given Case ID");
	check(cf.case_status > case_status::AWAITING_ARBS, "case_status must be greater than AWAITING_ARBS");
	check(cf.case_status < case_status::RESOLVED && cf.case_status != case_status::DISMISSED, "Case has already been resolved or dismissed");
	if (cf.case_status + 1 == case_status::RESOLVED) {
		check(cf.case_ruling != string(""), "Case Ruling must be set before advancing case to RESOLVED status");
	}

	auto arb_it = std::find(cf.arbitrators.begin(), cf.arbitrators.end(), assigned_arb);
	check(arb_it != cf.arbitrators.end(), "actor is not assigned to this case_id");

	auto approval_it = std::find(cf.approvals.begin(), cf.approvals.end(), assigned_arb);
	check(approval_it == cf.approvals.end(), "arbitrator has already approved advancing this case");

	auto case_status = cf.case_status;
	auto approvals = cf.approvals;

	if (approvals.size() + 1 < cf.arbitrators.size()) {
		approvals.emplace_back(assigned_arb);
	} else if (cf.approvals.size() + 1 >= cf.arbitrators.size()) {
		case_status++;
		approvals.clear();
	}

	casefiles.modify(cf, same_payer, [&](auto &row) {
		row.case_status = case_status;
		row.approvals = approvals;
	});
}

void arbitration::dismisscase(uint64_t case_id, name assigned_arb, string ruling_link)
{
	require_auth(assigned_arb);
	validate_ipfs_url(ruling_link);

	casefiles_table casefiles(get_self(), get_self().value);
	const auto& cf = casefiles.get(case_id, "No case found with given case_id");

	auto arb_case = std::find(cf.arbitrators.begin(), cf.arbitrators.end(), assigned_arb);
	check(arb_case != cf.arbitrators.end(), "Arbitrator isn't selected for this case");
	check(cf.case_status == case_status::CASE_INVESTIGATION, "Case is already dismissed or complete");

	casefiles.modify(cf, same_payer, [&](auto &row) {
		row.case_status = static_cast<uint8_t>(case_status::DISMISSED);
		row.case_ruling = ruling_link;
		row.update_ts = time_point_sec(current_time_point());
	});
}

void arbitration::recuse(uint64_t case_id, string rationale, name assigned_arb)
{
	require_auth(assigned_arb);

	casefiles_table casefiles(get_self(), get_self().value);
	const auto& cf = casefiles.get(case_id, "No case found for given case_id");

	check(cf.case_status > case_status::AWAITING_ARBS && cf.case_status < case_status::RESOLVED, 
		"unable to recuse if the case is resolved");	

	auto arb_case = std::find(cf.arbitrators.begin(), cf.arbitrators.end(), assigned_arb);
	check(arb_case != cf.arbitrators.end(), "Arbitrator isn't selected for this case.");

	assert_string(rationale, std::string("rationale must be greater than 0 and less than 255"));

	vector<name> new_arbs = cf.arbitrators;
	auto arb_it = find(new_arbs.begin(), new_arbs.end(), assigned_arb);
	new_arbs.erase(arb_it);

	casefiles.modify(cf, same_payer, [&](auto &row) {
		row.arbitrators = new_arbs;
		row.update_ts = time_point_sec(current_time_point());
	});
}

#pragma endregion Case_Actions

#pragma region Arb_Actions

void arbitration::newarbstatus(name arbitrator, uint8_t new_status)
{
	require_auth(arbitrator);

	arbitrators_table arbitrators(_self, _self.value);
	const auto& arb = arbitrators.get(arbitrator.value, "Arbitrator not found");

	check(arb.arb_status != arb_status::SEAT_EXPIRED && arb.arb_status != arb_status::REMOVED, "Arbitrator must be active");
	check(time_point_sec(current_time_point()) < arb.term_expiration, "Arbitrator term expired");

	check(new_status == arb_status::AVAILABLE || new_status == arb_status::UNAVAILABLE, 
	"Supplied status code is invalid for this action");

	arbitrators.modify(arb, same_payer, [&](auto &row) {
		row.arb_status = new_status;
	});
}

void arbitration::setlangcodes(name arbitrator, vector<uint16_t> lang_codes)
{
	require_auth(arbitrator);
	arbitrators_table arbitrators(get_self(), get_self().value);
	const auto& arb = arbitrators.get(arbitrator.value, "Arbitrator not found");

	check(arb.arb_status != arb_status::SEAT_EXPIRED && arb.arb_status != arb_status::REMOVED, "Arbitrator must be active");
	check(time_point_sec(current_time_point()) < arb.term_expiration, "Arbitrator term expired");

	arbitrators.modify(arb, same_payer, [&](auto& a) {
		a.languages = lang_codes;
	});
}

void arbitration::deletecase(uint64_t case_id)
{
	require_auth( permission_level{ get_self(), "major"_n } );

	casefiles_table casefiles(get_self(), get_self().value);

	const auto& cf = casefiles.get(case_id, "case file not found");
	check(cf.case_status >= case_status::RESOLVED, "case must either be RESOLVED or DISMISSED");
	casefiles.erase(cf);

	claims_table claims(get_self(), case_id);
	auto claim_it = claims.begin();
	while(claim_it != claims.end()) {
		claim_it = claims.erase(claim_it);
	}
}

#pragma endregion Arb_Actions

#pragma region BP_Multisig_Actions

	void arbitration::dismissarb(name arb, bool remove_from_cases)
	{
		require_auth("eosio"_n);
		check(is_account(arb), "arb must be account");

		arbitrators_table arbitrators(get_self(), get_self().value);

		const auto& to_dismiss = arbitrators.get(arb.value, "arbitrator not found");

		check(to_dismiss.arb_status != arb_status::SEAT_EXPIRED && to_dismiss.arb_status != arb_status::REMOVED, 
			"arbitrator is already removed or their seat has expired");

		arbitrators.modify(to_dismiss, same_payer, [&](auto& a) {
			a.arb_status = static_cast<uint8_t>(arb_status::REMOVED);
		});

		auto perms = get_arb_permissions();
		set_permissions(perms);

		if(remove_from_cases) {
			casefiles_table casefiles(get_self(), get_self().value);

			for(const auto &id: to_dismiss.open_case_ids) {
				auto cf_it = casefiles.find(id);

				if (cf_it != casefiles.end()) {
					auto case_arbs = cf_it->arbitrators;
					auto arb_it = find(case_arbs.begin(), case_arbs.end(), to_dismiss.arb);

					if (arb_it != case_arbs.end() && cf_it->case_status < case_status::RESOLVED) {
						case_arbs.erase(arb_it);
						casefiles.modify(cf_it, same_payer, [&](auto &row) {
							row.arbitrators = case_arbs;
						});
					}
				}
			}

			auto open_ids = to_dismiss.open_case_ids;
			open_ids.clear();

			arbitrators.modify(to_dismiss, same_payer, [&](auto& a) {
				a.arb_status = static_cast<uint8_t>(arb_status::REMOVED);
				a.open_case_ids = open_ids;
			});
		}
	}
#pragma endregion BP_Multisig_Actions

#pragma region Helpers

typedef arbitration::claim claim;


void arbitration::validate_ipfs_url(string ipfs_url)
{
	check(ipfs_url.length() == 46 || ipfs_url.length() == 49, "invalid ipfs string, valid schema: <hash>");
}

void arbitration::assert_string(string to_check, string error_msg)
{
	check(to_check.length() > 0 && to_check.length() < 255, error_msg.c_str());
}


void arbitration::start_new_election(uint8_t available_seats, bool runoff, string content)
{
	elections_table elections(_self, _self.value);
    uint64_t new_election_id = elections.available_primary_key();

	//open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

	conf.current_election_id = new_election_id;

	//set new config
    configs.set(conf, _self);

	auto now = time_point_sec(current_time_point());
	auto end_ts = runoff ? now : now + conf.election_add_candidates_ts;
    elections.emplace(get_self(), [&]( auto& col ) {
        col.election_id = new_election_id;
        col.info_url = std::string(content);
        col.candidates = vector<candidate> {};
        col.available_seats = available_seats;
		col.begin_add_candidates_ts = now;
        col.end_add_candidates_ts = end_ts;
        col.status = static_cast<uint8_t>(election_status::CREATED);
    });
}

bool arbitration::has_available_seats(arbitrators_table &arbitrators, uint8_t &available_seats)
{
	uint8_t occupied_seats = 0;

	//open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

	for (auto &arb : arbitrators)
	{
		// check if arb seat is expired
		if (time_point_sec(current_time_point()) > arb.term_expiration 
		&& arb.arb_status != arb_status::SEAT_EXPIRED)
		{
			arbitrators.modify(arb, same_payer, [&](auto &a) {
				a.arb_status = static_cast<uint8_t>(arb_status::SEAT_EXPIRED);
			});
		}

		if (arb.arb_status != arb_status::SEAT_EXPIRED && arb.arb_status != arb_status::REMOVED) {
			occupied_seats++;
		}
	}

	available_seats = uint8_t(conf.max_elected_arbs - occupied_seats);

	return available_seats > 0;
}

vector<arbitration::permission_level_weight> arbitration::get_arb_permissions() {
	arbitrators_table arbitrators(get_self(), get_self().value);
	vector<permission_level_weight> perms;
	for(const auto &a: arbitrators) {
		if (a.arb_status != arb_status::SEAT_EXPIRED && a.arb_status != arb_status::REMOVED)
		{
			perms.emplace_back(permission_level_weight{permission_level{a.arb, "active"_n}, 1});
		}
	}
	return perms;
}

void arbitration::set_permissions(vector<permission_level_weight> &perms) {
	//review update auth permissions and weights.
	if (perms.size() > 0)
	{
		sort(perms.begin(), perms.end(), [](const auto &first, const auto &second) 
			{ return first.permission.actor.value < second.permission.actor.value; });

		uint32_t weight = perms.size() > 3 ? (((2 * perms.size()) / uint32_t(3)) + 1) : 1;

		action(permission_level{get_self(), "active"_n}, "eosio"_n, "updateauth"_n,
				std::make_tuple(
					get_self(),
					"major"_n,
					"active"_n,
					authority{
						weight,
						std::vector<key_weight>{},
						perms,
						std::vector<wait_weight>{}}))
			.send();
	}
}

void arbitration::add_arbitrator(arbitrators_table &arbitrators, name arb_name, string credential_link)
{	
	//open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto _config = configs.get();

	auto arb = arbitrators.find(arb_name.value);
	if (arb == arbitrators.end())
	{
		arbitrators.emplace(_self, [&](auto &a) {
			a.arb = arb_name;
			a.arb_status = static_cast<uint8_t>(arb_status::UNAVAILABLE);
			a.elected_time = time_point_sec(current_time_point());
			a.term_expiration = time_point_sec(current_time_point().sec_since_epoch() + _config.arb_term_length);
			a.open_case_ids = vector<uint64_t>();
			a.closed_case_ids = vector<uint64_t>();
			a.credentials_link = credential_link;
		});
	}
	else
	{
		arbitrators.modify(arb, same_payer, [&](auto &a) {
			a.arb_status = static_cast<uint8_t>(arb_status::UNAVAILABLE);
			a.elected_time = time_point_sec(current_time_point());
			a.term_expiration = time_point_sec(current_time_point().sec_since_epoch() + _config.arb_term_length);
			a.credentials_link = credential_link;
		});
	}
}


void arbitration::sub_balance(name owner, asset value){
	accounts_table from_acnts(_self, owner.value);

	const auto &from = from_acnts.get(value.symbol.code().raw(), "no balance object found");
	check(from.balance.amount >= value.amount, "overdrawn balance");

	if (from.balance - value == asset(0, value.symbol))
	{
		from_acnts.erase(from);
	}
	else
	{
		from_acnts.modify(from, owner, [&](auto &a) {
			a.balance -= value;
		});
	}
}

void arbitration::add_balance(name owner, asset value, name ram_payer){
	accounts_table to_acnts(_self, owner.value);
	auto to = to_acnts.find(value.symbol.code().raw());
	if (to == to_acnts.end())
	{
		to_acnts.emplace(ram_payer, [&](auto &a) {
			a.balance = value;
		});
	}
	else
	{
		to_acnts.modify(to, same_payer, [&](auto &a) {
			a.balance += value;
		});
	}
}

checksum256 arbitration::get_rngseed(uint64_t seed)
{
  auto trxsize = transaction_size();
  char* trxbuf = (char*) malloc(trxsize);   // no need to free as it's executed once and VM gets destroyed
  uint32_t trxread = read_transaction( trxbuf + 8, trxsize );
  check( trxsize == trxread, "read_transaction failed");
  *((uint64_t*)trxbuf) = seed;
  return sha256(trxbuf, trxsize + 8);
}


//Get Random Number (based in WAX RNG contract example)
inline uint64_t get_rand() {
  auto size = transaction_size();
  char buf[size];

  auto read = read_transaction(buf, size);
  check(size == read, "read_transaction() has failed.");

  checksum256 tx_id = sha256(buf, size); 
  uint64_t signing_value;
  memcpy(&signing_value, tx_id.data(), sizeof(signing_value)); //Converting checksum256 to uint64_t
  return signing_value;
}

string arbitration::get_rand_ballot_name() {
	RandomnessProvider randomness_provider(get_rngseed(get_rand()));

	string ballot_name = "";

	string possibleCharacters = string("12345abcdefghijklmnopqrstuvwxyz");
	for(auto i = 0; i < 12; ++i) {
		uint32_t rand = randomness_provider.get_rand(possibleCharacters.length() - 1); 
		ballot_name += possibleCharacters[rand];
	}

	return ballot_name;
}


#pragma endregion Helpers

#pragma region Notification_handlers

void arbitration::transfer_handler(name from, name to, asset quantity, string memo){
	require_auth(from);

	check(quantity.is_valid(), "Invalid quantity");
	check(quantity.symbol == symbol("TLOS", 4), "only TLOS tokens are accepted by this contract");

	if (from == get_self()) {
		if(to != TELOSDECIDE) sub_balance(from, quantity);
		return;
	}

	check(to == get_self(), "to must be self");

	if(memo == string("skip")) {
		//open config singleton, get config
		config_singleton configs(get_self(), get_self().value);
		auto conf = configs.get();

		conf.available_funds += quantity;
		
		configs.set(conf, get_self());

	} else {
		accounts_table accounts(get_self(), from.value);
		const auto &from_bal = accounts.find(quantity.symbol.code().raw());

		add_balance(from, quantity, get_self());
	}
}

void arbitration:: catch_broadcast(name ballot_name, map<name, asset> final_results, uint32_t total_voters) {
	
	//get initial receiver contract
    name rec = get_first_receiver();

	//open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();


    //if notification not from decide account
    if (rec != TELOSDECIDE) {
        return;
    }

    elections_table elections(get_self(), get_self().value);
    auto elections_by_ballot = elections.get_index<name("byballot")>();
    auto by_ballot_itr = elections_by_ballot.find(ballot_name.value);


    if (by_ballot_itr != elections_by_ballot.end()) {

		vector<pair<name, asset>> pairs;
        for (auto itr = final_results.begin(); itr != final_results.end(); ++itr){
            pairs.push_back(*itr);
        }

        sort(pairs.begin(), pairs.end(), [=](pair<name, asset>& a, std::pair<name, asset>& b)
        {
            return a.second.amount > b.second.amount;
        });

		auto available_seats = by_ballot_itr->available_seats;

		uint8_t number_tied_candidates = 0;

		vector<name> tied_candidates;

		uint8_t seats_occupied = available_seats;

		//find if there are any tied candidates
		if (pairs[available_seats].second.amount > 0){
			auto first_candidate_out = pairs[available_seats];
			int i = available_seats + 1;

			while(i < pairs.size() && pairs[i].second.amount == first_candidate_out.second.amount) {
				++number_tied_candidates;
				tied_candidates.push_back(pairs[i].first);
				++i;
			}

			int j = available_seats - 1;
			
			while(j >= 0 && pairs[j].second.amount == first_candidate_out.second.amount) {
				++number_tied_candidates;
				tied_candidates.push_back(pairs[j].first);
				--j;
				--seats_occupied;
			}		
		}

		if(tied_candidates.size() > 0) {
			tied_candidates.push_back(pairs[available_seats].first);
			++number_tied_candidates;
		} 

		nominees_table nominees(get_self(), get_self().value);
	
		arbitrators_table arbitrators(get_self(), get_self().value);

		vector<candidate> candidates_results = vector<candidate>{};

		for(int i = 0; i < pairs.size(); ++i) {
			name candidate_name = pairs[i].first;
			auto candidate_itr = nominees.find(candidate_name.value);

			if(candidate_itr != nominees.end()) {
				auto cand_credential = candidate_itr->credentials_link;
				auto cand_votes = pairs[i].second.amount;
				
				if(i >= seats_occupied) {
					if(std::find(tied_candidates.begin(), tied_candidates.end(), candidate_name) == tied_candidates.end()) {
						nominees.erase(candidate_itr);
					}
				} else {
					add_arbitrator(arbitrators, candidate_name, cand_credential);
					nominees.erase(candidate_itr);
				}
				
				candidates_results.push_back(candidate{pairs[i].first, pairs[i].second});
			}
		}

		elections_by_ballot.modify(by_ballot_itr, get_self(), [&](auto& col){
			col.candidates = candidates_results;
			col.status = static_cast<uint8_t>(election_status::ENDED);
		});

		auto perms = get_arb_permissions();
		set_permissions(perms);
		
		//start new election if there were tied candidates
		if (number_tied_candidates > 0) {
			
			uint8_t remaining_available_seats = available_seats - seats_occupied;

			start_new_election(remaining_available_seats, true, by_ballot_itr->info_url);

			elections_table elections(get_self(), get_self().value);
			auto& election = elections.get(conf.current_election_id + 1, "Election doesn't exist");

			for(auto i = 0; i < tied_candidates.size(); ++i){
				auto nominee = nominees.find(tied_candidates[i].value);

				elections.modify(election, same_payer, [&]( auto& col ) {
					col.candidates.push_back(candidate{tied_candidates[i], asset(0, VOTE_SYM)});
				});
			}
			
			auto ballot_name = get_rand_ballot_name();
  	
			action(permission_level{get_self(), name("active")}, get_self(), name("beginvoting"), make_tuple(
				string(ballot_name), //ballot name
				true //runoff
			)).send();
		}
	}
}

#pragma endregion Notification_handlers

#pragma region Test_Actions

//======================= test actions ====================

void arbitration::skiptovoting(){
    
	//open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

	//open elections table
    elections_table elections(get_self(), get_self().value);
	auto& election = elections.get(conf.current_election_id, "Election doesn't exist");
	
	time_point_sec now = time_point_sec(current_time_point());

	//update election
    elections.modify(election, get_self(), [&](auto& col) {
        col.end_add_candidates_ts = now;
    });

}


#pragma endregion Test_Actions