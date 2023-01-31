const { loadConfig, Blockchain } = require("@klevoya/hydra");

const config = loadConfig("hydra.yml");

describe("Starts case Telos Arbitration Smart Contract Tests", () => {
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
        await arbitration.loadFixtures("config", require("../fixtures/arbitration/config.json"));
        await arbitration.loadFixtures("arbitrators", {   
            arbitration: [{
                "arb": "user3",
                "arb_status": 1,
                "open_case_ids": [],
                "closed_case_ids": [],
                "recused_case_ids": [],
                "credentials_link": "link",
                "elected_time": "1999-01-01T00:00:00.000",
                "term_expiration":"2001-01-01T00:00:00.000",
                "languages": [0,1]
            }]
        });

        await arbitration.loadFixtures("casefiles", {
            arbitration: [{
                case_id: '0',
                case_status: 2,
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
                    response_link: '',
                    status: 1,
                    claim_category: 1, 
                    claimant_limit_time: '2000-01-01T00:00:00.000',
                    claim_info_needed: false,
                    claim_info_required: '',
                    respondant_limit_time: '2000-01-01T00:00:00.000',
                    response_info_needed: false,
                    response_info_required: '',
                }
            ]
        })

    });

    it("starts a case", async () => {
        await arbitration.contract.startcase({
            case_id: "0",
            assigned_arb: "user3",
            number_days_respondant: 10,
            response_info_required: "Response info"
        }, [{
             actor: user3.accountName,
              permission: "active"
        }]);
            
        const casefiles = arbitration.getTableRowsScoped("casefiles")[arbitration.accountName];
        expect(casefiles.find(cf => cf.case_id === "0")).toEqual({
            case_id: '0',
            case_status: 3,
            claimant: 'user1',
            respondant: 'user2',
            arbitrators: ["user3"],
            approvals: [],
            number_claims: 1,
            number_offers: 1,
            required_langs: [ 0, 1, 2 ],
            case_ruling: '',
            update_ts: '2000-01-01T00:00:00.000',
            fee_paid_tlos: "100.0000 TLOS",
            arbitrator_cost_tlos: "150.0000 TLOS",
            sending_offers_until_ts: "2000-01-10T00:00:00.000"
        }); 
        
        const claims = arbitration.getTableRowsScoped("claims")[""];
        expect(claims.find(cl => cl.claim_id === "0")).toEqual({
            claim_id: '0',
            claim_summary: 'QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKdFe5a',
            decision_link: '',
            response_link: '',
            status: 1,
            claim_category: 1,
            claim_info_needed: false,
            claim_info_required: '',
            response_info_needed: true,
            response_info_required: 'Response info',
            claimant_limit_time: "2000-01-01T00:00:00.000",
            respondant_limit_time: "2000-01-11T00:00:00.000"
        })

        const arbitrators = arbitration.getTableRowsScoped("arbitrators")[arbitration.accountName];
        expect(arbitrators.find(arb => arb.arb === "user3").open_case_ids).toEqual(["0"]);
    });

    it("fails if case not found", async () => {
         await expect(arbitration.contract.startcase({
            case_id: "2",
            assigned_arb: "user3",
            number_days_respondant: 10,
            response_info_required: "Response info"
         },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("Case not found");
    })

    it("fails if user is not an arbitrator", async () => {
        await expect(arbitration.contract.startcase({
            case_id: "0",
            assigned_arb: "user2",
            number_days_respondant: 10,
            response_info_required: "Response info"
        },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow("Only an assigned arbitrator can start a case");
    });

    it("fails if case is not in arbs assigned status", async () => {
         await arbitration.loadFixtures("casefiles", {
            arbitration: [{
                case_id: '1',
                case_status: 6,
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
            "............1": [
                {
                    claim_id: '0',
                    claim_summary: 'QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKdFe5a',
                    decision_link: '',
                    response_link: '',
                    status: 2,
                    claim_category: 1, 
                    claimant_limit_time: '2000-01-01T00:00:00.000',
                    claim_info_needed: false,
                    claim_info_required: '',
                    respondant_limit_time: '2000-01-01T00:00:00.000',
                    response_info_needed: false,
                    response_info_required: '',
                }
            ]
        })

        await expect(arbitration.contract.startcase({
            case_id: "1",
            assigned_arb: "user3",
            number_days_respondant: 10,
            response_info_required: "Response info"
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("Case status must be in ARBS_ASSIGNED");
    });
    
    it("fails if user other than arb tries to start a case", async () => {
        await expect(arbitration.contract.startcase({
            case_id: "0",
            assigned_arb: "user3",
            number_days_respondant: 10,
            response_info_required: "Response info"
        },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow();
    });

    
});
