#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/asset.hpp>
#include <eosio/action.hpp>

using namespace eosio;
using namespace std;

namespace delphioracle {

    TABLE datapoints {
        uint64_t id;
        name owner; 
        uint64_t value;
        uint64_t median;
        time_point timestamp;

        uint64_t primary_key() const {return id;}
        uint64_t by_timestamp() const {return timestamp.elapsed.to_seconds();}
        uint64_t by_value() const {return value;}
    };

    typedef eosio::multi_index<name("datapoints"), datapoints,
      indexed_by<name("value"), const_mem_fun<datapoints, uint64_t, &datapoints::by_value>>, 
      indexed_by<name("timestamp"), const_mem_fun<datapoints, uint64_t, &datapoints::by_timestamp>>> datapoints_table;
};