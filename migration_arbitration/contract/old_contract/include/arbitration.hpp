/**
 * Arbitration Contract Interface
 *
 * @author Roger Taulé, Craig Branscom, Peter Bue, Ed Silva, Douglas Horn
 * @copyright defined in telos/LICENSE.txt
 */

#pragma once
#include <eosio/action.hpp>
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/permission.hpp>
#include <eosio/singleton.hpp>
#include "telosdecide-interface.hpp"
#include "delphioracle-interface.hpp"
#include "eosiosystem-interface.hpp"

using namespace std;
using namespace eosio;
using namespace telosdecide;
using namespace delphioracle;
using namespace eosiosystem;

CONTRACT arbitration : public eosio::contract
{

  public:
	using contract::contract;

	static constexpr symbol TLOS_SYM = symbol("TLOS", 4);
    static constexpr symbol VOTE_SYM = symbol("VOTE", 4);
	static constexpr symbol USD_SYM = symbol("USD", 4);

#pragma region Enums

	//TODO: describe each enum in README

	enum class case_status : uint8_t
	{
		CASE_SETUP          = 0,		
		AWAITING_ARBS		= 1,
		ARBS_ASSIGNED       = 2,
		CASE_INVESTIGATION  = 3,
		DECISION			= 4,
		ENFORCEMENT			= 5,
		RESOLVED			= 6,
		DISMISSED			= 7,
		CANCELLED           = 8,
		MISTRIAL            = 9,
	};

	friend constexpr bool operator == ( const uint8_t& a, const case_status& b) {
		return a == static_cast<uint8_t>(b);
	}

	friend constexpr bool operator != ( const uint8_t& a, const case_status& b) {
		return a == static_cast<uint8_t>(b);
	}

	friend constexpr bool operator < ( const uint8_t& a, const case_status& b) {
		return a < static_cast<uint8_t>(b);
	}

	friend constexpr bool operator > ( const uint8_t& a, const case_status& b) {
		return a > static_cast<uint8_t>(b);
	}

	friend constexpr bool operator >= ( const uint8_t& a, const case_status& b) {
		return a >= static_cast<uint8_t>(b);
	}

	enum class offer_status : uint8_t
	{
		PENDING            	= 1,
		ACCEPTED        	= 2,
		REJECTED            = 3,
		DISMISSED           = 4,
	};

	friend constexpr bool operator == ( const uint8_t& a, const offer_status& b) {
		return a == static_cast<uint8_t>(b);
	}

	friend constexpr bool operator != ( const uint8_t& a, const offer_status& b) {
		return a == static_cast<uint8_t>(b);
	}

	enum class claim_status : uint8_t
	{
		FILED            	= 1,
		RESPONDED       	= 2,
		ACCEPTED            = 3,
		DISMISSED           = 4,
	};

	friend constexpr bool operator == ( const uint8_t& a, const claim_status& b) {
		return a == static_cast<uint8_t>(b);
	}

	friend constexpr bool operator != ( const uint8_t& a, const claim_status& b) {
		return a == static_cast<uint8_t>(b);
	}

	enum class claim_category : uint8_t
	{
		LOST_KEY_RECOVERY	= 1,
		TRX_REVERSAL		= 2,
		EMERGENCY_INTER		= 3,
		CONTESTED_OWNER		= 4,
		UNEXECUTED_RELIEF	= 5,
		CONTRACT_BREACH		= 6,
		MISUSED_CR_IP		= 7,
		A_TORT				= 8,
		BP_PENALTY_REVERSAL	= 9,
		WRONGFUL_ARB_ACT 	= 10,
		ACT_EXEC_RELIEF		= 11,
		WP_PROJ_FAILURE		= 12,
		TBNOA_BREACH		= 13,
		MISC				= 14
	};

	friend constexpr bool operator >= ( const uint8_t& a, const claim_category& b) {
		return a >= static_cast<uint8_t>(b);
	}

	friend constexpr bool operator <= ( const uint8_t& a, const claim_category& b) {
		return a <= static_cast<uint8_t>(b);
	}

	enum class arb_status : uint8_t
	{
		AVAILABLE           = 1,    
		UNAVAILABLE         = 2, 
		REMOVED             = 3,	 
		SEAT_EXPIRED        = 4, 
	};

	friend constexpr bool operator == ( const uint8_t& a, const arb_status& b) {
		return a == static_cast<uint8_t>(b);
	}

	friend constexpr bool operator != ( const uint8_t& a, const arb_status& b) {
		return a != static_cast<uint8_t>(b);
	}


	enum class election_status : uint8_t
	{
		CREATED             = 1,
		LIVE                = 2,   
		ENDED               = 3, 
	};

	friend constexpr bool operator == ( const uint8_t& a, const election_status& b) {
		return a == static_cast<uint8_t>(b);
	}

	enum class lang_code : uint16_t
	{
		ENGL                = 0, // English
		FRCH                = 1, // French
		GRMN                = 2, // German
		KREA                = 3, // Korean
		JAPN                = 4, // Japanese
		CHNA                = 5, // Chinese
		SPAN                = 6, // Spanish
		PGSE                = 7, // Portuguese
		SWED                = 8  // Swedish
	};

	struct candidate {
		name name;
		asset votes;
	};

#pragma endregion Enums

#pragma region Config_Actions

	//initialize the contract
	//pre: config table not initialized
	//auth: self
	ACTION init(name initial_admin);

	//set new admin
	//pre: new_admin account exists 
	//auth: admin
	ACTION setadmin(name new_admin);

	//set contract version
    //auth: admin
	ACTION setversion(string new_version);

	//set configuration parameters
    //auth: admin
	ACTION setconfig(uint16_t max_elected_arbs, uint32_t election_duration, uint32_t runoff_duration, 
		uint32_t election_add_candidates_duration, uint32_t arbitrator_term_length, uint8_t max_claims_per_case, 
		asset fee_usd, uint32_t claimant_accepting_offers_duration);

#pragma endregion Config_Actions

#pragma region Arb_Elections

	//Create a new election
	//auth: admin
	ACTION initelection(string content);

	//Register a new nominee
	//pre: Election must be in accepting candidates status
	//auth: nominee
	ACTION regarb(name nominee, string credentials_link);
    //NOTE: actually regnominee, currently regarb for nonsense governance reasons

	//Unregister a nominee
	//pre: Election must be in accepting candidates status
	//auth: nominee
	ACTION unregnominee(name nominee);

	//Set a nominee as a possible arbitrator candidate in the election
	//pre: Election must be in accepting candidates status
	//auth: nominee
	ACTION candaddlead(name nominee);

	//Remove a nomine as a possible arbitrator candidate in the election
	//pre: Election must be in accepting candidates status
	//auth: nominee
	ACTION candrmvlead(name nominee);

	//Starts the election voting
	//auth: admin
	ACTION beginvoting(name ballot_name, bool runoff);
	
	//Ends the election and set the new arbitrators
	//auth: admin
	ACTION endelection();

#pragma endregion Arb_Elections

#pragma region Claimant_Actions

	//Allows the owner to withdraw their funds
	//pre: balance > 0
	//auth: owner
	ACTION withdraw(name owner);

	//Files a new case
	//auth: claimant
	//NOTE: filing a case doesn't require a respondent
	ACTION filecase(name claimant, string claim_link, vector<uint16_t> lang_codes,
	        std::optional<name> respondant, uint8_t claim_category);

	//Adds a claim for an existing case
	//pre: case must be in setup status
	//auth: claimant
	ACTION addclaim(uint64_t case_id, string claim_link, name claimant, uint8_t claim_category);

	//Updates a claim for an existing case
	//pre: case must be in investigation or setup status
	//auth: claimant
	ACTION updateclaim(uint64_t case_id, uint64_t claim_id, name claimant, string claim_link);

	//Remove a claim for an existing case
	//pre: case must be in setup status
	//auth: claimant
	ACTION removeclaim(uint64_t case_id, uint64_t claim_id, name claimant);

	//Remove an existing case
	//pre: case must be in setup status
	//auth: claimant
	ACTION shredcase(uint64_t case_id, name claimant);

	//Set a case as ready to proceed
	//pre: case must be in setup status
	//post: case moves to awaiting arbs stage
	//auth: claimant
	ACTION readycase(uint64_t case_id, name claimant);

	//Respond an offer of an arbitator to take the case
	//pre: case must be in awaiting arbs status
	//post: if the offer is accepted, case moves to arbs assigned stage
	//auth: claimant
	ACTION respondoffer(uint64_t case_id, uint64_t offer_id, bool accept);

	//Cancel a case before accepting any of the arbitators offer
	//pre: case must be in awaiting arbs status
	//auth: claimant
	ACTION cancelcase(uint64_t case_id);

#pragma endregion Claimant_Actions

#pragma region Respondant_Actions

	//Allows the respondant to respond to a claim
	//pre: case must be in investigation status
	//auth: respondant
	ACTION respond(uint64_t case_id, uint64_t claim_id, name respondant, string response_link);

#pragma endregion Respondant_Actions

#pragma region Case_Actions

	//Starts the case investigation period
	//pre: case must be in arbs_assigned status
	//auth: assigned arbitrator
	ACTION startcase(uint64_t case_id, name assigned_arb, uint8_t number_days_respondant);

	//Ask the respondant and the claimant to provide more information if needed
	//pre: case must be in investigation status
	//auth: assigned arbitrator
	ACTION reviewclaim(uint64_t case_id, uint64_t claim_id, name assigned_arb, bool claim_info_needed, 
	bool response_info_needed, uint8_t number_days_claimant, uint8_t number_days_respondant);

	//Accepts or denies a claim of a particular case
	//pre: case must be in investigation status
	//auth: assigned arbitrator
	ACTION settleclaim(uint64_t case_id, name assigned_arb, uint64_t claim_id, bool accept, string decision_link);

	//After settling all the claims, set a ruling for the whole case
	//pre: case must be in investigation status and all claims settled
	//post: moves the case to decision stage
	//auth: assigned arbitrator
	ACTION setruling(uint64_t case_id, name assigned_arb, string case_ruling);

#pragma endregion Case_Actions

#pragma region BP_Actions

	//Validates that the case and the decision taken by the arbitrator are valid
	//pre: case must be in decision stage
	//post: if not valid, case is considered mistrial. Otherwise, move the case to enforcement stage
	//auth: admin
	ACTION validatecase(uint64_t case_id, bool proceed);

	//Closes a case after the ruling has been enforced
	//pre: case must be in enforcement status
	//post: moves the case to resolved status
	//auth: admin
	ACTION closecase(uint64_t case_id);

	//Forces the recusal of an arbitrator from a case
	//pre: case must not be enforced yet
	//post: Case is considered void and mistrial status is set
	//auth: admin
	ACTION forcerecusal(uint64_t case_id, string rationale, name arbitrator);

	//Dismiss an arbitrator from all his cases
	//auth: admin
	ACTION dismissarb(name arbitrator, bool remove_from_cases);

#pragma endregion BP_Actions

#pragma region Arb_Actions

	//Makes an offer with an hourly rate and the number of estimated ours for a case
	//pre: case must in awaiting arbs status
	//auth: arbitrator
	ACTION makeoffer(uint64_t case_id, int64_t offer_id, name arbitrator, asset hourly_rate, uint8_t estimated_hours);

	//Dismiss an offer made for a case
	//pre: case must in awaiting arbs status and offer not accepted nor declined yet
	//auth: arbitrator
	ACTION dismissoffer(uint64_t case_id, uint64_t offer_id);

	//Set the different languages the arbitrator will handle cases
	//auth: arbitrator
	ACTION setlangcodes(name arbitrator, vector<uint16_t> lang_codes);

	//Set a new arbitrator status
	//auth: arbitrator
	ACTION newarbstatus(name arbitrator, uint8_t new_status);

	//Recuse from a case
	//post: Case is considered void and mistrial status is set
	//auth: arbitrator
	ACTION recuse(uint64_t case_id, string rationale, name assigned_arb);

#pragma endregion Arb_Actions

#pragma region Test_Actions
	
#pragma endregion Test_Actions

#pragma region System Structs

	struct permission_level_weight
	{
		permission_level permission;
		uint16_t weight;

		EOSLIB_SERIALIZE(permission_level_weight, (permission)(weight))
	};

	struct key_weight
	{
		eosio::public_key key;
		uint16_t weight;

		EOSLIB_SERIALIZE(key_weight, (key)(weight))
	};

	struct wait_weight
	{
		uint32_t wait_sec;
		uint16_t weight;

		EOSLIB_SERIALIZE(wait_weight, (wait_sec)(weight))
	};


	struct authority
	{
		uint32_t threshold = 0;
		std::vector<key_weight> keys;
		std::vector<permission_level_weight> accounts;
		std::vector<wait_weight> waits;

		EOSLIB_SERIALIZE(authority, (threshold)(keys)(accounts)(waits))
	};

#pragma endregion System Structs

#pragma region Tables and Structs

	/**
   * Holds all arbitrator nominee applications.
   * @scope get_self().value
   * @key uint64_t nominee_name.value
   */
	TABLE nominee
	{
		name nominee_name;
		string credentials_link;
		time_point_sec application_time;

		uint64_t primary_key() const { return nominee_name.value; }
		EOSLIB_SERIALIZE(nominee, (nominee_name)(credentials_link)(application_time))
	};
	typedef multi_index<name("nominees"), nominee> nominees_table;


	/**
   * Holds all currently elected arbitrators.
   * @scope get_self().value
   * @key uint64_t arb.value
   */
	TABLE arbitrator
	{
		name arb;
		uint8_t arb_status;
		vector<uint64_t> open_case_ids;
		vector<uint64_t> closed_case_ids;
		vector<uint64_t> recused_case_ids;
		string credentials_link; //NOTE: ipfs_url of arbitrator credentials
		time_point_sec elected_time;
		time_point_sec term_expiration;
		vector<uint16_t> languages; //NOTE: language codes

		uint64_t primary_key() const { return arb.value; }
		EOSLIB_SERIALIZE(arbitrator, (arb)(arb_status)(open_case_ids)(closed_case_ids)
			(recused_case_ids)(credentials_link)(elected_time)(term_expiration)(languages))
	};
	typedef multi_index<name("arbitrators"), arbitrator> arbitrators_table;


	//NOTE: Stores all information related to a single claim.
	TABLE claim
	{
		uint64_t claim_id;
		string claim_summary; //NOTE: ipfs link to claim document from claimant
		string decision_link; //NOTE: ipfs link to decision document from arbitrator
		string response_link; //NOTE: ipfs link to response document from respondant (if any)
		time_point_sec claimant_limit_time;
		bool claim_info_needed = false;
		time_point_sec respondant_limit_time;
		bool response_info_needed = false;
		uint8_t status = static_cast<uint8_t>(claim_status::FILED);
		uint8_t claim_category;

		uint64_t primary_key() const { return claim_id; }
		EOSLIB_SERIALIZE(claim, (claim_id)(claim_summary)(decision_link)(response_link)
		(claimant_limit_time)(claim_info_needed)(respondant_limit_time)(response_info_needed)
		(status)(claim_category))
	};
	typedef multi_index<name("claims"), claim> claims_table;


	/**
   * Case Files for all arbitration cases.
   * @scope get_self().value
   * @key case_id
   */
	TABLE casefile
	{
		uint64_t case_id;
		uint8_t case_status;
		name claimant;
		name respondant;
		vector<name> arbitrators;
		vector<name> approvals;
		uint8_t number_claims;
		uint8_t number_offers;
		vector<uint16_t> required_langs;
		string case_ruling;
		asset fee_paid_tlos = asset(0, TLOS_SYM);
		asset arbitrator_cost_tlos = asset(0, TLOS_SYM);
		time_point_sec update_ts; 
		time_point_sec sending_offers_until_ts;

		uint64_t primary_key() const { return case_id; }

		uint64_t by_claimant() const { return claimant.value; }
		uint128_t by_uuid() const
		{
			uint128_t claimant_id = static_cast<uint128_t>(claimant.value);
			uint128_t respondant_id = static_cast<uint128_t>(respondant.value);
			return (claimant_id << 64) | respondant_id;
		}
		
		EOSLIB_SERIALIZE(casefile, (case_id)(case_status)(claimant)(respondant)(arbitrators)(approvals)
		(number_claims)(number_offers)(required_langs)(case_ruling)(fee_paid_tlos)
		(arbitrator_cost_tlos)(update_ts)(sending_offers_until_ts))
	};
	typedef multi_index<name("casefiles"), casefile> casefiles_table;


	/**
   * Singleton for global config settings.
   * @scope singleton scope (get_self().value)
   * @key table name
   */
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

		EOSLIB_SERIALIZE(config, (admin)(contract_version)(max_elected_arbs)(election_voting_ts)
			(runoff_election_voting_ts)(election_add_candidates_ts)(arb_term_length)
			(claimant_accepting_offers_ts)(current_election_id)
			(max_claims_per_case)(fee_usd)(available_funds)(reserved_funds))
	};
	typedef singleton<name("config"), config> config_singleton;

	/**
   * Holds instances of joinder cases.
   * @scope get_self().value
   * @key uint64_t join_id
   */
	TABLE joinder {
		uint64_t join_id;
		vector<uint64_t> cases;
		uint32_t join_time;
		name joined_by;

		uint64_t primary_key() const { return join_id; }
		EOSLIB_SERIALIZE(joinder, (join_id)(cases)(join_time)(joined_by))
	};
	typedef multi_index<name("joinedcases"), joinder> joinders_table;

	//scope: account name
	TABLE account
	{
		asset balance;

		uint64_t primary_key() const { return balance.symbol.code().raw(); }
		EOSLIB_SERIALIZE(account, (balance))
	};
	typedef multi_index<name("accounts"), account> accounts_table;

	//NOTE: scope: self
	TABLE offer {
		uint64_t offer_id;
		uint64_t case_id;
		uint8_t status;
		uint8_t estimated_hours;
		name arbitrator;
		asset hourly_rate;

		uint64_t primary_key() const { return offer_id; }

		uint64_t by_arbitrator() const {return arbitrator.value; }

		uint64_t by_case() const {return case_id; }

		uint128_t by_case_and_arbitrator() const { return ((uint128_t)case_id << 64)|((uint128_t)arbitrator.value); }

		EOSLIB_SERIALIZE(offer, (offer_id)(case_id)(status)(estimated_hours)(arbitrator)(hourly_rate))
	};

	typedef multi_index<name("offers"), offer,
		indexed_by<name("byarb"), const_mem_fun<offer, uint64_t, &offer::by_arbitrator>>,
		indexed_by<name("bycase"), const_mem_fun<offer, uint64_t, &offer::by_case>>,
		indexed_by<name("bycasearb"), const_mem_fun<offer, uint128_t, &offer::by_case_and_arbitrator>>
	> offers_table;



	//NOTE: scope: self
	TABLE election {
		uint64_t election_id;
		name ballot_name;
		string info_url;
		vector<candidate> candidates;
		uint8_t available_seats;
		time_point_sec begin_add_candidates_ts;
		time_point_sec end_add_candidates_ts;
		time_point_sec begin_voting_ts;
		time_point_sec end_voting_ts;
		uint8_t status;

		uint64_t primary_key() const { return election_id; }

		uint64_t by_ballot() const { return ballot_name.value; }

		EOSLIB_SERIALIZE(election, (election_id)(ballot_name)(info_url)
			(candidates)(available_seats)(begin_add_candidates_ts)
			(end_add_candidates_ts)(begin_voting_ts)(end_voting_ts)(status))
	};
	typedef multi_index<name("elections"), election,
	    indexed_by<name("byballot"), const_mem_fun<election, uint64_t, &election::by_ballot>>
		> elections_table;



#pragma endregion Tables and Structs

#pragma region Helpers

	void validate_ipfs_url(string ipfs_url);

	void assert_string(string to_check, string error_msg);

	void start_new_election(uint8_t available_seats, bool runoff, string content);

	bool has_available_seats(arbitrators_table & arbitrators, uint8_t & available_seats);

	void add_arbitrator(arbitrators_table & arbitrators, name arb_name, std::string credential_link);

	bool all_claims_resolved(uint64_t case_id);

	vector<permission_level_weight> get_arb_permissions();

	void set_permissions(vector<permission_level_weight> &perms);

	vector<claim>::iterator get_claim_at(string claim_hash, vector<claim>& claims);

	void sub_balance(name owner, asset value);

	void add_balance(name owner, asset value, name ram_payer);

	// mixes current transaction with seed and returns a hash
  	checksum256 get_rngseed(uint64_t seed);

	string get_rand_ballot_name();

	uint64_t tlosusdprice();

	void notify_bp_accounts();

#pragma endregion Helpers

#pragma region Notification_handlers

	[[eosio::on_notify("eosio.token::transfer")]]
	void transfer_handler(name from, name to, asset quantity, string memo);

	//catches broadcast notification from decide
    [[eosio::on_notify("telos.decide::broadcast")]]
    void catch_broadcast(name ballot_name, map<name, asset> final_results, uint32_t total_voters);

#pragma endregion Notification_handlers

#pragma region Test_Actions

	ACTION skiptovoting();
	
#pragma endregion Test_Actions

};
