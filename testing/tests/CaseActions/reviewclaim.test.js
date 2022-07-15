const { loadConfig, Blockchain } = require("@klevoya/hydra");

const config = loadConfig("hydra.yml");

describe("Review claim Telos Arbitration Smart Contract Tests", () => {
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
                case_status:3,
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
                    status: 2,
                    claim_category: 1, 
                    claimant_limit_time: '2000-01-01T00:00:00.000',
                    claim_info_needed: false,
                    respondant_limit_time: '2000-01-01T00:00:00.000',
                    response_info_needed: false,
                }
            ]
        })

    });

    it("reviews a claim and adds extra time for respondant and claimant", async () => {
        await arbitration.contract.reviewclaim({
            case_id: "0",
            assigned_arb: "user3",
            claim_id: "0",
            claim_info_needed: true,
            response_info_needed: true,
            number_days_claimant: 4,
            number_days_respondant: 4
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])
        
        const claims = arbitration.getTableRowsScoped("claims")[""];
        expect(claims.find(cl => cl.claim_id === "0")).toEqual({
            claim_id: '0',
            claim_summary: 'QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKdFe5a',
            decision_link: '',
            response_link: 'QmTtDqW001TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5gj7',
            status: 2,
            claim_category: 1,
            claim_info_needed: true,
            response_info_needed: true,
            claimant_limit_time: "2000-01-05T00:00:00.000",
            respondant_limit_time: "2000-01-05T00:00:00.000"
        })
    })

    it("fails if case is not in investigation status", async () => {
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
                    respondant_limit_time: '2000-01-01T00:00:00.000',
                    response_info_needed: false,
                }
            ]
        })

        await expect(arbitration.contract.reviewclaim({
            case_id: "1",
            assigned_arb: "user3",
            claim_id: "0",
            claim_info_needed: true,
            response_info_needed: true,
            number_days_claimant: 4,
            number_days_respondant: 0
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("To review a claim, case should be in investigation status");

        
    })

    it("fails if case not found", async () => {
        await expect(arbitration.contract.reviewclaim({
            case_id: "2",
            assigned_arb: "user3",
            claim_id: "1",
            claim_info_needed: true,
            response_info_needed: true,
            number_days_claimant: 4,
            number_days_respondant: 0
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("Case not found");
         
    })
    

    it("fails if not assigned arbitrator", async() => {
        await expect(arbitration.contract.reviewclaim({
            case_id: "0",
            assigned_arb: "user2",
            claim_id: "1",
            claim_info_needed: true,
            response_info_needed: true,
            number_days_claimant: 4,
            number_days_respondant: 0
        },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow("Only an assigned arbitrator can review a claim");
    })
    
    it("fails if claim not found", async () => {
        await expect(arbitration.contract.reviewclaim({
            case_id: "0",
            assigned_arb: "user3",
            claim_id: "1",
            claim_info_needed: true,
            response_info_needed: true,
            number_days_claimant: 4,
            number_days_respondant: 0
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("Claim not found");
    })

    it("fails if claim status is accepted", async () => {
        await arbitration.loadFixtures("claims", {
            "": [
                {
                    claim_id: '1',
                    claim_summary: 'QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKdFe5a',
                    decision_link: '',
                    response_link: 'QmTtDqW001TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5gj7',
                    status: 3,
                    claim_category: 1, 
                    claimant_limit_time: '2000-01-01T00:00:00.000',
                    claim_info_needed: false,
                    respondant_limit_time: '2000-01-01T00:00:00.000',
                    response_info_needed: false,
                }
            ]
        })

        await expect(arbitration.contract.reviewclaim({
            case_id: "0",
            assigned_arb: "user3",
            claim_id: "1",
            claim_info_needed: true,
            response_info_needed: true,
            number_days_claimant: 4,
            number_days_respondant: 0
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("Claim status needs to be filed or responded to review");
    })

    it("fails if claim status is dismissed", async () => {
        await arbitration.loadFixtures("claims", {
            "": [
                {
                    claim_id: '1',
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

        await expect(arbitration.contract.reviewclaim({
            case_id: "0",
            assigned_arb: "user3",
            claim_id: "1",
            claim_info_needed: true,
            response_info_needed: true,
            number_days_claimant: 4,
            number_days_respondant: 0
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("Claim status needs to be filed or responded to review");
    })

    it("fails if not reviewing neither claim or response", async () => {
        await expect(arbitration.contract.reviewclaim({
            case_id: "0",
            assigned_arb: "user3",
            claim_id: "0",
            claim_info_needed: false,
            response_info_needed: false,
            number_days_claimant: 4,
            number_days_respondant: 0
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("Need to update at least respondant or claimant");
    })

    it("fails if reviewing claim and not adding any extra day", async () => {
        await expect(arbitration.contract.reviewclaim({
            case_id: "0",
            assigned_arb: "user3",
            claim_id: "0",
            claim_info_needed: true,
            response_info_needed: true,
            number_days_claimant: 0,
            number_days_respondant: 1
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("At least one day has to be given to the claimant");
    })

    it("fails if reviewing response and not adding any extra day", async () => {
        await expect(arbitration.contract.reviewclaim({
            case_id: "0",
            assigned_arb: "user3",
            claim_id: "0",
            claim_info_needed: true,
            response_info_needed: true,
            number_days_claimant: 4,
            number_days_respondant: 0
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("At least one day has to be given to the respondant");
    })

    it("fails if user other than arbitrator tries", async () => {
        await expect(arbitration.contract.reviewclaim({
            case_id: "0",
            assigned_arb: "user3",
            claim_id: "0",
            claim_info_needed: true,
            response_info_needed: true,
            number_days_claimant: 4,
            number_days_respondant: 4
        },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow();
    })

    
});
