const { loadConfig, Blockchain } = require("@klevoya/hydra");

const config = loadConfig("hydra.yml");

describe("Withdraw Telos Arbitration Smart Contract Tests", () => {
    let blockchain = new Blockchain(config);
    let arbitration = blockchain.createAccount("arbitration");
    let admin = blockchain.createAccount("admin");
    let user1 = blockchain.createAccount("user1");
    let user2 = blockchain.createAccount("user2");
    let user3 = blockchain.createAccount("user3");
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
        
        await arbitration.loadFixtures("accounts", {
            user1: [
                {
                    balance: "15.0000 TLOS",
                }
            ]
        })

        await eosiotoken.loadFixtures();
    });

    it("withdraws balance", async () => {
        expect.assertions(2);

        await arbitration.contract.withdraw({ owner: "user1" },
            [{
                actor: user1.accountName,
                permission: "active"
            }])
        
        
        const accounts = arbitration.getTableRowsScoped("accounts")["arbitration"];
        expect(accounts).toBeUndefined();

        const balance = eosiotoken.getTableRowsScoped("accounts")["user1"][0];
        expect(balance).toEqual({ balance: "65.0000 TLOS" });
    });
    
    it("fails to withdraw if user has not balance", async () => {
         await expect(arbitration.contract.withdraw({ owner: "user2" },
            [{
                actor: user2.accountName,
                permission: "active"
            }])).rejects.toThrow();
    })

    it("fails to withdraw if user other than owner tries", async () => {
        await expect(arbitration.contract.withdraw({ owner: "user1" },
            [{
                actor: user2.accountName,
                permission: "active"
            }])).rejects.toThrow();
    });
});
    