const { loadConfig, Blockchain } = require("@klevoya/hydra");

const config = loadConfig("hydra.yml");

describe("Set Config Telos Arbitration Smart Contract Tests", () => {
    let blockchain = new Blockchain(config);
    let arbitration = blockchain.createAccount("arbitration");
    let admin = blockchain.createAccount("admin");
    let user1 = blockchain.createAccount("user1");

    beforeAll(async () => {
        arbitration.setContract(blockchain.contractTemplates[`arbitration`]);
        arbitration.updateAuth(`active`, `owner`, {
        accounts: [
            {
            permission: {
                actor: arbitration.accountName,
                permission: `eosio.code`
            },
            weight: 1
            }
        ]
        });
    });

    beforeEach(async () => {
        arbitration.resetTables();

        await arbitration.loadFixtures("config", require("../fixtures/arbitration/config.json"));

    });

    it("change current config", async () => {
        expect.assertions(1);

        await arbitration.contract.setconfig({ max_elected_arbs: 25, election_duration: 3600, runoff_duration: 1800,
          election_add_candidates_duration: 1800, arbitrator_term_length: 86400, fees: [], max_claims_per_case: 11,
          fee_usd: "10.0000 USD", claimant_accepting_offers_duration: 604800},
          [{
            actor: admin.accountName,
            permission: "active"
          }]);

        expect(arbitration.getTableRowsScoped("config")["arbitration"][0]).toEqual({
            contract_version: "0.1.0",
            admin: "admin",
            max_elected_arbs: 25,
            election_voting_ts: 3600,
            runoff_election_voting_ts: 1800,
            election_add_candidates_ts: 1800,
            arb_term_length: 86400,
            current_election_id: "0",
            available_funds: "15.0000 TLOS",
            max_claims_per_case: 11,
            fee_usd: "10.0000 USD",
            claimant_accepting_offers_ts: 604800,
            reserved_funds: "0.0000 TLOS"
        });
    })

    it("do not change current config if max elected arbitrators is 0", async () => {

        await expect(arbitration.contract.setconfig({ max_elected_arbs: 0, election_duration: 3600, runoff_duration: 1800,
          election_add_candidates_duration: 1800, arbitrator_term_length: 86400, fees: [], max_claims_per_case: 11,
          fee_usd: "10.0000 USD", claimant_accepting_offers_duration: 604800},
          [{
            actor: admin.accountName,
            permission: "active"
          }]
        )).rejects.toThrow("Arbitrators must be greater than 0");
    })
  
  it("do not change current config if max claims per case is 0", async () => {

        await expect(arbitration.contract.setconfig({ max_elected_arbs: 2, election_duration: 3600, runoff_duration: 1800,
          election_add_candidates_duration: 1800, arbitrator_term_length: 86400, fees: [], max_claims_per_case: 0,
          fee_usd: "10.0000 USD", claimant_accepting_offers_duration: 604800},
          [{
            actor: admin.accountName,
            permission: "active"
          }]
        )).rejects.toThrow("Minimum 1 claim");
    })
  
  

    it("fails to set new config if user different than admin tries", async () => {
        await expect(arbitration.contract.setconfig({ max_elected_arbs: 25, election_duration: 3600, runoff_duration: 1800,
          election_add_candidates_duration: 1800, arbitrator_term_length: 86400, fees: [], max_claims_per_case: 11,
          fee_usd: "10.0000 USD", claimant_accepting_offers_duration: 604800},
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow();
    })

});