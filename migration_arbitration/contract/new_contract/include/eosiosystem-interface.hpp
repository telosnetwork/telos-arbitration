#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/asset.hpp>
#include <eosio/action.hpp>

using namespace eosio;
using namespace std;

namespace eosiosystem {

    TABLE producer_info {
      name                  owner;
      double                total_votes = 0;
      eosio::public_key     producer_key; /// a packed public key object
      bool                  is_active = true;
      std::string           unreg_reason;
      std::string           url;
      uint32_t              unpaid_blocks = 0;
      uint32_t              lifetime_produced_blocks = 0;
      uint32_t              missed_blocks_per_rotation = 0;
      uint32_t              lifetime_missed_blocks = 0;
      time_point            last_claim_time;
      uint16_t              location = 0;
      uint32_t              kick_reason_id = 0;
      std::string           kick_reason;
      uint32_t              times_kicked = 0;
      uint32_t              kick_penalty_hours = 0;
      block_timestamp       last_time_kicked;

      uint64_t primary_key()const { return owner.value;                             }
      double   by_votes()const    { return is_active ? -total_votes : total_votes;  }
      bool     active()const      { return is_active;                               }
      void     deactivate()       { producer_key = public_key(); is_active = false; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( producer_info, (owner)(total_votes)(producer_key)(is_active)(unreg_reason)(url)
                        (unpaid_blocks)(lifetime_produced_blocks)(missed_blocks_per_rotation)(lifetime_missed_blocks)(last_claim_time)
                        (location)(kick_reason_id)(kick_reason)(times_kicked)(kick_penalty_hours)(last_time_kicked) )
   };

    typedef eosio::multi_index< "producers"_n, producer_info,
        indexed_by<"prototalvote"_n, const_mem_fun<producer_info, double, &producer_info::by_votes>  >
        > producers_table;
};