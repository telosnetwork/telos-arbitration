const { loadConfig, Blockchain } = require("@klevoya/hydra");

const config = loadConfig("hydra.yml");

describe("Ready Case Telos Arbitration Smart Contract Tests", () => {
    let blockchain = new Blockchain(config);
    let arbitration = blockchain.createAccount("arbitration");
    let admin = blockchain.createAccount("admin");
    let user1 = blockchain.createAccount("user1");
    let user2 = blockchain.createAccount("user2");
    let user3 = blockchain.createAccount("user3");
    let delphioracle = blockchain.createAccount("delphioracle");
    let eosiotoken = blockchain.createAccount("eosio.token");


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

        eosiotoken.setContract(blockchain.contractTemplates["eosio.token"]);
        eosiotoken.updateAuth(`active`, `owner`, {
            accounts: [
                {
                permission: {
                    actor: eosiotoken.accountName,
                    permission: `eosio.code`
                },
                weight: 1
                }
            ]
        });

    });

    beforeEach(async () => {
        arbitration.resetTables(); 
        eosiotoken.resetTables();
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
                    claim_info_required: '',
                    respondant_limit_time: '2000-01-01T00:00:00.000',
                    response_info_needed: false,
                    response_info_required: '',
                }
            ]
        })
        
        await eosiotoken.loadFixtures();

    });

    it("ready case", async () => {
        expect.assertions(3);

        await arbitration.loadFixtures("accounts", {
            user1: [{balance: "50.0000 TLOS"}]
        })

        await arbitration.contract.readycase({
            case_id: "0",
            claimant: "user1",
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])
        
        const casefiles = arbitration.getTableRowsScoped("casefiles")[arbitration.accountName];
        expect(casefiles.find(cf => cf.case_id === "0")).toEqual({
            case_id: '0',
            case_status: 1,
            claimant: 'user1',
            respondant: 'user2',
            arbitrators: [],
            approvals: [],
            fee_paid_tlos: "15.3846 TLOS",
            arbitrator_cost_tlos: "0.0000 TLOS",
            number_claims: 1,
            number_offers: 1,
            required_langs: [ 0, 1, 2 ],
            case_ruling: '',
            update_ts: '2000-01-01T00:00:00.000',
            sending_offers_until_ts: "2000-01-08T00:00:00.000"
        });
        
        const accounts = arbitration.getTableRowsScoped("accounts")[user1.accountName];
        expect(accounts[0].balance).toEqual("34.6154 TLOS");

        const config = arbitration.getTableRowsScoped("config")[arbitration.accountName][0];
        expect(config.reserved_funds).toEqual("15.3846 TLOS");
    });

    it("fails if case not found", async () => {
        await arbitration.loadFixtures("accounts", {
            user1: [{balance: "50.0000 TLOS"}]
        })

        await expect(arbitration.contract.readycase({
            case_id: "1",
            claimant: "user1",
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow("Case Not Found");
    })

    it("fails if claimant doesn't have enough funds", async () => {
          await arbitration.loadFixtures("accounts", {
            user1: [{balance: "5.0000 TLOS"}]
        })

        await expect(arbitration.contract.readycase({
            case_id: "0",
            claimant: "user1",
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow("overdrawn balance");
    });

    it("fails if case is not in setup mode", async () => {
        await arbitration.loadFixtures("accounts", {
            user1: [{balance: "50.0000 TLOS"}]
        })
        
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

         await expect(arbitration.contract.readycase({
            case_id: "1",
            claimant: "user1",
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow("Cases can only be readied during CASE_SETUP");
    })

    it("fails if claimant doesn't belong to the case", async () => {
        await arbitration.loadFixtures("accounts", {
        user1: [{ balance: "50.0000 TLOS"}]
        })
        
        await expect(arbitration.contract.readycase({
            case_id: "0",
            claimant: "user2",
        },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow("you are not the claimant of this case.");
    })

    it("fails if case doesn't have any claim", async () => {
        await arbitration.loadFixtures("accounts", {
        user1: [{balance: "50.0000 TLOS"}]
        })
        
        await arbitration.loadFixtures("casefiles", {
            arbitration: [{
                case_id: '1',
                case_status: 0,
                claimant: 'user1',
                respondant: 'user2',
                arbitrators: [],
                approvals: [],
                number_claims: 0,
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

        await expect(arbitration.contract.readycase({
            case_id: "1",
            claimant: "user1",
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow("Cases must have at least one claim");
    })



    it("fails if user other than claimant tries", async () => {
        await arbitration.loadFixtures("accounts", {
            user1: [{balance: "50.0000 TLOS"}]
        })

        await expect(arbitration.contract.readycase({
            case_id: "0",
            claimant: "user1",
        },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow();
    })

    
});