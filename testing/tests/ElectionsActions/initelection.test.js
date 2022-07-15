const { loadConfig, Blockchain } = require("@klevoya/hydra");

const config = loadConfig("hydra.yml");

describe("Init election Telos Arbitration Smart Contract Tests", () => {
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

    it("inits a new election", async () => {
        expect.assertions(1);

        await arbitration.contract.initelection({ content: "ipfs content" },
            [{
                actor: admin.accountName,
                permission: "active"
            }]);
        
        const elections = arbitration.getTableRowsScoped("elections")["arbitration"];
        expect(elections.find(elec => elec.election_id === '0')).toEqual({  
          election_id: '0',
          ballot_name: '',
          info_url: 'ipfs content',
          candidates: [],
          available_seats: 2,
          begin_add_candidates_ts: '2000-01-01T00:00:00.000',
          end_add_candidates_ts: '2000-01-08T00:00:00.000',
          begin_voting_ts: '1970-01-01T00:00:00.000',
          end_voting_ts: '1970-01-01T00:00:00.000',
          status: 1
        })
    });

    it("fails if no seats are available", async () => {

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
                },
                {   
                    arb: "user2",
                    arb_status: 1,
                    open_case_ids: [],
                    closed_case_ids: [],
                    recused_case_ids: [],
                    credentials_link: "link",
                    elected_time: '1999-01-01T00:00:00.000',
                    term_expiration: '2000-12-01T00:00:00.000',
                    languages: [0,1]
                }
            ]
        })

        await expect(arbitration.contract.initelection({ content: "ipfs content" },
            [{
              actor: admin.accountName,
              permission: "active"
            }])).rejects.toThrow("No seats are available!");
    });

    it("fails if there is an election already created and not ended", async () => {
        await arbitration.loadFixtures("elections", {
            "arbitration": [
                {
                    election_id: 0,
                    ballot_name: "",     
                    info_url: "info",
                    candidates: [],
                    available_seats: 5,
                    begin_add_candidates_ts: "2000-01-01T00:16:10",
                    end_add_candidates_ts: "2000-01-08T00:16:10",
                    begin_voting_ts: "1970-01-01T00:00:00",
                    end_voting_ts: "1970-01-01T00:00:00",
                    status: 1
                }
            ]
        })
        await expect(arbitration.contract.initelection({ content: "ipfs content" },
            [{
              actor: admin.accountName,
              permission: "active"
            }])).rejects.toThrow("There is an election already running");
    });

    it("fails to init election if user different than admin tries", async () => {
        await expect(arbitration.contract.initelection({ content: "ipfs content" },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow();
    })

});