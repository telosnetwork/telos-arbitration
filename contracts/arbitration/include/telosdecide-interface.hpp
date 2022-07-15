#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/asset.hpp>
#include <eosio/action.hpp>

using namespace eosio;
using namespace std;

namespace telosdecide {

    static constexpr name TELOSDECIDE = name("telos.decide");

    //scope: singleton
    //ram: 
    TABLE config {
        string app_name;
        string app_version;
        asset total_deposits;
        map<name, asset> fees; //ballot, treasury, archival
        map<name, uint32_t> times; //balcooldown, minballength, forfeittime

        EOSLIB_SERIALIZE(config, (app_name)(app_version)(total_deposits)(fees)(times))
    };
    typedef singleton<name("config"), config> config_singleton;

    //scope: DECIDE.value
    //ram: 
    TABLE treasury {
        asset supply; //current supply
        asset max_supply; //maximum supply
        name access; //public, private, invite, membership
        name manager; //treasury manager

        string title;
        string description;
        string icon;

        uint32_t voters;
        uint32_t delegates;
        uint32_t committees;
        uint32_t open_ballots;

        bool locked; //locks all settings
        name unlock_acct; //account name to unlock
        name unlock_auth; //authorization name to unlock
        map<name, bool> settings; //setting_name -> on/off

        uint64_t primary_key() const { return supply.symbol.code().raw(); }
        EOSLIB_SERIALIZE(treasury, 
            (supply)(max_supply)(access)(manager)
            (title)(description)(icon)
            (voters)(delegates)(committees)(open_ballots)
            (locked)(unlock_acct)(unlock_auth)(settings))
    };
    typedef multi_index<name("treasuries"), treasury> treasuries_table;

    //scope: treasury_symbol.code().raw()
    //ram:
    TABLE payroll {
        name payroll_name; //workers, delegates
        asset payroll_funds; //TLOS, TLOSD

        uint32_t period_length; //in seconds
        asset per_period; //amount made claimable from payroll funds every period
        time_point_sec last_claim_time; //last time pay was claimed

        asset claimable_pay; //funds tapped when claimpayment() is called
        name payee; //craig.tf, workers, delegates

        uint64_t primary_key() const { return payroll_name.value; }
        EOSLIB_SERIALIZE(payroll, (payroll_name)(payroll_funds)
            (period_length)(per_period)(last_claim_time)
            (claimable_pay)(payee))
    };
    typedef multi_index<name("payrolls"), payroll> payrolls_table;

    //scope: treasury_symbol.code().raw()
    //ram: 
    TABLE labor_bucket {
        name payroll_name; //workers, delegates
        map<name, asset> claimable_volume; //rebalvolume, dgatevolume
        map<name, uint32_t> claimable_events; //rebalcount, cleancount, cleanspeed, rebalspeed, dgatecount

        uint64_t primary_key() const { return payroll_name.value; }
        EOSLIB_SERIALIZE(labor_bucket, (payroll_name)(claimable_volume)(claimable_events))
    };
    typedef multi_index<name("laborbuckets"), labor_bucket> laborbuckets_table;

    //scope: treasury_symbol.code().raw()
    //ram:
    TABLE labor {
        name worker_name;
        time_point_sec start_time; //time point work was first credited
        map<name, asset> unclaimed_volume; //rebalvolume
        map<name, uint32_t> unclaimed_events; //rebalcount, cleancount, cleanspeed, rebalspeed

        uint64_t primary_key() const { return worker_name.value; }
        EOSLIB_SERIALIZE(labor, (worker_name)(start_time)(unclaimed_volume)(unclaimed_events))
    };
    typedef multi_index<name("labors"), labor> labors_table;

    //scope: DECIDE.value
    //ram:
    TABLE ballot {
        name ballot_name;
        name category; //proposal, referendum, election, poll, leaderboard
        name publisher;
        name status; //setup, voting, closed, cancelled, archived

        string title; //markdown
        string description; //markdown
        string content; //IPFS link to content or markdown

        symbol treasury_symbol; //treasury used for counting votes
        name voting_method; //1acct1vote, 1tokennvote, 1token1vote, 1tsquare1v, quadratic
        uint8_t min_options; //minimum options per voter
        uint8_t max_options; //maximum options per voter
        map<name, asset> options; //option name -> total weighted votes

        uint32_t total_voters; //number of voters who voted on ballot
        uint32_t total_delegates; //number of delegates who voted on ballot
        asset total_raw_weight; //total raw weight cast on ballot
        uint32_t cleaned_count; //number of expired vote receipts cleaned
        map<name, bool> settings; //setting name -> on/off
        
        time_point_sec begin_time; //time that voting begins
        time_point_sec end_time; //time that voting closes

        uint64_t primary_key() const { return ballot_name.value; }
        uint64_t by_category() const { return category.value; }
        uint64_t by_status() const { return status.value; }
        uint64_t by_symbol() const { return treasury_symbol.code().raw(); }
        uint64_t by_end_time() const { return static_cast<uint64_t>(end_time.utc_seconds); }
        
        EOSLIB_SERIALIZE(ballot, 
            (ballot_name)(category)(publisher)(status)
            (title)(description)(content)
            (treasury_symbol)(voting_method)(min_options)(max_options)(options)
            (total_voters)(total_delegates)(total_raw_weight)(cleaned_count)(settings)
            (begin_time)(end_time))
    };
    typedef multi_index<name("ballots"), ballot,
        indexed_by<name("bycategory"), const_mem_fun<ballot, uint64_t, &ballot::by_category>>,
        indexed_by<name("bystatus"), const_mem_fun<ballot, uint64_t, &ballot::by_status>>,
        indexed_by<name("bysymbol"), const_mem_fun<ballot, uint64_t, &ballot::by_symbol>>,
        indexed_by<name("byendtime"), const_mem_fun<ballot, uint64_t, &ballot::by_end_time>>
    > ballots_table;

    //scope: ballot_name.value
    //ram: 
    TABLE vote {
        name voter;
        bool is_delegate;
        asset raw_votes;
        map<name, asset> weighted_votes;
        time_point_sec vote_time;
        
        name worker;
        uint8_t rebalances;
        asset rebalance_volume;

        uint64_t primary_key() const { return voter.value; }
        uint64_t by_time() const { return static_cast<uint64_t>(vote_time.utc_seconds); }
        EOSLIB_SERIALIZE(vote, 
            (voter)(is_delegate)(raw_votes)(weighted_votes)(vote_time)
            (worker)(rebalances)(rebalance_volume))
    };
    typedef multi_index<name("votes"), vote,
        indexed_by<name("bytime"), const_mem_fun<vote, uint64_t, &vote::by_time>>
    > votes_table;

    //scope: voter.value
    //ram: 
    TABLE voter {
        asset liquid;

        asset staked;
        time_point_sec staked_time;

        asset delegated;
        name delegated_to;
        time_point_sec delegation_time;

        uint64_t primary_key() const { return liquid.symbol.code().raw(); }
        EOSLIB_SERIALIZE(voter,
            (liquid)
            (staked)(staked_time)
            (delegated)(delegated_to)(delegation_time))
    };
    typedef multi_index<name("voters"), voter> voters_table;

    //scope: treasury_symbol.code().raw()
    //ram: 
    TABLE delegate {
        name delegate_name;
        asset total_delegated;
        uint32_t constituents;

        uint64_t primary_key() const { return delegate_name.value; }
        EOSLIB_SERIALIZE(delegate, (delegate_name)(total_delegated)(constituents))
    };
    typedef multi_index<name("delegates"), delegate> delegates_table;

    //scope: treasury_symbol.code().raw()
    //ram: 
    TABLE committee {
        string committee_title;
        name committee_name;

        symbol treasury_symbol;
        map<name, name> seats; //seat_name -> seat_holder (0 if empty)

        name updater_acct; //account name that can update committee members
        name updater_auth; //auth name that can update committee members

        uint64_t primary_key() const { return committee_name.value; }
        EOSLIB_SERIALIZE(committee, 
            (committee_title)(committee_name)
            (treasury_symbol)(seats)
            (updater_acct)(updater_auth))
    };
    typedef multi_index<name("committees"), committee> committees_table;

    //scope: DECIDE.value
    //ram:
    TABLE archival {
        name ballot_name;
        time_point_sec archived_until;

        uint64_t primary_key() const { return ballot_name.value; }
        EOSLIB_SERIALIZE(archival, (ballot_name)(archived_until))
    };
    typedef multi_index<name("archivals"), archival> archivals_table;

    //scope: treasury_symbol.code().raw()
    //ram:
    TABLE featured_ballot {
        name ballot_name;
        time_point_sec featured_until;

        uint64_t primary_key() const { return ballot_name.value; }
        EOSLIB_SERIALIZE(featured_ballot, (ballot_name)(featured_until))
    };
    typedef multi_index<name("featured"), featured_ballot> featured_table;

    //scope: account_name.value
    //ram: 
    TABLE account {
        asset balance;

        uint64_t primary_key() const { return balance.symbol.code().raw(); }
        EOSLIB_SERIALIZE(account, (balance))
    };
    typedef multi_index<name("accounts"), account> accounts_table;

    
};