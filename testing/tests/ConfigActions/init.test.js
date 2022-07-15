const { loadConfig, Blockchain } = require("@klevoya/hydra");

const config = loadConfig("hydra.yml");

describe("Init Telos Arbitration Smart Contract Tests", () => {
    let blockchain = new Blockchain(config);
    let arbitration = blockchain.createAccount("arbitration");
    let admin = blockchain.createAccount("admin");

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
    });

    it("init the SC", async () => {
        expect.assertions(1);

        await arbitration.contract.init({
            initial_admin: "admin"
        })

        expect(arbitration.getTableRowsScoped("config")["arbitration"][0]).toEqual({
            contract_version: "0.1.0",
            admin: "admin",
            max_elected_arbs: 21,
            election_voting_ts: 2505600,
            runoff_election_voting_ts: 604800,
            election_add_candidates_ts: 604800,
            arb_term_length: 31536000,
            current_election_id: "0",
            available_funds: "0.0000 TLOS",
            max_claims_per_case: 21,
            claimant_accepting_offers_ts: 604800,
            fee_usd: "10.0000 USD",
            reserved_funds: "0.0000 TLOS",
        });
    });

    it("return an error if contract is already initialized", async () => {
        await arbitration.loadFixtures("config", require("../fixtures/arbitration/config.json"));

        await expect(arbitration.contract.init({
            initial_admin: "admin"
        })).rejects.toThrow("contract already initialized")
    
    });


    it("return an error if admin account doesn't exist", async () => {
        await expect(arbitration.contract.init({
            initial_admin: "falseadmin"
        })).rejects.toThrow("initial admin account doesn't exist")
    });
});
