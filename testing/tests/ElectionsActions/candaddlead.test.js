const { loadConfig, Blockchain } = require("@klevoya/hydra");

const config = loadConfig("hydra.yml");

describe("Add nominee as a candidate Telos Arbitration Smart Contract Tests", () => {
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
                }]
        });
    });

    it("registers a new nominee as a candidate", async () => {
        expect.assertions(1);

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

        await arbitration.contract.candaddlead({ nominee: "user1" },
        [{
            actor: user1.accountName,
            permission: "active"
        }])

        const elections = arbitration.getTableRowsScoped("elections")["arbitration"];
        expect(elections.find(elec => elec.election_id === '0')).toEqual({  
          election_id: '0',
          ballot_name: '',
          info_url: 'info',
          candidates: [{ name: "user1", votes: "0.0000 VOTE" }],
          available_seats: 5,
          begin_add_candidates_ts: '2000-01-01T00:16:10.000',
          end_add_candidates_ts: '2000-01-08T00:16:10.000',
          begin_voting_ts: '1970-01-01T00:00:00.000',
          end_voting_ts: '1970-01-01T00:00:00.000',
          status: 1
        })
    });

    it("fails if election doesn't exist", async () => {
        
        await expect(arbitration.contract.candaddlead({ nominee: "user1" },
            [{
                actor: user1.accountName,
                permission: "active"
            }])).rejects.toThrow("Election doesn't exist");
    });

    it("fails if election is not in created status", async () => {
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
                    status: 2
                }
            ]
         })
        
         await expect(arbitration.contract.candaddlead({ nominee: "user1" },
            [{
                actor: user1.accountName,
                permission: "active"
            }])).rejects.toThrow("Election needs to be in CREATED status to add a new candidate");
    });

    it("fails if time for removing candidates has ended", async () => {
         await arbitration.loadFixtures("elections", {
            "arbitration": [
                {
                    election_id: 0,
                    ballot_name: "",
                    
                    info_url: "info",
                    candidates: [],
                    available_seats: 5,
                    begin_add_candidates_ts: "1999-12-01T00:16:10",
                    end_add_candidates_ts: "1999-12-31T00:16:10",
                    begin_voting_ts: "1970-01-01T00:00:00",
                    end_voting_ts: "1970-01-01T00:00:00",
                    status: 1
                }
            ]
         })
        
         await expect(arbitration.contract.candaddlead({ nominee: "user1" },
            [{
                actor: user1.accountName,
                permission: "active"
            }])).rejects.toThrow("Cannot add candidates if period has ended");
    });

    it("fails if candidate is not in the nominee list", async () => {
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
        
        await expect(arbitration.contract.candaddlead({ nominee: "user2" },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow("Nominee isn't an applicant. Use regarb action to register as a nominee");
        
    });


    it("fails if candidate is already added", async () => {
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
        
          await expect(arbitration.contract.candaddlead({ nominee: "user1" },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow("Candidate already added");
    });


    it("fails to add a candidate if user different than nominee tries", async () => {
        await expect(arbitration.contract.candaddlead({ nominee: "user1" },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow();
    })

});