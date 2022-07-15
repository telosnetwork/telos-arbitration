const { loadConfig, Blockchain } = require("@klevoya/hydra");

const config = loadConfig("hydra.yml");

describe("Recuse case Telos Arbitration Smart Contract Tests", () => {
    let blockchain = new Blockchain(config);
    let arbitration = blockchain.createAccount("arbitration");
    let admin = blockchain.createAccount("admin");
    let user1 = blockchain.createAccount("user1");
    let user2 = blockchain.createAccount("user2");
    let user3 = blockchain.createAccount("user3");

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
        await arbitration.loadFixtures("config", {
            "arbitration": [
                {
                    "contract_version": "0.1.0",
                    "admin": "admin",
                    "max_elected_arbs": 2,
                    "election_voting_ts": 2505600,
                    "runoff_election_voting_ts": 1252800,
                    "election_add_candidates_ts": 604800,
                    "arb_term_length": 31536000,
                    "claimant_accepting_offers_ts":604800,
                    "current_election_id": "0",
                    "fee_usd": "10.0000 USD",
                    "available_funds": "15.0000 TLOS",
                    "reserved_funds": "300.0000 TLOS",
                    "max_claims_per_case": 21
                }
            ]
        });
        await arbitration.loadFixtures("arbitrators", require("../fixtures/arbitration/arbitrators.json"));

        await arbitration.loadFixtures("casefiles", {
            arbitration: [{
                case_id: '0',
                case_status: 4,
                claimant: 'user1',
                respondant: 'user2',
                arbitrators: ["user3"],
                approvals: [],
                number_claims: 1,
                number_offers: 1,
                required_langs: [ 0, 1, 2 ],
                case_ruling: '',
                recusal: "",
                update_ts: '2000-01-01T00:00:00.000',
                fee_paid_tlos: "100.0000 TLOS",
                arbitrator_cost_tlos: "150.0000 TLOS",
                sending_offers_until_ts: "2000-01-10T00:00:00.000"
            }]
            
        })

        await arbitration.loadFixtures("claims", {
            "": [
                {
                    claim_id: '0',
                    claim_summary: 'QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKdFe5a',
                    decision_link: '',
                    response_link: 'QmTtDqW001TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5gj7',
                    status: 4,
                    claim_category: 1, 
                    claimant_limit_time: '2000-01-01T00:00:00.000',
                    claim_info_needed: false,
                    respondant_limit_time: '2000-01-01T00:00:00.000',
                    response_info_needed: false,
                }
            ]
        })

    });

    it("recuse a case", async () => {
        expect.assertions(5);
        
        await arbitration.contract.recuse({ case_id: "0", rationale: "Recusal", assigned_arb: "user3" },
            [{
                actor: user3.accountName,
                permission: "active"
            }]);
        
        const casefiles = arbitration.getTableRowsScoped("casefiles")[arbitration.accountName];
        expect(casefiles.find(cf => cf.case_id === "0").case_status).toEqual(9);

        const arbitrators = arbitration.getTableRowsScoped("arbitrators")[arbitration.accountName];
        expect(arbitrators.find(arb => arb.arb === "user3").open_case_ids).toEqual([]);
        expect(arbitrators.find(arb => arb.arb === "user3").recused_case_ids).toEqual(["0"]);

        const balances = arbitration.getTableRowsScoped("accounts");
        expect(balances["user1"][0]).toEqual({ balance: "250.0000 TLOS" });

        const config = arbitration.getTableRowsScoped("config")[arbitration.accountName][0];
        expect(config.reserved_funds).toEqual("50.0000 TLOS");
    });

    it("fails if case not found", async () => {
        await expect(arbitration.contract.recuse({ case_id: "5", rationale: "Recusal", assigned_arb: "user3" },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("Case not found");
    });

    it("fails if trying to recuse a finished case", async () => {
        await arbitration.loadFixtures("casefiles", {
            arbitration: [{
                case_id: '1',
                case_status: 7,
                claimant: 'user1',
                respondant: 'user2',
                arbitrators: ["user3"],
                approvals: [],
                number_claims: 1,
                number_offers: 1,
                required_langs: [ 0, 1, 2 ],
                case_ruling: '',
                recusal: "",
                update_ts: '2000-01-01T00:00:00.000',
                fee_paid_tlos: "0.0000 TLOS",
                arbitrator_cost_tlos: "0.0000 TLOS",
                sending_offers_until_ts: "2000-01-10T00:00:00.000"
            }]
            
        })
        
         await expect(arbitration.contract.recuse({ case_id: "1", rationale: "Recusal", assigned_arb: "user3" },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("Unable to recuse if the case has not started or it is resolved");
    });

    it("fails if trying to recuse a case that has not started", async () => {
         await arbitration.loadFixtures("casefiles", {
            arbitration: [{
                case_id: '1',
                case_status: 1,
                claimant: 'user1',
                respondant: 'user2',
                arbitrators: ["user3"],
                approvals: [],
                number_claims: 1,
                number_offers: 1,
                required_langs: [ 0, 1, 2 ],
                case_ruling: '',
                recusal: "",
                update_ts: '2000-01-01T00:00:00.000',
                fee_paid_tlos: "0.0000 TLOS",
                arbitrator_cost_tlos: "0.0000 TLOS",
                sending_offers_until_ts: "2000-01-10T00:00:00.000"
            }]
            
         })
        
         await expect(arbitration.contract.recuse({ case_id: "1", rationale: "Recusal", assigned_arb: "user3" },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("Unable to recuse if the case has not started or it is resolved");
    });

    it("fails if arbitrator is not selected for the case", async () => {
        await expect(arbitration.contract.recuse({ case_id: "0", rationale: "Recusal", assigned_arb: "user2" },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow("Arbitrator isn't selected for this case.");
    });

    it("fails if user other than arbitrator tries", async () => {
        await expect(arbitration.contract.recuse({ case_id: "0", rationale: "Recusal", assigned_arb: "user3" },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow();
    });
    
});
