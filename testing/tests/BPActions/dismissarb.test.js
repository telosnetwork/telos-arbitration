const { loadConfig, Blockchain } = require("@klevoya/hydra");

const config = loadConfig("hydra.yml");

describe("Dismiss Arb Telos Arbitration Smart Contract Tests", () => {
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
            arbitration: [
                {
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
                },
                {
                    case_id: '1',
                    case_status: 8,
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
                }

            ]            
        })

    });

    it("dismiss an arb but not close cases", async () => {
        await arbitration.contract.dismissarb({ arbitrator: "user3", remove_from_cases: false },
        [{
            actor: admin.accountName,
            permission: "active"
            }])
        
        const arbitrators = arbitration.getTableRowsScoped("arbitrators")[arbitration.accountName];
        expect(arbitrators.find(arb => arb.arb === "user3").arb_status).toEqual(3);
    });

    it("dismiss an arb and close cases", async () => {
        expect.assertions(7);
        
        await arbitration.contract.dismissarb({ arbitrator: "user3", remove_from_cases: true },
            [{
                actor: admin.accountName,
                permission: "active"
            }]);
        
        const arbitrators = arbitration.getTableRowsScoped("arbitrators")[arbitration.accountName];
        expect(arbitrators.find(arb => arb.arb === "user3").arb_status).toEqual(3);
        expect(arbitrators.find(arb => arb.arb === "user3").recused_case_ids).toEqual(["0"]);
        expect(arbitrators.find(arb => arb.arb === "user3").open_case_ids).toEqual([]);

        const casefiles = arbitration.getTableRowsScoped("casefiles")[arbitration.accountName];
        expect(casefiles.find(cf => cf.case_id === "0").case_status).toEqual(9);
        expect(casefiles.find(cf => cf.case_id === "1").case_status).toEqual(8);

        const config = arbitration.getTableRowsScoped("config")[arbitration.accountName][0];
        expect(config.reserved_funds).toEqual("50.0000 TLOS");

        const accounts = arbitration.getTableRowsScoped("accounts")[user1.accountName];
        expect(accounts[0].balance).toEqual("250.0000 TLOS");
    });

    it("fails if arbitrator not found", async () => {
        await expect(arbitration.contract.dismissarb({ arbitrator: "user2", remove_from_cases: false },
            [{
              actor: admin.accountName,
              permission: "active"
            }])).rejects.toThrow("Arbitrator not found");
    });

    it("fails if arbitrator is already removed", async () => {
        await arbitration.loadFixtures("arbitrators", {
            "arbitration": [
                {   
                    arb: "user2",
                    arb_status: 3,
                    open_case_ids: [],
                    closed_case_ids: [],
                    recused_case_ids: [],
                    credentials_link: "link",
                    elected_time: '1999-01-01T00:00:00.000',
                    term_expiration:'2001-01-01T00:00:00.000',
                    languages: [0,1]
                }
            ]
        })
        
         await expect(arbitration.contract.dismissarb({ arbitrator: "user2", remove_from_cases: false },
            [{
              actor: admin.accountName,
              permission: "active"
            }])).rejects.toThrow("Arbitrator is already removed");
    });

    it("fails if arbitrators seat has expired", async () => {
         await arbitration.loadFixtures("arbitrators", {
            "arbitration": [
                {   
                    arb: "user2",
                    arb_status: 4,
                    open_case_ids: [],
                    closed_case_ids: [],
                    recused_case_ids: [],
                    credentials_link: "link",
                    elected_time: '1999-01-01T00:00:00.000',
                    term_expiration:'2001-01-01T00:00:00.000',
                    languages: [0,1]
                }
            ]
         })
        
         await expect(arbitration.contract.dismissarb({ arbitrator: "user2", remove_from_cases: false },
            [{
              actor: admin.accountName,
              permission: "active"
            }])).rejects.toThrow("Arbitrator term has expired");
    });

    it("fails if user other than admin tries", async () => {
        await expect(arbitration.contract.dismissarb({ arbitrator: "user3", remove_from_cases: false },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow();
    });
    
});
