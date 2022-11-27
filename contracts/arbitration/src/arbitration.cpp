/**
 * Arbitration Contract Implementation. See function bodies for further notes.
 * 
 * @author Roger Taulé, Craig Branscom, Peter Bue, Ed Silva
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
	uint32_t election_add_candidates_duration, uint32_t arbitrator_term_length, uint8_t max_claims_per_case, 
	asset fee_usd, uint32_t claimant_accepting_offers_duration)
{
	//open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

	//authenticate
	require_auth(conf.admin);

	//Configuration checks
	check(max_elected_arbs > 0, "Arbitrators must be greater than 0");
	check(max_claims_per_case > 0, "Minimum 1 claim");
	check(fee_usd.symbol == USD_SYM, "Fee must be set in USD");

	conf.max_elected_arbs = max_elected_arbs;
	conf.election_voting_ts = election_duration;
	conf.election_add_candidates_ts = election_add_candidates_duration;
	conf.arb_term_length = arbitrator_term_length;
	conf.runoff_election_voting_ts = runoff_duration;
	conf.max_claims_per_case = max_claims_per_case;
	conf.claimant_accepting_offers_ts = claimant_accepting_offers_duration;
	conf.fee_usd = fee_usd;

	//set new config
    configs.set(conf, get_self());
}


#pragma endregion Config_Actions

#pragma region Arb_Elections

void arbitration::initelection(string election_info)
{
	//open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

	//authenticate
	require_auth(conf.admin);

	//Open elections table, get the last election
	elections_table elections(get_self(), get_self().value);
	auto last_election = elections.find(conf.current_election_id);
	
	//Check that there isn't any election live
	check(last_election == elections.end() || last_election->status == election_status::ENDED, "There is an election already running");	

	//Open arbitrators table
	arbitrators_table arbitrators(get_self(), get_self().value);

	//Check that there are seats available 
	uint8_t available_seats = 0;
	check(has_available_seats(arbitrators, available_seats), "No seats are available!");
	
	start_new_election(available_seats, false, election_info);
	
}

void arbitration::regarb(name nominee, string credentials_link)
{
	//Check that new nominee is a valid account
	check(is_account(nominee), "New candidate is not an account");

	//authenticate
	require_auth(nominee);

	//Check that the credentials link is a valid IPFS Hash
	validate_ipfs_url(credentials_link);

	//Open nominees table, check that the nominee is not already registered
	nominees_table nominees(get_self(), get_self().value);
	auto nom_itr = nominees.find(nominee.value);
	check(nom_itr == nominees.end(), "Nominee is already an applicant");

	//Open arbitrators table
	arbitrators_table arbitrators(get_self(), get_self().value);
	auto arb_itr = arbitrators.find(nominee.value);

	//If nominee is already an arbitrator, check that he was removed or his seat is expired. 
	//If the arbitrator has a valid term is not eligible fo the election
	if (arb_itr != arbitrators.end()) {
		check(time_point_sec(current_time_point()) > arb_itr->term_expiration || 
		arb_itr->arb_status == arb_status::SEAT_EXPIRED || arb_itr->arb_status == arb_status::REMOVED,
			"Nominee is already an Arbitrator and the seat hasn't expired or been removed");

		//NOTE: set arb_status to SEAT_EXPIRED until re-election
		arbitrators.modify(arb_itr, same_payer, [&](auto &col) {
			col.arb_status = static_cast<uint8_t>(arb_status::SEAT_EXPIRED);
		});
	}

	//Register the nominee
	nominees.emplace(nominee, [&](auto &col) {
		col.nominee_name = nominee;
		col.credentials_link = credentials_link;
		col.application_time = time_point_sec(current_time_point());
	});
}

void arbitration::unregnominee(name nominee)
{
	//authenticate
	require_auth(nominee);

	//open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto _config = configs.get();

	//Open nominees table, check that the nominee is registered
	nominees_table nominees(get_self(), get_self().value);
	auto nom_itr = nominees.find(nominee.value);
	check(nom_itr != nominees.end(), "Nominee isn't an applicant");

	//Open elections table
	elections_table elections(get_self(), get_self().value);
	auto election = elections.find(_config.current_election_id);

	//A nominee can only be removed if it is not already registered as a current election candidate
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
	//authenticate
	require_auth(nominee);
	
	//open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto _config = configs.get();

	//open elections table, get current election
	elections_table elections(get_self(), get_self().value);
	auto& election = elections.get(_config.current_election_id, "Election doesn't exist");

	//Candidates can only be added if the election isn't live and the period for adding candidates hasn't ended.
	check(election.status == election_status::CREATED, "Election needs to be in CREATED status to add a new candidate");
	check(time_point_sec(current_time_point()) < election.end_add_candidates_ts, "Cannot add candidates if period has ended");

	//open nominees table, check if the nominee is registered
	nominees_table nominees(get_self(), get_self().value);
	auto nom_itr = nominees.find(nominee.value);
	check(nom_itr != nominees.end(), "Nominee isn't an applicant. Use regarb action to register as a nominee");

    auto existing_candidate = std::find_if(election.candidates.begin(), election.candidates.end(), [&](auto &candidate) {
		return candidate.name == nominee;
	});
	
	//A candidate can not be added twice
    check(existing_candidate == election.candidates.end(), "Candidate already added");

	//Update the candidates list in the election table
    elections.modify(election, same_payer, [&]( auto& col ) {
        col.candidates.push_back(candidate{
        	nominee,
        	asset(0, VOTE_SYM),
		});
    });
}

void arbitration::candrmvlead(name nominee)
{
	//authenticate
	require_auth(nominee);

	//open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto _config = configs.get();

	//open elections table, get current election
	elections_table elections(get_self(), get_self().value);
	auto& election = elections.get(_config.current_election_id, "Election doesn't exist");

	//Candidates can only be removed if the election isn't live and the period for adding candidates hasn't ended.
	check(election.status == election_status::CREATED, "Election needs to be in CREATED status to remove a new candidate");
	check(time_point_sec(current_time_point()) < election.end_add_candidates_ts, "Cannot remove candidates if period has ended");

    bool found = false;

	//To be removed, a candidate needs to be added in the list of candidates that can be found in the election table
	auto new_candidates = election.candidates;
    for (auto i = 0; i < new_candidates.size(); ++i) {
        if (new_candidates[i].name == nominee) {
            new_candidates.erase(new_candidates.begin() + i);
            found = true;
            break;
        }
    }

    check(found == true, "Candidate not found in election list");

	//Update the election's candidates list
	elections.modify(election, same_payer, [&]( auto& col ) {
		col.candidates = new_candidates;
	});
}

void arbitration::beginvoting(name ballot_name, bool runoff) {
	
	//open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

	//open elections table, get current election
	elections_table elections(get_self(), get_self().value);
	auto& election = elections.get(conf.current_election_id, "Election doesn't exist");

    //authenticate
    check(has_auth(conf.admin) || has_auth(get_self()), "Only admin and SC account can start a voting process");
	
    time_point_sec now = time_point_sec(current_time_point());
    
    //A ballot can only be started if the election isn't already live and the period for adding candidates has ended.
    check(election.status == election_status::CREATED, "Election must be in created status");
    check(time_point_sec(current_time_point()) >= election.end_add_candidates_ts, "Can't start voting process until add candidates time has ended.");
    
	//A ballot will only be done if there are more candidates than available seats. 
	//Otherwise, all the candidates will be automatically assigned as arbitrators
	if(election.candidates.size() > election.available_seats) {

		//Check that the ballot name is unique
    	auto elections_by_ballot = elections.get_index<name("byballot")>();
    	auto by_ballot_itr = elections_by_ballot.find(ballot_name.value);
    	check(by_ballot_itr == elections_by_ballot.end(), "Ballot name already used");

		//Set the ballot end time
		time_point_sec ballot_end_time = !runoff ? now + conf.election_voting_ts : now + conf.runoff_election_voting_ts;
    
    	//Get ballot options
    	vector<name> ballot_options = {};
    
		//Add all the candidates to the ballot
		for (auto i = 0; i < election.candidates.size(); ++i) {
		ballot_options.push_back(election.candidates[i].name);
		}

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

		//Check that the Smart Contract has enough funds to pay for the Telos.decide fee
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
		//If there were less or equal candidates than available seats, add those candidates as arbitrators
		//and remove them from the nominee table

		nominees_table nominees(get_self(), get_self().value);
		arbitrators_table arbitrators(get_self(), get_self().value);

		for(int i = 0; i < election.candidates.size(); ++i) {
			name candidate_name = election.candidates[i].name;
			auto& candidate_itr = nominees.get(candidate_name.value);
			auto cand_credential = candidate_itr.credentials_link;

			add_arbitrator(arbitrators, candidate_name, cand_credential);
			nominees.erase(candidate_itr);
			
		}

		//Set the corresponding permissions for the arbitrators
		auto perms = get_arb_permissions();
		set_permissions(perms);

		//update election as finished
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
	//authenticate
	require_auth(owner);

	//open accounts table, get balance for the owner
	accounts_table accounts(get_self(), owner.value);
	const auto &bal = accounts.get(TLOS_SYM.code().raw(), "balance does not exist");

	//Transfer funds from the Smart Contract to the owner
	action(permission_level{get_self(), name("active")}, name("eosio.token"), name("transfer"),
		   make_tuple(get_self(),
					  owner,
					  bal.balance,
					  std::string("Telos Arbitration withdrawal")))
		.send();

	accounts.erase(bal);
}

void arbitration::filecase(name claimant, string claim_link, vector<uint16_t> lang_codes, std::optional<name> respondant, uint8_t claim_category)
{
	//authenticate
	require_auth(claimant);

	//Check that tha claim_link is a valid IPFS Hash
	validate_ipfs_url(claim_link);

	//If a respondant is added, need to check that it is a valid account
	if(respondant) {
		check(is_account(*respondant), "Respondant must be an account");
	}

	//Check that the claim category is valid
	check(claim_category <= claim_category::MISC && claim_category >= claim_category::LOST_KEY_RECOVERY, "Claim category not found");

	//open casefiles table
	casefiles_table casefiles(get_self(), get_self().value);
	uint64_t new_case_id = casefiles.available_primary_key();

	//Create a new case file
	casefiles.emplace(claimant, [&](auto &col) {
		col.case_id = new_case_id;
		col.case_status = static_cast<uint8_t>(case_status::CASE_SETUP);
		col.claimant = claimant;
		col.respondant = *respondant;
		col.arbitrators = {};
		col.approvals = {};
		col.required_langs = lang_codes;
		col.number_claims = 1;
		col.case_ruling = std::string("");
		col.update_ts = time_point_sec(current_time_point());
	});

	//open claims table
	claims_table claims(get_self(), new_case_id);
	uint64_t new_claim_id = claims.available_primary_key();

	//Create a new claim
	claims.emplace(claimant, [&](auto& col){
		col.claim_id = new_claim_id;
		col.claim_summary = claim_link;
		col.claim_category = claim_category;
		col.status = static_cast<uint8_t>(claim_status::FILED);
	});
}

void arbitration::addclaim(uint64_t case_id, string claim_link, name claimant, uint8_t claim_category)
{
	//authenticate
	require_auth(claimant);

	//Check that tha claim_link is a valid IPFS Hash
	validate_ipfs_url(claim_link);

	//open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

	//open casefile tables and checks that the case exists
	casefiles_table casefiles(get_self(), get_self().value);
	const auto& cf = casefiles.get(case_id, "Case Not Found");

	//Check that the claim category is valid
	check(claim_category <= claim_category::MISC && claim_category >= claim_category::LOST_KEY_RECOVERY, "Claim category not found");
	
	//Only the claimant can add new claims, the number of claims cannot exceed the maximum permitted and case must be in setup
	check(cf.case_status == case_status::CASE_SETUP, "claims cannot be added after CASE_SETUP is complete.");
	check(cf.number_claims < conf.max_claims_per_case, "case file has reached maximum number of claims");
	check(claimant == cf.claimant, "you are not the claimant of this case.");

	//open claims table
	claims_table claims(get_self(), case_id);

	//Check that two different claims doesn't have the same link
	for(auto itr = claims.begin(); itr != claims.end(); ++itr) {
		check(itr->claim_summary != claim_link, "Claim Link already exists in another claim");
	}

	//Creates a new claim
	uint64_t new_claim_id = claims.available_primary_key();
	claims.emplace(claimant, [&](auto& col){
		col.claim_id = new_claim_id;
		col.claim_summary = claim_link;
		col.claim_category = claim_category;
		col.status = static_cast<uint8_t>(claim_status::FILED);
	});
	
	//Update casefile table
	casefiles.modify(cf, same_payer, [&](auto &col) {
		col.number_claims += 1;
		col.update_ts = time_point_sec(current_time_point());
	});
}

void arbitration::updateclaim(uint64_t case_id, uint64_t claim_id, name claimant, string claim_link)
{
	//authenticate
	require_auth(claimant);

	//Check that tha claim_link is a valid IPFS Hash
	validate_ipfs_url(claim_link);

	//open casefile tables and checks that the case exists
	casefiles_table casefiles(get_self(), get_self().value);
	const auto& cf = casefiles.get(case_id, "Case not found");

	//Only the claimant can update the claim. A claim can only be updated in case investigation
	//or case setup
	check(cf.claimant == claimant, "must be the claimant of this case_id");
	check(cf.case_status == case_status::CASE_INVESTIGATION 
		|| cf.case_status == case_status::CASE_SETUP, "case status does NOT allow responses at this time");

	//open claims table and checks that the claim exists
	claims_table claims(get_self(), case_id);
	auto claim_it = claims.find(claim_id);
	check(claim_it != claims.end(), "Claim not found");

	//To update a claim, claim_status cannot be accepted nor declined
	check(claim_it->status == claim_status::FILED || 
		(claim_it->claim_info_needed && claim_it->status == claim_status::RESPONDED), "Claim cannot be updated");

	//Update a claim
	claims.modify(claim_it, get_self(), [&](auto& col){
		col.claim_summary = claim_link;
		col.claim_info_needed = false;
	});	
}

void arbitration::removeclaim(uint64_t case_id, uint64_t claim_id, name claimant)
{	
	//authenticate
	require_auth(claimant);

	//open casefile tables and checks that the case exists
	casefiles_table casefiles(get_self(), get_self().value);
	const auto& cf = casefiles.get(case_id, "Case Not Found");

	//Only the claimant can remove a claim and case must be in setup status to do so
	check(cf.case_status == case_status::CASE_SETUP, "Claims cannot be removed after CASE_SETUP is complete");
	check(claimant == cf.claimant, "you are not the claimant of this case.");
	
	//open claims table and checks that the claim exists
	claims_table claims(get_self(), case_id);
	auto claim_it = claims.find(claim_id);
	check(claim_it != claims.end(), "Claim not found");

	//erase the claim
	claims.erase(claim_it);

	//Update casefile table
	casefiles.modify(cf, same_payer, [&](auto &col) {
		col.update_ts = time_point_sec(current_time_point());
		col.number_claims -= 1;
	});
}

void arbitration::shredcase(uint64_t case_id, name claimant)
{	
	//authenticate
	require_auth(claimant);

	//open casefile tables and checks that the case exists
	casefiles_table casefiles(get_self(), get_self().value);
	auto c_itr = casefiles.find(case_id);
	check(c_itr != casefiles.end(), "Case Not Found");

	//Only the claimant can shred the case and it must be in setup status to do so
	check(claimant == c_itr->claimant, "you are not the claimant of this case.");
	check(c_itr->case_status == case_status::CASE_SETUP, "cases can only be shredded during CASE_SETUP");
	
	//Open the claims table for the case and erase all the claims
	claims_table claims(get_self(), case_id);
	auto claim_it = claims.begin();
	while(claim_it != claims.end()) {
		claim_it = claims.erase(claim_it);
	}

	//erase the case
	casefiles.erase(c_itr);
}

void arbitration::readycase(uint64_t case_id, name claimant)
{	
	//open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();

	//authenticate 
	require_auth(claimant);

	//open casefile tables and checks that the case exists
	casefiles_table casefiles(get_self(), get_self().value);
	const auto& cf = casefiles.get(case_id, "Case Not Found");

	//To ready a case, it has to be done by the claimant, the case must have at least a claim and must be in setup status
	check(cf.case_status == case_status::CASE_SETUP, "Cases can only be readied during CASE_SETUP");
	check(claimant == cf.claimant, "you are not the claimant of this case.");
	check(cf.number_claims >= 1, "Cases must have at least one claim");
	
	//Get the TLOS-USD conversion through delphioracle
	auto tlosusd = tlosusdprice();

	//Calculates the fee amount the claimant needs to pay in TLOS to ready the case and substract that balance from his account
	float tlos_fee_amount = float(conf.fee_usd.amount*10000 / tlosusd);
    auto tlos_fee = asset(tlos_fee_amount, TLOS_SYM);
	sub_balance(claimant, tlos_fee);

	//The fee amount will be added to the reserved funds, because in some scenarios it might be returned
	//In particular, the fee will be returned in the case that there's a mistrial, case is dismissed or claimant decides to cancel a case 
	//before any arbitator makes an offer
	conf.reserved_funds += tlos_fee;
		
	configs.set(conf, get_self());

	//Update casefile table
	casefiles.modify(cf, get_self(), [&](auto &col) {
		col.case_status = static_cast<uint8_t>(case_status::AWAITING_ARBS);
		col.update_ts = time_point_sec(current_time_point());
		col.sending_offers_until_ts = time_point_sec(current_time_point().sec_since_epoch() + conf.claimant_accepting_offers_ts);
		col.fee_paid_tlos = tlos_fee;
	});
}


void arbitration::respondoffer(uint64_t case_id, uint64_t offer_id, bool accept) {

	//open config singleton, get config
	config_singleton configs(get_self(), get_self().value);
	auto conf = configs.get();

	//open casefile tables and checks that the case exists
	casefiles_table casefiles(get_self(), get_self().value);
	const auto& cf = casefiles.get(case_id, "Case not found");

	//To respond an offer, case must be awaiting for arbitrators
	check(cf.case_status == case_status::AWAITING_ARBS, "Case needs to be in AWAITING ARBS status");

	//authenticate
	require_auth(cf.claimant);

	//open offers table and checks that the offer exists, belongs to the case ID and it has not been responded yet
	offers_table offers(get_self(), get_self().value);
	const auto& offer = offers.get(offer_id, "Offer not found");
	check(offer.case_id == case_id, "Offer does not belong to the case ID");
	check(offer.status == offer_status::PENDING, "Offer needs to be pending to be responded");

	//If the claimant accepts the offer, he will need to pay in advance all the arbitrator rate. 
	//The arbitrator will not receive those funds until the case is resolved and validated by the BPs
	//If an offer is accepted, all other offers will automatically be declined
	if(accept) {
		//Calculate the total USD cost
		auto usd_total_cost = offer.estimated_hours*offer.hourly_rate;

		//Get the TLOS-USD conversion through delphioracle
		auto tlosusd = tlosusdprice();

		//Calculate the total amount that needs to be paid by the claimant and substract it from his balance
		float tlos_cost_amount = float(usd_total_cost.amount*10000 / tlosusd);
		auto tlos_cost = asset(tlos_cost_amount, TLOS_SYM);
		sub_balance(cf.claimant, tlos_cost);
		
		//The arbitrator rate cost will be added to the reserved funds, because in some scenarios it might be returned
		//In particular, it will be returned in the case that there's a mistrial (arbitrator recused, was forced to recuse)
		//or it was dismissed (BPs determined that the ruling wasn't valid)
		conf.reserved_funds += tlos_cost;
		configs.set(conf, get_self());

		//Updates casefile
		casefiles.modify(cf, get_self(), [&](auto &col) {
			col.arbitrators = vector<name>{offer.arbitrator};;
			col.case_status = static_cast<uint8_t>(case_status::ARBS_ASSIGNED);
			col.update_ts = time_point_sec(current_time_point());
			col.arbitrator_cost_tlos = tlos_cost;
		});

		//Use secondary index to get all the other offers of the case
		auto offers_idx = offers.get_index<name("bycase")>();
		auto itr_start = offers_idx.lower_bound(case_id);
		auto itr_end = offers_idx.upper_bound(case_id);
		
		//Accept the corresponding offer and decline all the other ones
		for(auto offer_itr = itr_start; offer_itr != itr_end; ++offer_itr) {
			offers_idx.modify(offer_itr, same_payer, [&](auto& col) {
				if(offer_itr->offer_id == offer_id) {
					col.status = static_cast<uint8_t>(offer_status::ACCEPTED);
				} else {
					col.status = static_cast<uint8_t>(offer_status::REJECTED);
				}
			});
		}
	} else {
		//If the offer was declined, update the status accordingly
		offers.modify(offer, same_payer, [&](auto& col) {
			col.status = static_cast<uint8_t>(offer_status::REJECTED);
		});
	}
	
};

void arbitration::cancelcase(uint64_t case_id) {

	//open config singleton, get config
	config_singleton configs(get_self(), get_self().value);
	auto conf = configs.get();

	//open casefile tables and checks that the case exists
	casefiles_table casefiles(get_self(), get_self().value);
	const auto& cf = casefiles.get(case_id, "Case not found");

	//A case can only be canceled while it is in awaiting arbs status
	check(cf.case_status == case_status::AWAITING_ARBS, "Case status must be in AWAITING ARBS");

	//authenticate
	require_auth(cf.claimant);
		
	//update casefiles status
	casefiles.modify(cf, same_payer, [&](auto& col){
		col.case_status = static_cast<uint8_t>(case_status::CANCELLED);
		col.update_ts = time_point_sec(current_time_point());
	});

	//Substract the fee paid by the claimant from the reserved funds
	conf.reserved_funds -= cf.fee_paid_tlos;

	//If there are no offers made, the fee will be returned to the claimant
	//Otherwise, the funds will be added to the available funds
	if(cf.number_offers == 0) {
		add_balance(cf.claimant, cf.fee_paid_tlos, get_self());
		configs.set(conf, get_self());
	} else {
		conf.available_funds += cf.fee_paid_tlos;
		configs.set(conf, get_self());
	}
};

#pragma endregion Claimant_Actions

#pragma region Respondant_Actions

void arbitration::respond(uint64_t case_id, uint64_t claim_id, name respondant, string response_link)
{	
	//authenticate
	require_auth(respondant);

	//Check that tha response_link is a valid IPFS Hash
	validate_ipfs_url(response_link);

	//open casefile tables and checks that the case exists
	casefiles_table casefiles(get_self(), get_self().value);
	const auto& cf = casefiles.get(case_id, "Case not found");

	//Only respondant can add a response to a claim, and a response can only be added during case investigation status
	check(cf.respondant != name(0), "case_id does not have a respondant");
	check(cf.respondant == respondant, "must be the respondant of this case_id");
	check(cf.case_status == case_status::CASE_INVESTIGATION, "case status does NOT allow responses at this time");

	//open claims tables and checks that the claim exists
	claims_table claims(get_self(), case_id);
	auto claim_it = claims.find(claim_id);
	check(claim_it != claims.end(), "Claim not found");

	//To respond a claim, a response information needs to be asked by the arbitrator and the claim cannot be resolved yet
	check(claim_it->status == claim_status::FILED || claim_it->status == claim_status::RESPONDED , "Claim must be in FILED status");
	check(claim_it->response_info_needed, "No response needed");

	//Update claim
	claims.modify(claim_it, get_self(), [&](auto& col){
		col.response_link = response_link;
		col.status = static_cast<uint8_t>(claim_status::RESPONDED);
		col.response_info_needed = false;
	});	
}


#pragma endregion Respondant_Actions

#pragma region Case_Actions

void arbitration::startcase(uint64_t case_id, name assigned_arb, uint8_t number_days_respondant, string response_info_required) {

	//authenticate
	require_auth(assigned_arb);

	//open casefile tables and checks that the case exists
	casefiles_table casefiles(get_self(), get_self().value);
	const auto& cf = casefiles.get(case_id, "Case not found");

	//Check that the arbitrator is assigned to the case
	auto arb_case = std::find(cf.arbitrators.begin(), cf.arbitrators.end(), assigned_arb);
	check(arb_case != cf.arbitrators.end(), "Only an assigned arbitrator can start a case");
	
	//Check that the case in arbs assigned status
	check(cf.case_status == case_status::ARBS_ASSIGNED, "Case status must be in ARBS_ASSIGNED");
	
	//Update casefile
	casefiles.modify(cf, get_self(), [&](auto &col) {
		col.case_status = static_cast<uint8_t>(case_status::CASE_INVESTIGATION);
		col.update_ts = time_point_sec(current_time_point());
	});

	//If there is a respondant, initialize all claims with a response limit time. If no response has 
	//been provided after that time, the arbitrator will be able to set a decision for the claim
	if(cf.respondant != name(0)) {
		claims_table claims(get_self(), case_id);
		for(auto claim_it = claims.begin(); claim_it != claims.end(); ++claim_it) {
			claims.modify(claim_it, get_self(), [&](auto& col){
				col.response_info_needed = true;
				col.response_info_required = response_info_required;
				col.respondant_limit_time = time_point_sec(current_time_point().sec_since_epoch() + number_days_respondant*86400);
			});	
		}
	}
	
	//Update the open cases for the corresponding arbitrator
	arbitrators_table arbitrators(get_self(), get_self().value);
	const auto& arb = arbitrators.get(assigned_arb.value);

	vector<uint64_t> new_open_cases = arb.open_case_ids;
	new_open_cases.emplace_back(case_id);

	arbitrators.modify(arb, same_payer, [&](auto &col) {
		col.open_case_ids = new_open_cases;
	});

}

void arbitration::reviewclaim(uint64_t case_id, uint64_t claim_id, name assigned_arb, 
	bool claim_info_needed, string claim_info_required, bool response_info_needed, 
	string response_info_required, uint8_t number_days_claimant, uint8_t number_days_respondant){
	
	//authenticate
	require_auth(assigned_arb);

	//open casefile tables and checks that the case exists
	casefiles_table casefiles(get_self(), get_self().value);
	const auto& cf = casefiles.get(case_id, "Case not found");

	//Check that the arbitrator is assigned to the case
	auto arb_case = std::find(cf.arbitrators.begin(), cf.arbitrators.end(), assigned_arb);
	check(arb_case != cf.arbitrators.end(), "Only an assigned arbitrator can review a claim");

	//Check that the case in investigation status
	check(cf.case_status == case_status::CASE_INVESTIGATION, "To review a claim, case should be in investigation status");

	//open claim tables and checks that the claim exists
	claims_table claims(get_self(), case_id);
	auto claim_it = claims.find(claim_id);
	check(claim_it != claims.end(), "Claim not found");

	//Check that the claim has not been settled yet and at least extra information is being asked from either the claimant or the respondant
	check(claim_it->status == claim_status::FILED || claim_it->status != claim_status::RESPONDED, "Claim status needs to be filed or responded to review");
	check(claim_info_needed || response_info_needed, "Need to update at least respondant or claimant");
	check(!claim_info_needed || number_days_claimant > 0, "At least one day has to be given to the claimant");
	check(!response_info_needed || number_days_respondant > 0, "At least one day has to be given to the respondant");

	//Update claim
	claims.modify(claim_it, get_self(), [&](auto& col) {
		if(claim_info_needed) {
			col.claim_info_needed = true;
			col.claim_info_required = claim_info_required;
			col.claimant_limit_time = time_point_sec(current_time_point().sec_since_epoch() + number_days_claimant*86400);
		}

		if(response_info_needed) {
			col.response_info_needed = true;
			col.response_info_required = response_info_required;
			col.respondant_limit_time = time_point_sec(current_time_point().sec_since_epoch() + number_days_respondant*86400);
		}
	});

};


void arbitration::settleclaim(uint64_t case_id, name assigned_arb, uint64_t claim_id, bool accept, string decision_link)
{	
	//authenticate
	require_auth(assigned_arb);

	//Check that the decision is a valid IPFS Hash
	validate_ipfs_url(decision_link);

	//open casefile tables and checks that the case exists
	casefiles_table casefiles(get_self(), get_self().value);
	const auto& cf = casefiles.get(case_id, "Case not found");

	//Check that the case in investigation status
	check(cf.case_status == case_status::CASE_INVESTIGATION, "To settle a claim, case should be in investigation status");

	//Check that the arbitrator is assigned to the case
	auto arb_case = std::find(cf.arbitrators.begin(), cf.arbitrators.end(), assigned_arb);
	check(arb_case != cf.arbitrators.end(), "Only an assigned arbitrator can settle a claim");

	//open claim tables and checks that the claim exists
	claims_table claims(get_self(), case_id);
	auto claim_it = claims.find(claim_id);
	check(claim_it != claims.end(), "Claim not found");

	auto now = time_point_sec(current_time_point());

	//If there is a respondant, check if it still has time to provide a response. If so, the claim can not be settled
	if(cf.respondant != name(0)) {
		check((!claim_it->response_info_needed  || (claim_it->response_info_needed && claim_it->respondant_limit_time <= now)), 
			"Respondant still have time to respond");
	}

	//If extra information was asked to the claimant, check if it still has time to provide it. If so, the claim can not be settled
	check((!claim_it->claim_info_needed  || (claim_it->claim_info_needed && claim_it->claimant_limit_time <= now)), 
			"Claimant still have time to respond");
	
	//Update claim
	claims.modify(claim_it, get_self(), [&](auto& col){
		col.decision_link = decision_link;
		if(accept) {
			col.status = static_cast<uint8_t>(claim_status::ACCEPTED);
		} else {
			col.status = static_cast<uint8_t>(claim_status::DISMISSED);
		}
	});	
}

void arbitration::setruling(uint64_t case_id, name assigned_arb, string case_ruling) {
	//authenticate
	require_auth(assigned_arb);

	//Check that the case ruling is a valid IPFS Hash
	validate_ipfs_url(case_ruling);

	//open casefile tables and checks that the case exists
	casefiles_table casefiles(get_self(), get_self().value);
	const auto& cf = casefiles.get(case_id, "Case not found");

	//Check that the case in investigation status
	check(cf.case_status == case_status::CASE_INVESTIGATION, "Case status must be CASE INVESTIGATION");

	//Check that all the claims has been resolved. If there's any claim left, the ruling can not be set
	check(all_claims_resolved(case_id), "There are claims that has not been resolved");
	
	//Check that the arbitrator is assigned to the case
	auto arb_it = std::find(cf.arbitrators.begin(), cf.arbitrators.end(), assigned_arb);
	check(arb_it != cf.arbitrators.end(), "Only an assigned arbitrator can set a ruling");

	//Update casefile
	casefiles.modify(cf, same_payer, [&](auto& col) {
		col.case_ruling = case_ruling;
		col.case_status = static_cast<uint8_t>(case_status::DECISION);
		col.update_ts = time_point_sec(current_time_point());
	});

	notify_bp_accounts();
}

#pragma endregion Case_Actions

#pragma region Arb_Actions

void arbitration::newarbstatus(name arbitrator, uint8_t new_status)
{	
	//authenticate
	require_auth(arbitrator);

	//open arbitrator table and checks that the arbitrator exists
	arbitrators_table arbitrators(_self, _self.value);
	const auto& arb = arbitrators.get(arbitrator.value, "Arbitrator not found");

	//If the seat has expired or is removed, or if the term has expired, the arbitrator can not set a new status
	check(arb.arb_status != arb_status::SEAT_EXPIRED && arb.arb_status != arb_status::REMOVED, "Arbitrator must be active");
	check(time_point_sec(current_time_point()) < arb.term_expiration, "Arbitrator term expired");

	//Update the arbitrator status
	arbitrators.modify(arb, same_payer, [&](auto &col) {
		col.arb_status = new_status;
	});
}

void arbitration::setlangcodes(name arbitrator, vector<uint16_t> lang_codes)
{	
	//authenticate
	require_auth(arbitrator);

	//open arbitrator table and checks that the arbitrator exists
	arbitrators_table arbitrators(get_self(), get_self().value);
	const auto& arb = arbitrators.get(arbitrator.value, "Arbitrator not found");

	//If the seat has expired or is removed, or if the term has expired, the arbitrator can not set a lang codes
	check(arb.arb_status != arb_status::SEAT_EXPIRED && arb.arb_status != arb_status::REMOVED, "Arbitrator must be active");
	check(time_point_sec(current_time_point()) < arb.term_expiration, "Arbitrator term expired");

	//Update the arbitrator lang codes
	arbitrators.modify(arb, same_payer, [&](auto& a) {
		a.languages = lang_codes;
	});
}

void arbitration::makeoffer(uint64_t case_id, int64_t offer_id, name arbitrator, asset hourly_rate, uint8_t estimated_hours) {
	//authenticate
	require_auth(arbitrator);

	//open config singleton, get config
	config_singleton configs(get_self(), get_self().value);
	auto conf = configs.get();

	//open arbitrators table and check that arbitrator is valid, meaning arbitrator status is available
	arbitrators_table arbitrators(get_self(), get_self().value);
	const auto& arb = arbitrators.get(arbitrator.value, "actor is not a registered Arbitrator");
	check(arb.arb_status != arb_status::REMOVED, "Arbitrator has been removed.");
	check(arb.arb_status != arb_status::SEAT_EXPIRED && 
		time_point_sec(current_time_point()) <= arb.term_expiration, "Arbitrator term has expired");
	check(arb.arb_status == arb_status::AVAILABLE, "Arb status isn't set to available, Arbitrator is unable to receive new cases");

	//open casefile tables and checks that the case exists
	casefiles_table casefiles(get_self(), get_self().value);
	const auto& cf = casefiles.get(case_id, "Case not found");

	check(cf.claimant != arbitrator && cf.respondant != arbitrator, "Arbitrator can not be neither claimant nor respondant");

	//Offers can only be made if case is in awaiting arbs status and the period for sending offers is valid
	check(cf.case_status == case_status::AWAITING_ARBS, "Case needs to be in AWAITING ARBS status");
	check(time_point_sec(current_time_point()) <= cf.sending_offers_until_ts, "Time for sending offers is over");

	//Offers needs to be done in USD and estimated hours need to be at least 1
	check(hourly_rate.symbol == USD_SYM, "Offer needs to be set in USD");
	check(estimated_hours > 0, "The minimum estimated hours is 1");

	
	offers_table offers(get_self(), get_self().value);

	if(offer_id == -1) {
		//If offer is -1, the arbitrator is trying to create a new offer. 
		//Use secondary index to search if arbitrator has already made an offer for that case id
		auto offers_idx = offers.get_index<name("bycasearb")>();
		auto itr_start = offers_idx.lower_bound(((uint128_t) case_id << 64) + arbitrator.value);
		auto itr_end = offers_idx.upper_bound(((uint128_t) case_id << 64) + arbitrator.value);
		check(itr_start == itr_end, "Arbitrator already has made an offer"); 

		//Create an offer
		offers.emplace(arbitrator, [&](auto& col) {
			col.offer_id = offers.available_primary_key();
			col.case_id = case_id;
			col.status = static_cast<uint8_t>(offer_status::PENDING);
			col.estimated_hours = estimated_hours;
			col.hourly_rate = hourly_rate;
			col.arbitrator = arbitrator;
		});

		//Update casefile
		casefiles.modify(cf, get_self(), [&](auto& col) {
			col.number_offers += 1;
			col.update_ts = time_point_sec(current_time_point());
		});
	} else {
		//If offer is -1, check if the offer id is valid, that the offer belongs to the 
		//case id, was proposed by the same arbitrator and that hasn't been accepted nor declined
		const auto& offer = offers.get(offer_id, "Offer not found");
		check(offer.case_id == case_id, "Offer does not belong to the case ID");
		check(offer.arbitrator == arbitrator, "Arbitrator is not the one who made the offer");
		check(offer.status == offer_status::PENDING, "Offer needs to be pending to be updated");

		//Update the offer
		offers.modify(offer, same_payer, [&](auto& col) {
			col.estimated_hours = estimated_hours;
			col.hourly_rate = hourly_rate;
		});
	}
};


void arbitration::dismissoffer(uint64_t case_id, uint64_t offer_id) {
	//open config singleton, get config
	config_singleton configs(get_self(), get_self().value);
	auto conf = configs.get();

	//open casefile tables and checks that the case exists
	casefiles_table casefiles(get_self(), get_self().value);
	const auto& cf = casefiles.get(case_id, "Case not found");

	//The case needs to be in awaiting arbs status to dismiss an offer
	check(cf.case_status == case_status::AWAITING_ARBS, "Case needs to be in AWAITING ARBS status");

	//open offers table and checks that the offer exists and belongs to the case id
	offers_table offers(get_self(), get_self().value);
	const auto& offer = offers.get(offer_id, "Offer not found");
	check(offer.case_id == case_id, "Offer does not belong to the case ID");

	//authenticate
	require_auth(offer.arbitrator);

	//To dismiss an offer it needs to be in pending status
	check(offer.status == offer_status::PENDING, "Offer needs to be pending to be updated");
	
	//Updates offer table
	offers.modify(offer, same_payer, [&](auto& col) {
		col.status = static_cast<uint8_t>(offer_status::DISMISSED);
	});
};

void arbitration::recuse(uint64_t case_id, string rationale, name assigned_arb)
{	
	//open config table, get config
	config_singleton configs(get_self(), get_self().value);
	auto conf = configs.get();

	//authenticate
	require_auth(assigned_arb);

	//open casefile tables and checks that the case exists
	casefiles_table casefiles(get_self(), get_self().value);
	const auto& cf = casefiles.get(case_id, "Case not found");

	//To recuse from a case, the case cannot be validated by the BPs and 
	//the arbitrator needs to be assigned to the case
	check(cf.case_status > case_status::AWAITING_ARBS && cf.case_status < case_status::ENFORCEMENT, 
	"Unable to recuse if the case has not started or it is resolved");	

	//Check that the arbitrator is assigned to the case
	auto arb_case = std::find(cf.arbitrators.begin(), cf.arbitrators.end(), assigned_arb);
	check(arb_case != cf.arbitrators.end(), "Arbitrator isn't selected for this case.");

	assert_string(rationale, std::string("rationale must be greater than 0 and less than 255"));

	//Update casefile
	casefiles.modify(cf, same_payer, [&](auto &col) {
		col.case_status = static_cast<uint8_t>(case_status::MISTRIAL);
	});

	//Return the fee paid and the arbitrator rate cost to the claimant, since the case is considered mistrial
	auto tlos_returned = cf.fee_paid_tlos + cf.arbitrator_cost_tlos;
	add_balance(cf.claimant, tlos_returned, get_self());

	//Remove the telos returned from reserved funds
	conf.reserved_funds -= tlos_returned;
	configs.set(conf, get_self());

	//open arbitrators table, get assigned arb
	arbitrators_table arbitrators(get_self(), get_self().value);
	const auto& arb = arbitrators.get(assigned_arb.value);

	//Update open and recused cases in the arbitrator table
	vector<uint64_t> new_open_cases = arb.open_case_ids;
	vector<uint64_t> new_recused_cases = arb.recused_case_ids;

	const auto case_itr = std::find(new_open_cases.begin(), new_open_cases.end(), case_id);
	if(case_itr != new_open_cases.end()) {
		new_open_cases.erase(case_itr);
	}
	new_recused_cases.emplace_back(case_id);

	arbitrators.modify(arb, same_payer, [&](auto &col) {
		col.open_case_ids = new_open_cases;
		col.recused_case_ids = new_recused_cases;
	});
}


#pragma endregion Arb_Actions

#pragma region BP_Actions

void arbitration::closecase(uint64_t case_id) {
	//open config table, get config
	config_singleton configs(get_self(), get_self().value);
	auto conf = configs.get();

	//authenticate
	require_auth(conf.admin);

	//open casefile tables and checks that the case exists
	casefiles_table casefiles(get_self(), get_self().value);
	const auto& cf = casefiles.get(case_id, "Case not found");

	//Check that the case is in enforcement status
	check(cf.case_status == case_status::ENFORCEMENT, "Case status must be ENFORCEMENT");

	//Update casefile
	casefiles.modify(cf, same_payer, [&](auto& col) {
		col.case_status = static_cast<uint8_t>(case_status::RESOLVED);
		col.update_ts = time_point_sec(current_time_point());
	});

	//open arbitrators table, get arbitrator
	arbitrators_table arbitrators(get_self(), get_self().value);
	const auto& arb = arbitrators.get(cf.arbitrators[0].value);

	//Update open and closed cases in the arbitrator table
	vector<uint64_t> new_open_cases = arb.open_case_ids;
	vector<uint64_t> new_closed_cases = arb.closed_case_ids;

    const auto case_itr = std::find(new_open_cases.begin(), new_open_cases.end(), case_id);
	if(case_itr != new_open_cases.end()) {
		new_open_cases.erase(case_itr);
	}
	new_closed_cases.emplace_back(case_id);

	arbitrators.modify(arb, same_payer, [&](auto &col) {
		col.open_case_ids = new_open_cases;
		col.closed_case_ids = new_closed_cases;
	});
}

void arbitration::validatecase(uint64_t case_id, bool proceed)
{	
	//open config table, get config
	config_singleton configs(get_self(), get_self().value);
	auto conf = configs.get();

	//authenticate
	require_auth(conf.admin);

	//open casefile tables and checks that the case exists
	casefiles_table casefiles(get_self(), get_self().value);
	const auto& cf = casefiles.get(case_id, "Case not found");

	//Check that the case is in decision status
	check(cf.case_status == case_status::DECISION, "Case must be in DECISION status");

	//BPs can decide either to proceed or dismiss the case if they consider that the arbitrator ruling isn't valid
	if(proceed) {
		//Remove the arbitrator cost and the fee from reserved funds, and add the fee to available funds.
		conf.reserved_funds -= cf.arbitrator_cost_tlos + cf.fee_paid_tlos;
		conf.available_funds += cf.fee_paid_tlos;
		configs.set(conf, get_self());

		//Add to the arbitrator balance the corresponding rate cost paid by the claimant
		add_balance(cf.arbitrators[0], cf.arbitrator_cost_tlos, get_self());

	} else {
		//If the case is considered not valid, return the fee paid and the arbitrator 
		//rate cost to the claimant, since the case is considered mistrial
		auto tlos_returned = cf.fee_paid_tlos + cf.arbitrator_cost_tlos;
		add_balance(cf.claimant, tlos_returned, get_self());

		//Remove the telos returned from reserved funds
		conf.reserved_funds -= tlos_returned;
		configs.set(conf, get_self());

		//open arbitrators table, get arbitrator
		arbitrators_table arbitrators(get_self(), get_self().value);
		const auto& arb = arbitrators.get(cf.arbitrators[0].value);

		//Update open and closed cases in the arbitrator table
		vector<uint64_t> new_open_cases = arb.open_case_ids;
		vector<uint64_t> new_closed_cases = arb.closed_case_ids;

		const auto case_itr = std::find(new_open_cases.begin(), new_open_cases.end(), case_id);
		if(case_itr != new_open_cases.end()) {
			new_open_cases.erase(case_itr);
		}
		new_closed_cases.emplace_back(case_id);

		arbitrators.modify(arb, same_payer, [&](auto &col) {
			col.open_case_ids = new_open_cases;
			col.closed_case_ids = new_closed_cases;
		});
	}

	//Update casefile
	casefiles.modify(cf, same_payer, [&](auto &col) {
		if(proceed) {
			col.case_status = static_cast<uint8_t>(case_status::ENFORCEMENT);
		} else {
			col.case_status = static_cast<uint8_t>(case_status::DISMISSED);
		}
		col.update_ts = time_point_sec(current_time_point());
	});
}

void arbitration::forcerecusal(uint64_t case_id, string rationale, name arbitrator) {
	//open config table, get config
	config_singleton configs(get_self(), get_self().value);
	auto conf = configs.get();

	//authenticate
	require_auth(conf.admin);

	//open casefile tables and checks that the case exists
	casefiles_table casefiles(get_self(), get_self().value);
	const auto& cf = casefiles.get(case_id, "Case not found");

	//To force a recusal of an arbitrator from a case, the case cannot be validated by the BPs and 
	//the arbitrator needs to be assigned to the case
	check(cf.case_status > case_status::AWAITING_ARBS && cf.case_status < case_status::ENFORCEMENT, 
		"Unable to force recusal if case has not started or if is resolved");	

	//Check that the arbitrator is assigned to the case
	auto arb_case = std::find(cf.arbitrators.begin(), cf.arbitrators.end(), arbitrator);
	check(arb_case != cf.arbitrators.end(), "Arbitrator isn't selected for this case.");

	assert_string(rationale, std::string("rationale must be greater than 0 and less than 255"));

	//Update casefile
	casefiles.modify(cf, same_payer, [&](auto &col) {
		col.case_status = static_cast<uint8_t>(case_status::MISTRIAL);
	});

	//Return the fee paid and the arbitrator rate cost to the claimant, since the case is considered mistrial
	auto tlos_returned = cf.fee_paid_tlos + cf.arbitrator_cost_tlos;
	add_balance(cf.claimant, tlos_returned, get_self());

	//Remove the telos returned from reserved funds
	conf.reserved_funds -= tlos_returned;
	configs.set(conf, get_self());

	//open arbitrators table, get arbitrator
	arbitrators_table arbitrators(get_self(), get_self().value);
	const auto& arb = arbitrators.get(arbitrator.value);

	//Update open and recused cases in the arbitrator table
	vector<uint64_t> new_open_cases = arb.open_case_ids;
	vector<uint64_t> new_recused_cases = arb.recused_case_ids;

	const auto case_itr = std::find(new_open_cases.begin(), new_open_cases.end(), case_id);
	if(case_itr != new_open_cases.end()) {
		new_open_cases.erase(case_itr);
	}		
	new_recused_cases.emplace_back(case_id);

	arbitrators.modify(arb, same_payer, [&](auto &col) {
		col.open_case_ids = new_open_cases;
		col.recused_case_ids = new_recused_cases;
	});
}

void arbitration::dismissarb(name arbitrator, bool remove_from_cases)
{	
	//open config table, get config
	config_singleton configs(get_self(), get_self().value);
	auto conf = configs.get();

	//authenticate
	require_auth(conf.admin);

	//open arbitrators table and checks that arbitrator exists and is not removed nor the term has expired
	arbitrators_table arbitrators(get_self(), get_self().value);
	const auto& arb_to_dismiss = arbitrators.get(arbitrator.value, "Arbitrator not found");
	check(arb_to_dismiss.arb_status != arb_status::REMOVED, "Arbitrator is already removed");
	check(arb_to_dismiss.arb_status != arb_status::SEAT_EXPIRED && 
			time_point_sec(current_time_point()) <= arb_to_dismiss.term_expiration, "Arbitrator term has expired");

	//Update arbitrator status to removed
	arbitrators.modify(arb_to_dismiss, same_payer, [&](auto& a) {
		a.arb_status = static_cast<uint8_t>(arb_status::REMOVED);
	});

	//Update arbitrator permissions
	auto perms = get_arb_permissions();
	set_permissions(perms);

	//When dismissing an arbitrator, admin can decide to keep the arbitrator in the cases he is handling or
	//directly removing it from all the cases. All the cases that are in progress will be considered mistrial
	if(remove_from_cases) {
		//open casefiles table
		casefiles_table casefiles(get_self(), get_self().value);

		//Variable containing the total sum of funds returned
		asset funds_returned = asset(0, TLOS_SYM);

		//Iterate through all the cases that are opened for that arbitrator
		for(const auto &id: arb_to_dismiss.open_case_ids) {
			auto cf_it = casefiles.find(id);

			if (cf_it != casefiles.end()) {
				auto case_arbs = cf_it->arbitrators;
				auto arb_it = find(case_arbs.begin(), case_arbs.end(), arb_to_dismiss.arb);

				//Only those cases that doesn't have a ruling validate by the BPs will be considered a mistrial
				if (arb_it != case_arbs.end() && cf_it->case_status < case_status::ENFORCEMENT) {
					//Update casefile
					casefiles.modify(cf_it, same_payer, [&](auto &col) {
						col.case_status = static_cast<uint8_t>(case_status::MISTRIAL);
					});

					//Return the fee paid and the arbitrator rate cost to the claimant, since the case is considered mistrial
					auto tlos_returned = cf_it->fee_paid_tlos + cf_it->arbitrator_cost_tlos;
					add_balance(cf_it->claimant, tlos_returned, get_self());

					//Add the tlos returned to funds_returned to keep track of the total
					funds_returned += tlos_returned;						
				}
			}
		}

		//Remove the funds returned from reserved funds
		conf.reserved_funds -= funds_returned;
		configs.set(conf, get_self());

		//Update open and closed cases in the arbitrator table
		auto open_ids = arb_to_dismiss.open_case_ids;
		auto recused_ids = arb_to_dismiss.recused_case_ids;

		recused_ids.insert(recused_ids.end(), open_ids.begin(), open_ids.end());
		open_ids.clear();

		arbitrators.modify(arb_to_dismiss, same_payer, [&](auto& a) {
			a.arb_status = static_cast<uint8_t>(arb_status::REMOVED);
			a.open_case_ids = open_ids;
			a.recused_case_ids = recused_ids;
		});
	}
}

#pragma endregion BP_Actions


#pragma region Helpers

typedef arbitration::claim claim;


void arbitration::validate_ipfs_url(string ipfs_url)
{
	//Check that ipfs_url is a valid ipfs HASH
	check(ipfs_url.length() == 46 || ipfs_url.length() == 49, "invalid ipfs string, valid schema: <hash>");
}

void arbitration::assert_string(string to_check, string error_msg)
{
	check(to_check.length() > 0 && to_check.length() < 255, error_msg.c_str());
}


void arbitration::start_new_election(uint8_t available_seats, bool runoff, string election_info)
{
	//open elections table
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
        col.info_url = std::string(election_info);
        col.candidates = vector<candidate> {};
        col.available_seats = available_seats;
		col.begin_add_candidates_ts = now;
        col.end_add_candidates_ts = end_ts;
        col.status = static_cast<uint8_t>(election_status::CREATED);
    });
}

bool arbitration::all_claims_resolved(uint64_t case_id) {
	//open claims table for case id
	claims_table claims(get_self(), case_id);

	//check that all the claims has been resolved, either accepted or declined
	for(auto claim_itr = claims.begin(); claim_itr != claims.end(); ++claim_itr) {
		if(claim_itr->status == claim_status::FILED || claim_itr->status == claim_status::RESPONDED) {
			return false;
		}
	}

	return true;
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

	//calculate the number of available seats
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

/**
* Notifies all the BPs active accounts using require_recipient
*/
void arbitration::notify_bp_accounts() {

	eosiosystem::producers_table producers(name("eosio"), name("eosio").value);

	auto most_voted_idx = producers.get_index<name("prototalvote")>();
	auto producer_itr = most_voted_idx.begin();

	auto bps_counter = 0;

	while(producer_itr != most_voted_idx.end() && bps_counter < 21) {
		require_recipient(producer_itr->owner);
		++producer_itr;
		++bps_counter;
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

uint64_t arbitration::tlosusdprice() {
	//Get TLOS-USD price from delphioracle
	delphioracle::datapoints_table _datapoints(name("delphioracle"), name("tlosusd").value);
	auto itr = _datapoints.find(1);
	return itr -> median;
}


#pragma endregion Helpers

#pragma region Notification_handlers

void arbitration::transfer_handler(name from, name to, asset quantity, string memo){
	//authenticate
	require_auth(from);

	check(quantity.is_valid(), "Invalid quantity");
	check(quantity.symbol == symbol("TLOS", 4), "only TLOS tokens are accepted by this contract");

	if (from == get_self()) {
		return;
	}

	check(to == get_self(), "to must be self");

	//If a user is sending funds to the Smart Contract with the memo "skip", funds will be added to 
	//available_funds in config. Otherwise funds will be added to the user's balance
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

void arbitration::catch_broadcast(name ballot_name, map<name, asset> final_results, uint32_t total_voters) {
	
	//get initial receiver contract
    name rec = get_first_receiver();

	//open config singleton, get config
    config_singleton configs(get_self(), get_self().value);
    auto conf = configs.get();


    //if notification not from decide account
    if (rec != TELOSDECIDE) {
        return;
    }

	//Open the elections table, get the election with the corresponding ballot using secondary indexes
    elections_table elections(get_self(), get_self().value);
    auto elections_by_ballot = elections.get_index<name("byballot")>();
    auto by_ballot_itr = elections_by_ballot.find(ballot_name.value);

	
    if (by_ballot_itr != elections_by_ballot.end()) {
		//Process all the voting results. To do so, we will first add those results in a vector and then
		//sort the vector by descending order
		vector<pair<name, asset>> pairs;
        for (auto itr = final_results.begin(); itr != final_results.end(); ++itr){
            pairs.push_back(*itr);
        }

        sort(pairs.begin(), pairs.end(), [=](pair<name, asset>& a, std::pair<name, asset>& b)
        	{ return a.second.amount > b.second.amount; });

		auto available_seats = by_ballot_itr->available_seats;

		//Keep track of tied candidates
		uint8_t number_tied_candidates = 0;
		vector<name> tied_candidates;

		//Number of seats that can potentially be occupied
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

		//If there is any candidate tied with the first candidate out that was supposed to had a seat, 
		//also add this candidate to the tied list
		if(available_seats > seats_occupied) {
			tied_candidates.push_back(pairs[available_seats].first);
			++number_tied_candidates;
		} 

		//open nominees and arbitrators table
		nominees_table nominees(get_self(), get_self().value);
		arbitrators_table arbitrators(get_self(), get_self().value);

		vector<candidate> candidates_results = vector<candidate>{};

		//Assign the most voted nominees that are not tied to an available seat
		for(int i = 0; i < pairs.size(); ++i) {
			name candidate_name = pairs[i].first;
			auto candidate_itr = nominees.find(candidate_name.value);

			if(candidate_itr != nominees.end()) {
				//Get candidate information
				auto cand_credential = candidate_itr->credentials_link;
				auto cand_votes = pairs[i].second.amount;
				
				if(i >= seats_occupied) {
					//If the candidate is not selected, remove it from the nominees table unless it is tied
					if(std::find(tied_candidates.begin(), tied_candidates.end(), candidate_name) == tied_candidates.end()) {
						nominees.erase(candidate_itr);
					}
				} else {
					//Add the candidate to the arbitrators table and remove it from the nominees table
					add_arbitrator(arbitrators, candidate_name, cand_credential);
					nominees.erase(candidate_itr);
				}
				
				candidates_results.push_back(candidate{pairs[i].first, pairs[i].second});
			}
		}

		//Update the elections table with the election results and set the election status to ended
		elections_by_ballot.modify(by_ballot_itr, get_self(), [&](auto& col){
			col.candidates = candidates_results;
			col.status = static_cast<uint8_t>(election_status::ENDED);
		});

		//Update the permissions for the new arbitrator accounts
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
