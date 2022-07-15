const { loadConfig, Blockchain } = require("@klevoya/hydra");

const config = loadConfig("hydra.yml");

describe("SetAdmin Telos Arbitration Smart Contract Tests", () => {
    let blockchain = new Blockchain(config);
    let arbitration = blockchain.createAccount("arbitration");
    let admin = blockchain.createAccount("admin");
    let user1 = blockchain.createAccount("user1");

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

    });

    it("change current admin", async () => {
        expect.assertions(1);

        blockchain.createAccount("newadmin");

        await arbitration.contract.setadmin({ new_admin: "newadmin" },
          [{
            actor: admin.accountName,
            permission: "active"
          }]);

        const config = arbitration.getTableRowsScoped("config")[arbitration.accountName][0];
        expect(config.admin).toEqual("newadmin");
      })

      it("fails to set new admin if user different than admin tries", async () => {

          await expect(arbitration.contract.setadmin({ new_admin: "user1" },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow();
      })

      it("do not change current admin if new admin do not exists", async () => {

        await expect(arbitration.contract.setadmin({ new_admin: "fakeadmin" },
          [{
            actor: admin.accountName,
            permission: "active"
          }]
        )).rejects.toThrow("new admin account doesn't exist");
      })
});