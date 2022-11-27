const { loadConfig, Blockchain } = require("@klevoya/hydra");

const config = loadConfig("hydra.yml");

describe("Respond Telos Arbitration Smart Contract Tests", () => {
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
                arbitrators: [],
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
                    claimant_limit_time: '2000-01-01T00:00:00.000',
                    claim_info_needed: false,
                    claim_info_required: '',
                    respondant_limit_time: '2000-01-10T00:00:00.000',
                    response_info_needed: true,
                    response_info_required: 'Update response info',
                    claim_category: 3,
                }
            ]
        })

    });

    it("responds a claim", async () => {
        expect.assertions(1);

        await arbitration.contract.respond({
            case_id: "0",
            claim_id: "0",
            respondant: "user2",
            response_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
        },
            [{
              actor: user2.accountName,
              permission: "active"
            }])
        
        const claims = arbitration.getTableRowsScoped("claims")[""];
        expect(claims.find(c => c.claim_id === "0")).toEqual({
            claim_id: '0',
            claim_summary: 'QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKdFe5a',
            decision_link: '',
            response_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
            status: 2,
            claim_category: 3,
            claim_info_needed: false,
            claim_info_required: '',
            response_info_needed: false,
            response_info_required: 'Update response info',
            claimant_limit_time: "2000-01-01T00:00:00.000",
            respondant_limit_time: "2000-01-10T00:00:00.000"
        })
    });

    it("fails if case not found", async () => {
        await expect(arbitration.contract.respond({
            case_id: "1",
            claim_id: "0",
            respondant: "user2",
            response_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
        },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow("Case not found");
    })

    it("fails if not response needed", async () => {
        await arbitration.loadFixtures("claims", {
            "": [
                {
                    claim_id: '1',
                    claim_summary: 'QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKdFe5a',
                    decision_link: '',
                    response_link: '',
                    status: 1,
                    claimant_limit_time: '2000-01-01T00:00:00.000',
                    claim_info_needed: false,
                    claim_info_required: '',
                    respondant_limit_time: '2000-01-01T00:00:00.000',
                    response_info_needed: false,
                    response_info_required: '',
                    claim_category: 3,
                }
            ]
        })

        await expect(arbitration.contract.respond({
            case_id: "0",
            claim_id: "1",
            respondant: "user2",
            response_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
        },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow("No response needed")
    })

    it("fails if case does not have a respondant", async () => {
        await arbitration.loadFixtures("casefiles", {
            arbitration: [{
                case_id: '1',
                case_status: 3,
                claimant: 'user1',
                respondant: '',
                arbitrators: [],
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
                    status: 1,
                    claimant_limit_time: '2000-01-01T00:00:00.000',
                    claim_info_needed: false,
                    claim_info_required: '',
                    respondant_limit_time: '2000-01-01T00:00:00.000',
                    response_info_needed: false,
                    response_info_required: '',
                    claim_category: 3,
                }
            ]
        })

        await expect(arbitration.contract.respond({
            case_id: "1",
            claim_id: "0",
            respondant: "user2",
            response_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
        },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow("case_id does not have a respondant");
    })

    it("fails if user is not the respondant", async () => {
        await expect(arbitration.contract.respond({
            case_id: "0",
            claim_id: "0",
            respondant: "user3",
            response_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("must be the respondant of this case_id");
    })

    it("fails if case is not in investigation mode", async () => {
        await arbitration.loadFixtures("casefiles", {
            arbitration: [{
                case_id: '1',
                case_status: 5,
                claimant: 'user1',
                respondant: 'user2',
                arbitrators: [],
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
                    status: 1,
                    claimant_limit_time: '2000-01-01T00:00:00.000',
                    claim_info_needed: false,
                    claim_info_required: '',
                    respondant_limit_time: '2000-01-01T00:00:00.000',
                    response_info_needed: false,  
                    response_info_required: '',
                    claim_category: 3,
                }
            ]
        })

        await expect(arbitration.contract.respond({
            case_id: "1",
            claim_id: "0",
            respondant: "user2",
            response_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
        },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow("case status does NOT allow responses at this time");
    })


    it("fails if claim not found", async () => {
        await expect(arbitration.contract.respond({
            case_id: "0",
            claim_id: "1",
            respondant: "user2",
            response_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
        },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow("Claim not found");
    })

    it("fails if claim is not in filed status", async () => {
        await arbitration.loadFixtures("claims", {
            "": [
                {
                    claim_id: '1',
                    claim_summary: 'QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKdFe5a',
                    decision_link: '',
                    response_link: '',
                    status: 3,
                    claimant_limit_time: '2000-01-01T00:00:00.000',
                    claim_info_needed: false,
                    claim_info_required: '',
                    respondant_limit_time: '2000-01-01T00:00:00.000',
                    response_info_needed: false,
                    response_info_required: '',
                    claim_category: 3,
                }
            ]
         })
        
        await expect(arbitration.contract.respond({
            case_id: "0",
            claim_id: "1",
            respondant: "user2",
            response_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
        },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow("Claim must be in FILED status");
    })

    
    it("fails if response link is invalid", async () => {
        await expect(arbitration.contract.respond({
            case_id: "0",
            claim_id: "0",
            respondant: "user2",
            response_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5A",
        },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow("invalid ipfs string, valid schema: <hash>");
    })

    
    it("fails if user other than respondant tries", async () => {
        await expect(arbitration.contract.respond({
            case_id: "0",
            claim_id: "0",
            respondant: "user2",
            response_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow();
    })

    
});