const { loadConfig, Blockchain } = require("@klevoya/hydra");

const config = loadConfig("hydra.yml");

describe("Set ruling Telos Arbitration Smart Contract Tests", () => {
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


        await arbitration.loadFixtures("casefiles", {
            arbitration: [{
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
                    claim_info_required: '',
                    respondant_limit_time: '2000-01-01T00:00:00.000',
                    response_info_needed: false,
                    response_info_required: '',
                }
            ]
        })

    });

    it("sets a ruling", async () => {
        await arbitration.contract.setruling({
            case_id: "0",
            assigned_arb: "user3",
            case_ruling: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
        }, [{
             actor: user3.accountName,
              permission: "active"
        }]);
            
        const casefiles = arbitration.getTableRowsScoped("casefiles")[arbitration.accountName];
        expect(casefiles.find(cf => cf.case_id === "0")).toEqual({
            case_id: '0',
            case_status: 4,
            claimant: 'user1',
            respondant: 'user2',
            arbitrators: ["user3"],
            approvals: [],
            number_claims: 1,
            number_offers: 1,
            required_langs: [ 0, 1, 2 ],
            case_ruling: 'QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe',
            update_ts: '2000-01-01T00:00:00.000',
            fee_paid_tlos: "100.0000 TLOS",
            arbitrator_cost_tlos: "150.0000 TLOS",
            sending_offers_until_ts: "2000-01-10T00:00:00.000"
        });  
    });

    it("fails if a claim is not resolved", async () => {
        await arbitration.loadFixtures("claims", {
            "": [
                {
                    claim_id: '1',
                    claim_summary: 'QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKdFe5a',
                    decision_link: '',
                    response_link: 'QmTtDqW001TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5gj7',
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

        await expect(arbitration.contract.setruling({
            case_id: "0",
            assigned_arb: "user3",
            case_ruling: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("There are claims that has not been resolved");
    })

    it("fails if not valid ipfs", async () => {
        await expect(arbitration.contract.setruling({
            case_id: "0",
            assigned_arb: "user3",
            case_ruling: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6d5AFe",
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("invalid ipfs string, valid schema: <hash>");
    })

    it("fails if case not found", async () => {
         await expect(arbitration.contract.setruling({
            case_id: "3",
            assigned_arb: "user3",
            case_ruling: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("Case not found");
    })

    it("fails if user is not an arbitrator", async () => {
        await expect(arbitration.contract.setruling({
            case_id: "0",
            assigned_arb: "user2",
            case_ruling: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
        },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow("Only an assigned arbitrator can set a ruling");
    });

    it("fails if case is not in case investigation status", async () => {
         await arbitration.loadFixtures("casefiles", {
            arbitration: [{
                case_id: '1',
                case_status: 5,
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

        await expect(arbitration.contract.setruling({
            case_id: "1",
            assigned_arb: "user3",
            case_ruling: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("Case status must be CASE INVESTIGATION");
    });
    
    it("fails if user other than arb tries to set a ruling", async () => {
        await expect(arbitration.contract.setruling({
            case_id: "0",
            assigned_arb: "user3",
            case_ruling: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
        },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow();
        
    });

    
});
