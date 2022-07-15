/**
 * Arbitration Contract Interface
 *
 * @author Craig Branscom, Peter Bue, Ed Silva, Douglas Horn
 * @copyright defined in telos/LICENSE.txt
 */

#pragma once
#include <eosio/action.hpp>
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/permission.hpp>
#include <eosio/singleton.hpp>
#include "telosdecide-interface.hpp"

using namespace std;
using namespace eosio;
using namespace telosdecide;

CONTRACT arbitration : public eosio::contract
{

  public:
	using contract::contract;

	static constexpr symbol TLOS_SYM = symbol("TLOS", 4);
    static constexpr symbol VOTE_SYM = symbol("VOTE", 4);

#pragma region Enums

	//TODO: describe each enum in README

	enum class case_status : uint8_t
	{
		CASE_SETUP          = 0,		
		AWAITING_ARBS		= 1,
		CASE_INVESTIGATION  = 2,
		HEARING             = 3,
		DELIBERATION        = 4,
		DECISION			= 5,
		ENFORCEMENT			= 6,
		RESOLVED			= 7,
		DISMISSED			= 8,// 8 NOTE: Dismissed cases advance and stop here
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

	enum class claim_decision : uint8_t
	{
		UNDECIDED			= 1,
		LOST_KEY_RECOVERY	= 2,
		TRX_REVERSAL		= 3,
		EMERGENCY_INTER		= 4,
		CONTESTED_OWNER		= 5,
		UNEXECUTED_RELIEF	= 6,
		CONTRACT_BREACH		= 7,
		MISUSED_CR_IP		= 8,
		A_TORT				= 9,
		BP_PENALTY_REVERSAL	= 10,
		WRONGFUL_ARB_ACT 	= 11,
		ACT_EXEC_RELIEF		= 12,
		WP_PROJ_FAILURE		= 13,
		TBNOA_BREACH		= 14,
		MISC				= 15
	};

	enum class arb_status : uint8_t
	{
		AVAILABLE           = 0,    
		UNAVAILABLE         = 1, 
		REMOVED             = 2,	 
		SEAT_EXPIRED        = 3, 
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

	ACTION setadmin(name new_admin);

	ACTION setversion(string new_version);

	ACTION setconfig(uint16_t max_elected_arbs, uint32_t election_duration, uint32_t runoff_duration, 
		uint32_t election_add_candidates_duration, uint32_t arbitrator_term_length, vector<int64_t> fees/*, 
		uint8_t max_claims_per_case*/);

	ACTION setclaimclss(uint8_t claim_class_id, asset initial_deposit, asset fee, uint16_t respondant_days,
	 	uint8_t number_arbitrators);

	ACTION rmvclaimclss(uint8_t claim_class_id);

#pragma endregion Config_Actions

#pragma region Arb_Elections

	ACTION initelection(string content);

	ACTION regarb(name nominee, string credentials_link);
    //NOTE: actually regnominee, currently regarb for nonsense governance reasons

	ACTION unregnominee(name nominee);

	ACTION candaddlead(name nominee);

	ACTION candrmvlead(name nominee);

	ACTION beginvoting(name ballot_name, bool runoff);
	
	ACTION endelection();

#pragma endregion Arb_Elections

#pragma region Claimant_Actions

	ACTION withdraw(name owner);

	//NOTE: filing a case doesn't require a respondent
	ACTION filecase(name claimant, string claim_link, vector<uint16_t> lang_codes,
	        std::optional<name> respondant);

	ACTION addclaim(uint64_t case_id, string claim_link, name claimant);

	//NOTE: claims can only be removed by a claimant during case setup
	ACTION removeclaim(uint64_t case_id, uint64_t claim_id, name claimant);

	//NOTE: member-level case removal, called during CASE_SETUP
	ACTION shredcase(uint64_t case_id, name claimant);

	//NOTE: enforce claimant has at least 1 claim before readying
	ACTION readycase(uint64_t case_id, name claimant);

#pragma endregion Claimant_Actions

#pragma region Case_Actions

	ACTION respond(uint64_t case_id, uint64_t claim_id, name respondant, string response_link);

	ACTION assigntocase(uint64_t case_id, name arb_to_assign);

	ACTION addarbs(uint64_t case_id, name assigned_arb, uint8_t num_arbs_to_assign);

	ACTION dismissclaim(uint64_t case_id, name assigned_arb, uint64_t claim_id, string memo);

	ACTION acceptclaim(uint64_t case_id, name assigned_arb, uint64_t claim_id, string decision_link,
	        uint8_t decision_class);

	ACTION execclaim(uint64_t new_claim_id, uint64_t case_id, name assigned_arb, string claim_hash,
        string decision_link, uint8_t decision_class);

	ACTION advancecase(uint64_t case_id, name assigned_arb);

	ACTION dismisscase(uint64_t case_id, name assigned_arb, string ruling_link);

	ACTION setruling(uint64_t case_id, name assigned_arb, string case_ruling);

	ACTION recuse(uint64_t case_id, string rationale, name assigned_arb);

#pragma endregion Case_Actions

#pragma region Arb_Actions

	ACTION setlangcodes(name arbitrator, vector<uint16_t> lang_codes);

	ACTION newarbstatus(name arbitrator, uint8_t new_status);

	ACTION deletecase(uint64_t case_id);
	
#pragma endregion Arb_Actions

#pragma region BP_Multisig_Actions

	ACTION dismissarb(name arb, bool remove_from_cases);

	//TODO: affidavit action, forced recusal of arbitrator from a specified case.

#pragma endregion BP_Multisig_Actions

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
		string credentials_link; //NOTE: ipfs_url of arbitrator credentials
		time_point_sec elected_time;
		time_point_sec term_expiration;
		vector<uint16_t> languages; //NOTE: language codes

		uint64_t primary_key() const { return arb.value; }
		EOSLIB_SERIALIZE(arbitrator, (arb)(arb_status)(open_case_ids)(closed_case_ids)(credentials_link)
		    (elected_time)(term_expiration)(languages))
	};
	typedef multi_index<name("arbitrators"), arbitrator> arbitrators_table;


	//NOTE: Stores all information related to a single claim.
	TABLE claim
	{
		uint64_t claim_id;
		string claim_summary; //NOTE: ipfs link to claim document from claimant
		string decision_link; //NOTE: ipfs link to decision document from arbitrator
		string response_link; //NOTE: ipfs link to response document from respondant (if any)
		uint8_t status;
		uint8_t decision_class;

		uint64_t primary_key() const { return claim_id; }
		EOSLIB_SERIALIZE(claim, (claim_id)(claim_summary)(decision_link)(response_link)(status)(decision_class))
	};
	typedef multi_index<name("claims"), claim> claims_table;


	/**
   * Case Files for all arbitration cases.
   * @scope get_self().value
   * @key case_id
   */
	TABLE casefile
	{
		//NOTE: alternative table design. No secondary indices. Scope by claimant. Index by respondant.
		//pros: easy to track by claimant, no longer need secondary indexes.
		//cons: limited discoverability, must avoid secondary indexes.
		uint64_t case_id;
		uint8_t case_status;
		name claimant;
		name respondant;
		vector<name> arbitrators;
		vector<name> approvals;
		uint8_t number_claims;
		vector<uint16_t> required_langs;
		string case_ruling;
		time_point_sec update_ts; 

		uint64_t primary_key() const { return case_id; }

		uint64_t by_claimant() const { return claimant.value; }
		uint128_t by_uuid() const
		{
			uint128_t claimant_id = static_cast<uint128_t>(claimant.value);
			uint128_t respondant_id = static_cast<uint128_t>(respondant.value);
			return (claimant_id << 64) | respondant_id;
		}
		
		EOSLIB_SERIALIZE(casefile, (case_id)(case_status)(claimant)(respondant)(arbitrators)(approvals)
		(number_claims)(required_langs)(case_ruling)(update_ts))
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
		vector<int64_t> fee_structure = vector<int64_t> {2000000}; //NOTE: always in TLOS so only store asset.amount value
		uint32_t arb_term_length = 31536000; //365 days
		uint64_t current_election_id;
		//uint8_t max_claims_per_case = 21;
		asset available_funds = asset(0, TLOS_SYM);

		EOSLIB_SERIALIZE(config, (admin)(contract_version)(max_elected_arbs)(election_voting_ts)
			(runoff_election_voting_ts)(election_add_candidates_ts)(fee_structure)(arb_term_length)
			(current_election_id)/*(max_claims_per_case)*/(available_funds))
	};
	typedef singleton<name("config"), config> config_singleton;


	TABLE categories
	{
		uint8_t claim_class_id;
		asset initial_deposit;
		asset fee;
		uint16_t respondant_days;
		uint8_t number_arbitrators;

		uint64_t primary_key() const { return (uint64_t) claim_class_id; }
		EOSLIB_SERIALIZE(categories, (claim_class_id)(initial_deposit)(fee)(respondant_days)(number_arbitrators))
	};
	typedef multi_index<name("claimclass"), categories> claim_class_table;

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

	vector<permission_level_weight> get_arb_permissions();

	void set_permissions(vector<permission_level_weight> &perms);

	vector<claim>::iterator get_claim_at(string claim_hash, vector<claim>& claims);

	void sub_balance(name owner, asset value);

	void add_balance(name owner, asset value, name ram_payer);

	// mixes current transaction with seed and returns a hash
  	checksum256 get_rngseed(uint64_t seed);

	string get_rand_ballot_name();

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

