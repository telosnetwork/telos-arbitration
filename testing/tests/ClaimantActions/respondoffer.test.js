const { loadConfig, Blockchain } = require("@klevoya/hydra");

const config = loadConfig("hydra.yml");

describe("Respond Offer Telos Arbitration Smart Contract Tests", () => {
    let blockchain = new Blockchain(config);
    let arbitration = blockchain.createAccount("arbitration");
    let admin = blockchain.createAccount("admin");
    let user1 = blockchain.createAccount("user1");
    let user2 = blockchain.createAccount("user2");
    let user3 = blockchain.createAccount("user3");

    let delphioracle = blockchain.createAccount("delphioracle");

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

        delphioracle.setContract(blockchain.contractTemplates["delphioracle"]);

        await delphioracle.loadFixtures();


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
                    "runoff_election_voting_ts": 604800,
                    "election_add_candidates_ts": 604800,
                    "arb_term_length": 31536000,
                    "current_election_id": "0",
                    "available_funds": "5.0000 TLOS",
                    "reserved_funds": "50.0000 TLOS",
                    "max_claims_per_case": 10,
                    "fee_usd": "10.0000 USD",
                    "claimant_accepting_offers_ts": 604800,

                }
            ]
        })


        await arbitration.loadFixtures("casefiles", {
            arbitration: [{
                case_id: '0',
                case_status: 1,
                claimant: 'user1',
                respondant: 'user2',
                arbitrators: [],
                approvals: [],
                number_claims: 1,
                number_offers: 2,
                required_langs: [ 0, 1, 2 ],
                case_ruling: '',
                recusal: "",
                update_ts: '2000-01-01T00:00:00.000',
                fee_paid_tlos: "50.0000 TLOS",
                arbitrator_cost_tlos: "0.0000 TLOS",
                sending_offers_until_ts: "2000-01-10T00:00:00.000"
            }]
            
        })

        await arbitration.loadFixtures("offers", {
            arbitration: [
                {
                    offer_id: 0,
                    case_id: "0",
                    status: "1",
                    estimated_hours: 10,
                    arbitrator: "user3",
                    hourly_rate: "10.0000 USD",
                },
                {
                    offer_id: 1,
                    case_id: "0",
                    status: "1",
                    estimated_hours: 10,
                    arbitrator: "user2",
                    hourly_rate: "10.0000 USD",
                },

            ]
        })

    });

    it("accept offer", async () => {
        expect.assertions(6);

        await arbitration.loadFixtures("accounts", {
            user1: [{balance: "1000.0000 TLOS"}]
        })
        
        await arbitration.contract.respondoffer({
            case_id: "0",
            offer_id: 0,
            accept: true
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])
        
        const offers = arbitration.getTableRowsScoped("offers")[arbitration.accountName];

        expect(offers.find(offer => offer.offer_id === "0").status).toEqual(2);
        expect(offers.find(offer => offer.offer_id === "1").status).toEqual(3);

               
        const casefiles = arbitration.getTableRowsScoped("casefiles")[arbitration.accountName];
        expect(casefiles.find(cf => cf.case_id === "0").arbitrators).toEqual(["user3"]);  
        expect(casefiles.find(cf => cf.case_id === "0").case_status).toEqual(2); 

        const accounts = arbitration.getTableRowsScoped("accounts")[user1.accountName];
        expect(accounts[0].balance).toEqual("846.1539 TLOS");

        const config = arbitration.getTableRowsScoped("config")[arbitration.accountName][0];
        expect(config.reserved_funds).toEqual("203.8461 TLOS");
    });

    it("rejects offer", async () => {
        await arbitration.contract.respondoffer({
            case_id: "0",
            offer_id: 0,
            accept: false
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])
        
        const offers = arbitration.getTableRowsScoped("offers")[arbitration.accountName];
        expect(offers.find(offer => offer.offer_id === "0")).toEqual({
            offer_id: "0",
            case_id: "0",
            status: 3,
            estimated_hours: 10,
            arbitrator: "user3",
            hourly_rate: "10.0000 USD",
        });
    });

    it("fails if case not found", async () => {
        await expect(arbitration.contract.respondoffer({
            case_id: "5",
            offer_id: 0,
            accept: false
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow("Case not found");
    });

    it("fails if case is not in awaiting arbs status", async () => {

    });

    it("fails if offer not found", async () => {
        await expect(arbitration.contract.respondoffer({
            case_id: "0",
            offer_id: 5,
            accept: false
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow("Offer not found");
    });

    it("fails if user is not claimant", async () => {
        await expect(arbitration.contract.respondoffer({
            case_id: "0",
            offer_id: 0,
            accept: false
        },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow();
    });

    it("fails if offer does not belong to the case", async () => {
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
                sending_offers_until_ts: "2000-01-10T00:00:00.000"
            }]
            
        })

    
        await expect(arbitration.contract.respondoffer({
            case_id: "1",
            offer_id: 0,
            accept: false
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow("Offer does not belong to the case ID");
    });

    it("fails if offer is not in pending status", async () => {
        await arbitration.loadFixtures("offers", {
            arbitration: [{
                offer_id: 5,
                case_id: "0",
                status: "2",
                estimated_hours: 10,
                arbitrator: "user3",
                hourly_rate: "10.0000 USD",
            }]
        })

        await expect(arbitration.contract.respondoffer({
            case_id: "0",
            offer_id: 5,
            accept: false
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow("Offer needs to be pending to be responded");
    });
    
});