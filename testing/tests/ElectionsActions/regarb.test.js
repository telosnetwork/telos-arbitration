const { loadConfig, Blockchain } = require("@klevoya/hydra");

const config = loadConfig("hydra.yml");

describe("Register Arbitrator Telos Arbitration Smart Contract Tests", () => {
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

    it("registers a new nominee", async () => {
        expect.assertions(1);

        await arbitration.contract.regarb({ nominee: "user1", credentials_link: "QmTtDqWzo179ujTXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd" },
            [{
                actor: user1.accountName,
                permission: "active"
            }]);
        
        const nominees = arbitration.getTableRowsScoped("nominees")["arbitration"];
        expect(nominees.find(nom => nom.nominee_name === "user1")).toEqual({
            application_time: "2000-01-01T00:00:00.000",
            credentials_link: "QmTtDqWzo179ujTXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd",
            nominee_name: "user1"
        });
    });

    it("registers that his arb term has expired", async () => {
        expect.assertions(1);

        await arbitration.loadFixtures("arbitrators", {
            "arbitration": [
                {   
                    arb: "user1",
                    arb_status: 0,
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

        await arbitration.contract.regarb({ nominee: "user1", credentials_link: "QmTtDqWzo179ujTXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd" },
            [{
                actor: user1.accountName,
                permission: "active"
            }]);
        
        const nominees = arbitration.getTableRowsScoped("nominees")["arbitration"];
        expect(nominees.find(nom => nom.nominee_name === "user1")).toEqual({
                    application_time: "2000-01-01T00:00:00.000",
                    credentials_link: "QmTtDqWzo179ujTXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd",
                    nominee_name: "user1"
        });
    });

    it("registers that has been removed", async () => {
        expect.assertions(1);

        await arbitration.loadFixtures("arbitrators", {
            "arbitration": [
                {   
                    arb: "user1",
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

        await arbitration.contract.regarb({ nominee: "user1", credentials_link: "QmTtDqWzo179ujTXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd" },
            [{
                actor: user1.accountName,
                permission: "active"
            }]);
        
        const nominees = arbitration.getTableRowsScoped("nominees")["arbitration"];
        expect(nominees.find(nom => nom.nominee_name === "user1")).toEqual({
                    application_time: "2000-01-01T00:00:00.000",
                    credentials_link: "QmTtDqWzo179ujTXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd",
                    nominee_name: "user1"
        });
    });

    it("registers that his seat has been expired", async () => {
        expect.assertions(1);

        await arbitration.loadFixtures("arbitrators", {
            "arbitration": [
                {   
                    arb: "user1",
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

        await arbitration.contract.regarb({ nominee: "user1", credentials_link: "QmTtDqWzo179ujTXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd" },
            [{
                actor: user1.accountName,
                permission: "active"
            }]);
        
        const nominees = arbitration.getTableRowsScoped("nominees")["arbitration"];
        expect(nominees.find(nom => nom.nominee_name === "user1")).toEqual({
                    application_time: "2000-01-01T00:00:00.000",
                    credentials_link: "QmTtDqWzo179ujTXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd",
                    nominee_name: "user1"
        });
    });

    it("fails if nominee is not an account", async () => {
        await expect(arbitration.contract.regarb({ nominee: "user", credentials_link: "QmTtDqWzo179ujTXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd" },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow("New candidate is not an account");
    });

    it("fails if nominee is already registered", async () => {
        await arbitration.loadFixtures("nominees", {
            "arbitration": [
                {
                    nominee_name: "user1",
                    credentials_link: "link",
                    application_time: '2000-01-01T00:00:00.000',
                }
            ]
        })
        await expect(arbitration.contract.regarb({ nominee: "user1", credentials_link: "QmTtDqWzo179ujTXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd" },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow("Nominee is already an applicant");
    });


    it("fails if nominee is already an arbitrator and his period hasn't ended", async () => {
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
                    languages: [0,1]
                }
            ]
        })
        
        await expect(arbitration.contract.regarb({ nominee: "user1", credentials_link: "QmTtDqWzo179ujTXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd" },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow("Nominee is already an Arbitrator and the seat hasn't expired");
    });

    it("fails to register an applicant if user different than him tries", async () => {
        await expect(arbitration.contract.regarb({ nominee: "user1", credentials_link: "QmTtDqWzo179ujTXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd" },
            [{
              actor: admin.accountName,
              permission: "active"
            }])).rejects.toThrow();
    })

});