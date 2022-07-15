const { loadConfig, Blockchain } = require("@klevoya/hydra");

const config = loadConfig("hydra.yml");

describe("Set Version Telos Arbitration Smart Contract Tests", () => {
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

    it("change current version", async () => {
        expect.assertions(1);


        await arbitration.contract.setversion({ new_version: "0.2.0" },
          [{
            actor: admin.accountName,
            permission: "active"
          }]);

        const config = arbitration.getTableRowsScoped("config")[arbitration.accountName][0];
        expect(config.contract_version).toEqual("0.2.0");
    })

    it("fails to set new version if user different than admin tries", async () => {
        await expect(arbitration.contract.setversion({ new_version: "0.2.0" },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow();
    })

});