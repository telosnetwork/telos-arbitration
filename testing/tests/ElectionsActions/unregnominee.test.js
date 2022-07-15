const { loadConfig, Blockchain } = require("@klevoya/hydra");

const config = loadConfig("hydra.yml");

describe("Register Arbitrator Telos Arbitration Smart Contract Tests", () => {
    let blockchain = new Blockchain(config);
    let arbitration = blockchain.createAccount("arbitration");
    let admin = blockchain.createAccount("admin");
    let user1 = blockchain.createAccount("user1");
    let user2 = blockchain.createAccount("user2");


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
        await arbitration.loadFixtures("nominees", {
            "arbitration": [
                {
                    nominee_name: "user1",
                    credentials_link: "link",
                    application_time: '2000-01-01T00:00:00.000',
                },
                {
                    nominee_name: "user3",
                    credentials_link: "link",
                    application_time: '2000-01-01T00:00:00.000',
                }
            ]
        })
    });

    it("unregisters a nominee", async () => {
        expect.assertions(1);

        
        await arbitration.contract.unregnominee({ nominee: "user1" },
            [{
                actor: user1.accountName,
                permission: "active"
            }]);
        
        const nominees = arbitration.getTableRowsScoped("nominees")["arbitration"];
        expect(nominees.find(nom => nom.nominee_name === "user1")).toBeUndefined();
    });

    it("fails if nominee is not registered", async () => {
        await expect(arbitration.contract.unregnominee({ nominee: "user2" },
        [{
            actor: user2.accountName,
            permission: "active"
        }])).rejects.toThrow("Nominee isn't an applicant");

    });

    it("fails if nominee is a candidate", async () => {
         await arbitration.loadFixtures("elections", {
            "arbitration": [
                {
                    election_id: 0,
                    ballot_name: "",
                    
                    info_url: "info",
                    candidates: [{ name: "user1", votes: "0.0000 VOTE" }],
                    available_seats: 5,
                    begin_add_candidates_ts: "2000-01-01T00:16:10",
                    end_add_candidates_ts: "2000-01-08T00:16:10",
                    begin_voting_ts: "1970-01-01T00:00:00",
                    end_voting_ts: "1970-01-01T00:00:00",
                    status: 1
                }
            ]
         })
        
        await expect(arbitration.contract.unregnominee({ nominee: "user1" },
        [{
            actor: user1.accountName,
            permission: "active"
        }])).rejects.toThrow("Cannot remove a nominee if is a candidate");
    });

    it("fails to unregister a nominee if user different than him tries", async () => {
        await expect(arbitration.contract.unregnominee({ nominee: "user1" },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow();
    })

});