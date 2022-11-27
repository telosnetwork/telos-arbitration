const { loadConfig, Blockchain } = require("@klevoya/hydra");

const config = loadConfig("hydra.yml");

describe("Add Claim Telos Arbitration Smart Contract Tests", () => {
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
                case_status: 0,
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
                fee_paid_tlos: "0.0000 TLOS",
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

    it("add claim", async () => {
        expect.assertions(3);

        await arbitration.contract.addclaim({
            case_id: "0",
            claim_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
            claimant: "user1",
            claim_category: 3,
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])
        
        const casefiles = arbitration.getTableRowsScoped("casefiles")[arbitration.accountName];
        expect(casefiles.find(cf => cf.case_id === "0").number_claims).toEqual(2);  

        const claims = arbitration.getTableRowsScoped("claims")[""];
        expect(claims.length).toEqual(2);
        expect(claims.find(cl => cl.claim_id === "1")).toEqual({
            claim_id: '1',
            claim_summary: 'QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe',
            decision_link: '',
            response_link: '',
            status: 1,
            claim_category: 3,
            claim_info_needed: false,
            claim_info_required: '',
            response_info_needed: false,
            response_info_required: '',
            claimant_limit_time: "1970-01-01T00:00:00.000",
            respondant_limit_time: "1970-01-01T00:00:00.000"
        })
        
    });

    it("fails if case not found", async () => {
         await expect(arbitration.contract.addclaim({
            case_id: "1",
            claim_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
            claimant: "user1",
            claim_category: 3,
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow("Case Not Found");
    })

    it("fails if claim category not found", async () => {
         await expect(arbitration.contract.addclaim({
            case_id: "0",
            claim_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
            claimant: "user1",
            claim_category: 24,
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow("Claim category not found");
    })

    it("fails if case is not in setup mode", async () => {
        await arbitration.loadFixtures("casefiles", {
            arbitration: [{
                case_id: '1',
                case_status: 1,
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
                fee_paid_tlos: "0.0000 TLOS",
                arbitrator_cost_tlos: "150.0000 TLOS",
                sending_offers_until_ts: "2000-01-10T00:00:00.000"
            }]
        })

         await expect(arbitration.contract.addclaim({
            case_id: "1",
            claim_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
            claimant: "user1",
            claim_category: 3,
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow("claims cannot be added after CASE_SETUP is complete.");
    })

    it("fails if case has reached maximum number of claims", async () => {
        await arbitration.loadFixtures("casefiles", {
            arbitration: [{
                case_id: '1',
                case_status: 0,
                claimant: 'user1',
                respondant: 'user2',
                arbitrators: [],
                approvals: [],
                number_claims: 21,
                number_offers: 1,
                required_langs: [ 0, 1, 2 ],
                case_ruling: '',
                recusal: "",
                update_ts: '2000-01-01T00:00:00.000',
                fee_paid_tlos: "0.0000 TLOS",
                arbitrator_cost_tlos: "150.0000 TLOS",
                sending_offers_until_ts: "2000-01-10T00:00:00.000"
            }]
        })

         await expect(arbitration.contract.addclaim({
            case_id: "1",
            claim_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
            claimant: "user1",
            claim_category: 3,
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow("case file has reached maximum number of claims");
    })

    it("fails if claim link is invalid", async () => {
        await expect(arbitration.contract.addclaim({
            case_id: "0",
            claim_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5A",
            claimant: "user1",
            claim_category: 3,
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow("invalid ipfs string, valid schema: <hash>");
    })

    it("fails if claimant doesn't belong to the case", async () => {
        await expect(arbitration.contract.addclaim({
            case_id: "0",
            claim_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5ARe",
            claimant: "user2",
            claim_category: 3,
        },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow("you are not the claimant of this case.");
    })

    it("fails if claim link is already used in another claim", async () => {
        await expect(arbitration.contract.addclaim({
            case_id: "0",
            claim_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKdFe5a",
            claimant: "user1",
            claim_category: 3,
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow("Claim Link already exists in another claim");
    })
    
    it("fails if user other than claimant tries", async () => {
        await expect(arbitration.contract.addclaim({
            case_id: "0",
            claim_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5ARe",
            claimant: "user1",
            claim_category: 3,
        },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow();
    })

    
});