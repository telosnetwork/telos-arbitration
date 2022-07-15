const { loadConfig, Blockchain } = require("@klevoya/hydra");

const config = loadConfig("hydra.yml");

describe("Set Lang Codes Status Telos Arbitration Smart Contract Tests", () => {
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
            "arbitration": [
                {   
                    arb: "user1",
                    arb_status: 1,
                    open_case_ids: [],
                    closed_case_ids: [],
                    recused_case_ids: [],
                    credentials_link: "link",
                    elected_time: '1999-01-01T00:00:00.000',
                    term_expiration:'2001-01-01T00:00:00.000',
                    languages: []
                }
            ]
        })
    });

    it("set lang codes", async () => {
        expect.assertions(1);

        await arbitration.contract.setlangcodes({lang_codes: [0,1], arbitrator: "user1"},
          [{
            actor: user1.accountName,
            permission: "active"
          }]);

        const arbitrators = arbitration.getTableRowsScoped("arbitrators")[arbitration.accountName];
        expect(arbitrators.find(arb => arb.arb === "user1").languages).toEqual([0,1]);  
    })
    
    it("fails if arbitrator not found", async () => {
         await expect(arbitration.contract.setlangcodes({lang_codes: [0,1], arbitrator: "user3"},
            [{
              actor: user3.accountName,
              permission: "active"
            }])).rejects.toThrow(); 
    })


    it("fails if arb term has expired", async () => {
         await arbitration.loadFixtures("arbitrators", {
            "arbitration": [
                {   
                    arb: "user2",
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
        
        await expect(arbitration.contract.setlangcodes({lang_codes: [0,1], arbitrator: "user2"},
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow("Arbitrator term expired"); 
    })

     it("fails if arbitrator status is invalid", async () => {
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
                    term_expiration:'2000-12-31T00:00:00.000',
                    languages: [0,1]
                }
            ]
         })
        
        await expect(arbitration.contract.setlangcodes({lang_codes: [0,1], arbitrator: "user2"},
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow("Arbitrator must be active"); 
    })

    it("fails to set arbitrator status if user different than arbitrator tries", async () => {

          await expect(arbitration.contract.setlangcodes({lang_codes: [0,1], arbitrator: "user1"},
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow();
      })
});