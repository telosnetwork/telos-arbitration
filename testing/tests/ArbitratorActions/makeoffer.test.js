const { loadConfig, Blockchain } = require("@klevoya/hydra");

const config = loadConfig("hydra.yml");

describe("Make Offer Telos Arbitration Smart Contract Tests", () => {
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
                arbitrator_cost_tlos: "0.0000 TLOS",
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
                    respondant_limit_time: '2000-01-01T00:00:00.000',
                    response_info_needed: false,
                }
            ]
         })
        
        await arbitration.loadFixtures("arbitrators", require("../fixtures/arbitration/arbitrators.json"));

    });

    it("make offer", async () => {
        await arbitration.contract.makeoffer({
            case_id: "0",
            offer_id: -1,
            arbitrator: "user3",
            hourly_rate: "10.0000 USD",
            estimated_hours: 10,
        },
            [{
                actor: user3.accountName,
                permission: "active"
            }]);
        
        const offers = arbitration.getTableRowsScoped("offers")[arbitration.accountName];
        expect(offers.find(offer => offer.offer_id === "0")).toEqual({
            offer_id: "0",
            case_id: "0",
            status: 1,
            estimated_hours: 10,
            arbitrator: "user3",
            hourly_rate: "10.0000 USD",
        });

        const casefiles = arbitration.getTableRowsScoped("casefiles")[arbitration.accountName];
        expect(casefiles.find(cf => cf.case_id === "0").number_offers).toEqual(2);  
    });

    it("update offer", async () => {
         await arbitration.loadFixtures("offers", {
            arbitration: [{
                offer_id: 0,
                case_id: "0",
                status: "1",
                estimated_hours: 10,
                arbitrator: "user3",
                hourly_rate: "10.0000 USD",
            }]
         })
        
        await arbitration.contract.makeoffer({
            case_id: "0",
            offer_id: 0,
            arbitrator: "user3",
            hourly_rate: "8.0000 USD",
            estimated_hours: 15,
        },
            [{
                actor: user3.accountName,
                permission: "active"
            }]);
        
        const offers = arbitration.getTableRowsScoped("offers")[arbitration.accountName];
        expect(offers.find(offer => offer.offer_id === "0")).toEqual({
            offer_id: "0",
            case_id: "0",
            status: 1,
            estimated_hours: 15,
            arbitrator: "user3",
            hourly_rate: "8.0000 USD",
        });

        const casefiles = arbitration.getTableRowsScoped("casefiles")[arbitration.accountName];
        expect(casefiles.find(cf => cf.case_id === "0").number_offers).toEqual(1);  
    });

    it("fails if actor is not an arbitrator", async () => {
         await expect(arbitration.contract.makeoffer({
            case_id: "0",
            offer_id: -1,
            arbitrator: "user2",
            hourly_rate: "10.0000 USD",
            estimated_hours: 10,
        },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow("actor is not a registered Arbitrator");
    });

    it("fails if arbitrator has been removed", async () => {
        await arbitration.loadFixtures("arbitrators", {
            "arbitration": [
                {   
                    arb: "admin",
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
        
        await expect(arbitration.contract.makeoffer({
            case_id: "0",
            offer_id: -1,
            arbitrator: "admin",
            hourly_rate: "10.0000 USD",
            estimated_hours: 10,
        },
            [{
              actor: admin.accountName,
              permission: "active"
            }])).rejects.toThrow("Arbitrator has been removed.");
    });

    it("fails if arbitrator term has expired", async () => {
        await arbitration.loadFixtures("arbitrators", {
            "arbitration": [
                {   
                    arb: "admin",
                    arb_status: 1,
                    open_case_ids: [],
                    closed_case_ids: [],
                    recused_case_ids: [],
                    credentials_link: "link",
                    elected_time: '1999-01-01T00:00:00.000',
                    term_expiration:'1999-12-31T00:00:00.000',
                    languages: [0,1]
                }
            ]
          })
        
        await expect(arbitration.contract.makeoffer({
            case_id: "0",
            offer_id: -1,
            arbitrator: "admin",
            hourly_rate: "10.0000 USD",
            estimated_hours: 10,
        },
            [{
              actor: admin.accountName,
              permission: "active"
            }])).rejects.toThrow("Arbitrator term has expired");
    });

    it("fails if arbitrator is not available", async () => {
        await arbitration.loadFixtures("arbitrators", {
            "arbitration": [
                {   
                    arb: "admin",
                    arb_status: 2,
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
        
        await expect(arbitration.contract.makeoffer({
            case_id: "0",
            offer_id: -1,
            arbitrator: "admin",
            hourly_rate: "10.0000 USD",
            estimated_hours: 10,
        },
            [{
              actor: admin.accountName,
              permission: "active"
            }])).rejects.toThrow("Arb status isn't set to available, Arbitrator is unable to receive new cases");
    });

    it("fails if case not found", async () => {
        await expect(arbitration.contract.makeoffer({
            case_id: "1",
            offer_id: -1,
            arbitrator: "user3",
            hourly_rate: "10.0000 USD",
            estimated_hours: 10,
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("Case not found");
    });

    it("fails if case is not in awaiting arbs status", async () => {
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
                fee_paid_tlos: "0.0000 TLOS",
                arbitrator_cost_tlos: "0.0000 TLOS",
                sending_offers_until_ts: "2000-01-10T00:00:00.000"
            }]
        })

        await expect(arbitration.contract.makeoffer({
            case_id: "1",
            offer_id: -1,
            arbitrator: "user3",
            hourly_rate: "10.0000 USD",
            estimated_hours: 10,
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("Case needs to be in AWAITING ARBS status");
    });

    it("fails if time for sending offers is over", async () => {
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
                arbitrator_cost_tlos: "0.0000 TLOS",
                sending_offers_until_ts: "1999-01-10T00:00:00.000"
            }]
        })

        await expect(arbitration.contract.makeoffer({
            case_id: "1",
            offer_id: -1,
            arbitrator: "user3",
            hourly_rate: "10.0000 USD",
            estimated_hours: 10,
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("Time for sending offers is over");
    });

    it("fails if offer is not in USD", async () => {
        await expect(arbitration.contract.makeoffer({
            case_id: "0",
            offer_id: -1,
            arbitrator: "user3",
            hourly_rate: "10.0000 TLOS",
            estimated_hours: 10,
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("Offer needs to be set in USD");

        
    });

    it("fails if estimated hours is zero", async () => {
        await expect(arbitration.contract.makeoffer({
            case_id: "0",
            offer_id: -1,
            arbitrator: "user3",
            hourly_rate: "10.0000 USD",
            estimated_hours: 0,
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("The minimum estimated hours is 1");
    });

    it("fails if arbitrator has already made an offer", async () => {
        await arbitration.loadFixtures("offers", {
            arbitration: [{
                offer_id: 0,
                case_id: "0",
                status: "1",
                estimated_hours: 10,
                arbitrator: "user3",
                hourly_rate: "10.0000 USD",
            }]
        })

        await expect(arbitration.contract.makeoffer({
            case_id: "0",
            offer_id: -1,
            arbitrator: "user3",
            hourly_rate: "10.0000 USD",
            estimated_hours: 5,
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("Arbitrator already has made an offer");
    });

    it("offer not found", async () => {
        await expect(arbitration.contract.makeoffer({
            case_id: "0",
            offer_id: 5,
            arbitrator: "user3",
            hourly_rate: "10.0000 USD",
            estimated_hours: 5,
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("Offer not found");
    });

    it("offer does not belong to the case ID", async () => {
        await arbitration.loadFixtures("offers", {
            arbitration: [{
                offer_id: 0,
                case_id: "1",
                status: "1",
                estimated_hours: 10,
                arbitrator: "user3",
                hourly_rate: "10.0000 USD",
            }]
        })

        await expect(arbitration.contract.makeoffer({
            case_id: "0",
            offer_id: "0",
            arbitrator: "user3",
            hourly_rate: "10.0000 USD",
            estimated_hours: 5,
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("Offer does not belong to the case ID");
    });

    it("arbitrator is not the one who made the offer", async () => {
         await arbitration.loadFixtures("offers", {
            arbitration: [{
                offer_id: 0,
                case_id: "0",
                status: "1",
                estimated_hours: 10,
                arbitrator: "admin",
                hourly_rate: "10.0000 USD",
            }]
        })

        await expect(arbitration.contract.makeoffer({
            case_id: "0",
            offer_id: "0",
            arbitrator: "user3",
            hourly_rate: "10.0000 USD",
            estimated_hours: 5,
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("Arbitrator is not the one who made the offer");
    });

    it("Offer needs to be pending to be updated", async () => {
        await arbitration.loadFixtures("offers", {
            arbitration: [{
                offer_id: 0,
                case_id: "0",
                status: "2",
                estimated_hours: 10,
                arbitrator: "user3",
                hourly_rate: "10.0000 USD",
            }]
        })

        await expect(arbitration.contract.makeoffer({
            case_id: "0",
            offer_id: "0",
            arbitrator: "user3",
            hourly_rate: "10.0000 USD",
            estimated_hours: 5,
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("Offer needs to be pending to be updated");
    });

    it("fails if user other than arbitrator tries to make an offer", async () => {
        await expect(arbitration.contract.makeoffer({
            case_id: "0",
            offer_id: -1,
            arbitrator: "user3",
            hourly_rate: "10.0000 USD",
            estimated_hours: 10,
        },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow();
    });
    
});