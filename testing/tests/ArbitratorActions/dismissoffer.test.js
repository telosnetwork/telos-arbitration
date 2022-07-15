const { loadConfig, Blockchain } = require("@klevoya/hydra");

const config = loadConfig("hydra.yml");

describe("Dissmis Offer Telos Arbitration Smart Contract Tests", () => {
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
                arbitrator_cost_tlos: "150.0000 TLOS",
                sending_offers_until_ts: "2000-01-10T00:00:00.000"
            }]
            
        })

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

    });

    it("dissmis offer", async () => {
        await arbitration.contract.dismissoffer({
            case_id: "0",
            offer_id: "0",
        },
            [{
                actor: user3.accountName,
                permission: "active"
            }]);
        
        const offers = arbitration.getTableRowsScoped("offers")[arbitration.accountName];
        expect(offers.length).toEqual(1);
        expect(offers.find(offer => offer.offer_id === "0").status).toEqual(4);

        const casefiles = arbitration.getTableRowsScoped("casefiles")[arbitration.accountName];
        expect(casefiles.find(cf => cf.case_id === "0").number_offers).toEqual(1);  


    });

    it("fails if case not found", async () => {
        await expect(arbitration.contract.dismissoffer({
            case_id: "45",
            offer_id: "0",
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
                arbitrator_cost_tlos: "150.0000 TLOS",
                sending_offers_until_ts: "2000-01-10T00:00:00.000"
            }]
        })

        await expect(arbitration.contract.dismissoffer({
            case_id: "1",
            offer_id: "5",
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("Case needs to be in AWAITING ARBS status");
    });

    it("fails if offer not found", async () => {
        await expect(arbitration.contract.dismissoffer({
            case_id: "0",
            offer_id: "5",
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("Offer not found");
    });

    it("fails if user is not the arbitrator of the offer", async () => {
        await expect(arbitration.contract.dismissoffer({
            case_id: "0",
            offer_id: "0",
        },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow();
    });

    it("fails if offer does not belong to the case", async () => {
        await arbitration.loadFixtures("offers", {
            arbitration: [{
                offer_id: 1,
                case_id: "1",
                status: "1",
                estimated_hours: 10,
                arbitrator: "user3",
                hourly_rate: "10.0000 USD",
            }]
        })

        await expect(arbitration.contract.dismissoffer({
            case_id: "0",
            offer_id: "1",
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("Offer does not belong to the case ID");
    });

    it("fails if offer is not in pending status", async () => {
        await arbitration.loadFixtures("offers", {
            arbitration: [{
                offer_id: 1,
                case_id: "0",
                status: "2",
                estimated_hours: 10,
                arbitrator: "user3",
                hourly_rate: "10.0000 USD",
            }]
        })

        await expect(arbitration.contract.dismissoffer({
            case_id: "0",
            offer_id: "1",
        },
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow("Offer needs to be pending to be updated");
    });
    
});