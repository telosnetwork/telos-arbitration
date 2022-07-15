const { loadConfig, Blockchain } = require("@klevoya/hydra");

const config = loadConfig("hydra.yml");

describe("Begin Voting Telos Arbitration Smart Contract Tests", () => {
    let blockchain = new Blockchain(config);
    let arbitration = blockchain.createAccount("arbitration");
    let admin = blockchain.createAccount("admin");
    let user1 = blockchain.createAccount("user1");
    let user2 = blockchain.createAccount("user2");
    let decide = blockchain.createAccount("telos.decide");
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


        decide.setContract(blockchain.contractTemplates["telos.decide"]);
        eosiotoken.setContract(blockchain.contractTemplates["eosio.token"]);
    });

    beforeEach(async () => {
        arbitration.resetTables();
        decide.resetTables();
        eosiotoken.resetTables();

        await eosiotoken.loadFixtures();

        await decide.loadFixtures();

        await arbitration.loadFixtures("nominees", {
            "arbitration": [
                {
                    nominee_name: "user1",
                    credentials_link: "link",
                    application_time: '1999-11-05T00:00:00.000',
                },
                {
                    nominee_name: "user2",
                    credentials_link: "link",
                    application_time: '1999-11-04T00:00:00.000',
                }
            ]
        });


    });

    it("begin voting succeeds", async () => {
        expect.assertions(3);

        await arbitration.loadFixtures("config", require("../fixtures/arbitration/config.json"));

        await arbitration.loadFixtures("elections", {
            "arbitration": [
                {
                    election_id: 0,
                    ballot_name: "",
                    info_url: "info",
                    candidates: [{ name: "user1", votes: "0.0000 VOTE" }, { name: "user2", votes: "0.0000 VOTE" }, { name: "user3", votes: "0.0000 VOTE" }],
                    available_seats: 2,
                    begin_add_candidates_ts: "1999-12-01T00:16:10",
                    end_add_candidates_ts: "1999-12-31T00:16:10",
                    begin_voting_ts: "1970-01-01T00:00:00",
                    end_voting_ts: "1970-01-01T00:00:00",
                    status: 1
                }
            ]
        })

        await arbitration.contract.beginvoting({ ballot_name: "ballotname11", runoff: false },
            [{
                actor: admin.accountName,
                permission: "active"
            }]);
        
        const elections = arbitration.getTableRowsScoped("elections")["arbitration"];
        expect(elections.find(elec => elec.election_id === '0')).toEqual({
            election_id: '0',
            ballot_name: 'ballotname11',
            info_url: 'info',
            candidates: [{ name: "user1", votes: "0.0000 VOTE" }, { name: "user2", votes: "0.0000 VOTE" }, { name: "user3", votes: "0.0000 VOTE" }],
            available_seats: 2,
            begin_add_candidates_ts: "1999-12-01T00:16:10.000",
            end_add_candidates_ts: "1999-12-31T00:16:10.000",
            begin_voting_ts: '2000-01-01T00:00:00.000',
            end_voting_ts: '2000-01-30T00:00:00.000',
            status: 2
        });
        
        
        const ballot = decide.getTableRowsScoped("ballots")["telos.decide"];
        expect(ballot.find(bal => bal.ballot_name === "ballotname11").options).toEqual([
            { key: 'user1', value: '0.0000 VOTE' },
            { key: 'user2', value: '0.0000 VOTE' },
            { key: 'user3', value: '0.0000 VOTE' },
        ]);
        expect(ballot.find(bal => bal.ballot_name === "ballotname11").max_options).toEqual(2);
    });

    it("begin voting succeeds and skips voting when candidates <= available seats", async () => {
        await arbitration.loadFixtures("config", require("../fixtures/arbitration/config.json"));

        await arbitration.loadFixtures("elections", {
            "arbitration": [
                {
                    election_id: 0,
                    ballot_name: "",
                    info_url: "info",
                    candidates: [{ name: "user1", votes: "0.0000 VOTE" }, { name: "user2", votes: "0.0000 VOTE" }],
                    available_seats: 2,
                    begin_add_candidates_ts: "1999-12-01T00:16:10",
                    end_add_candidates_ts: "1999-12-31T00:16:10",
                    begin_voting_ts: "1970-01-01T00:00:00",
                    end_voting_ts: "1970-01-01T00:00:00",
                    status: 1
                }
            ]
        })

         await arbitration.contract.beginvoting({ ballot_name: "ballotname11", runoff: false },
            [{
                actor: admin.accountName,
                permission: "active"
            }]);
        
        const elections = arbitration.getTableRowsScoped("elections")["arbitration"];
        expect(elections.find(elec => elec.election_id === '0')).toEqual({  
            election_id: '0',
            ballot_name: '',
            info_url: 'info',
            candidates: [{ name: "user1", votes: "0.0000 VOTE" }, { name: "user2", votes: "0.0000 VOTE" }],
            available_seats: 2,
            begin_add_candidates_ts: "1999-12-01T00:16:10.000",
            end_add_candidates_ts: "1999-12-31T00:16:10.000",
            begin_voting_ts: '2000-01-01T00:00:00.000',
            end_voting_ts: '2000-01-01T00:00:00.000',
            status: 3
        })

        const arbitrators = arbitration.getTableRowsScoped("arbitrators")["arbitration"];
        expect(arbitrators.length).toEqual(2);

        const nominees = arbitration.getTableRowsScoped("nominees")["arbitration"];
        expect(nominees).toBeUndefined();
    })

    it("fails if election does not exist", async () => {
        await arbitration.loadFixtures("config", require("../fixtures/arbitration/config.json"));

        await expect(arbitration.contract.beginvoting({ ballot_name: "ballotname11", runoff: false },
        [{
            actor: admin.accountName,
            permission: "active"
        }])).rejects.toThrow("Election doesn't exist");
    });

    it("fails if ballot name is already used", async () => {
        await arbitration.loadFixtures("config", require("../fixtures/arbitration/config.json"));

        await arbitration.loadFixtures("elections", {
            "arbitration": [
                {
                    election_id: 1,
                    ballot_name: "ballotname11",
                    info_url: "info",
                    candidates: [{ name: "user1", votes: "0.0000 VOTE" }, { name: "user2", votes: "0.0000 VOTE" },{ name: "user3", votes: "0.0000 VOTE" }],
                    available_seats: 5,
                    begin_add_candidates_ts: "1999-12-01T00:16:10",
                    end_add_candidates_ts: "1999-12-31T00:16:10",
                    begin_voting_ts: "1970-01-01T00:00:00",
                    end_voting_ts: "1970-01-01T00:00:00",
                    status: 3
                }
            ]
        })
        await arbitration.loadFixtures("elections", {
            "arbitration": [
                {
                    election_id: 0,
                    ballot_name: "",  
                    info_url: "info",
                    candidates: [{ name: "user1", votes: "0.0000 VOTE" }, { name: "user2", votes: "0.0000 VOTE" },{ name: "user3", votes: "0.0000 VOTE" }],
                    available_seats: 2,
                    begin_add_candidates_ts: "1999-12-01T00:16:10",
                    end_add_candidates_ts: "1999-12-31T00:16:10",
                    begin_voting_ts: "1970-01-01T00:00:00",
                    end_voting_ts: "1970-01-01T00:00:00",
                    status: 1
                }
            ]
        })

        await expect(arbitration.contract.beginvoting({ ballot_name: "ballotname11", runoff: false },
        [{
            actor: admin.accountName,
            permission: "active"
        }])).rejects.toThrow("Ballot name already used");
    });

    it("fails if election is not in created status", async () => {
        await arbitration.loadFixtures("config", require("../fixtures/arbitration/config.json"));

        await arbitration.loadFixtures("elections", {
            "arbitration": [
                {
                    election_id: 0,
                    ballot_name: "",
                    
                    info_url: "info",
                    candidates: [{ name: "user1", votes: "0.0000 VOTE" }, { name: "user2", votes: "0.0000 VOTE" },{ name: "user3", votes: "0.0000 VOTE" }],
                    available_seats: 5,
                    begin_add_candidates_ts: "1999-12-01T00:16:10",
                    end_add_candidates_ts: "1999-12-31T00:16:10",
                    begin_voting_ts: "1970-01-01T00:00:00",
                    end_voting_ts: "1970-01-01T00:00:00",
                    status: 2
                }
            ]
        })

        await expect(arbitration.contract.beginvoting({ ballot_name: "ballotname11", runoff: false },
        [{
            actor: admin.accountName,
            permission: "active"
        }])).rejects.toThrow("Election must be in created status");
    });

    it("fails if time for adding candidates has not ended", async () => {
        await arbitration.loadFixtures("config", require("../fixtures/arbitration/config.json"));

        await arbitration.loadFixtures("elections", {
            "arbitration": [
                {
                    election_id: 0,
                    ballot_name: "",
                    
                    info_url: "info",
                    candidates: [{ name: "user1", votes: "0.0000 VOTE" }, { name: "user2", votes: "0.0000 VOTE" },{ name: "user3", votes: "0.0000 VOTE" }],
                    available_seats: 5,
                    begin_add_candidates_ts: "1999-12-01T00:16:10",
                    end_add_candidates_ts: "2000-01-05T00:16:10",
                    begin_voting_ts: "1970-01-01T00:00:00",
                    end_voting_ts: "1970-01-01T00:00:00",
                    status: 1
                }
            ]
        })

        await expect(arbitration.contract.beginvoting({ ballot_name: "ballotname11", runoff: false },
        [{
            actor: admin.accountName,
            permission: "active"
        }])).rejects.toThrow("Can't start voting process until add candidates time has ended.");
    });

    it("fails if SC does not have enough funds", async () => {
        await arbitration.loadFixtures("config", {
            "arbitration": [
                {
                    "contract_version": "0.1.0",
                    "admin": "admin",
                    "max_elected_arbs": 2,
                    "election_voting_ts": 2505600,
                    "runoff_election_voting_ts": 604800,
                    "election_add_candidates_ts": 604800,
                    "arb_term_length": 31536000,
                    "current_election_id": "0",
                    "available_funds": "5.0000 TLOS",
                    "reserved_funds": "0.0000 TLOS",
                    "max_claims_per_case": 10,
                    "fee_usd": "10.0000 USD",
                    "claimant_accepting_offers_ts": 604800,

                }
            ]
        })

        await arbitration.loadFixtures("elections", {
            "arbitration": [
                {
                    election_id: 0,
                    ballot_name: "",
                    info_url: "info",
                    candidates: [{ name: "user1", votes: "0.0000 VOTE" }, { name: "user2", votes: "0.0000 VOTE" },{ name: "user3", votes: "0.0000 VOTE" }],
                    available_seats: 2,
                    begin_add_candidates_ts: "1999-12-01T00:16:10",
                    end_add_candidates_ts: "1999-12-31T00:16:10",
                    begin_voting_ts: "1970-01-01T00:00:00",
                    end_voting_ts: "1970-01-01T00:00:00",
                    status: 1
                }
            ]
         })
    
        await expect(arbitration.contract.beginvoting({ ballot_name: "ballotname11", runoff: false },
        [{
            actor: admin.accountName,
            permission: "active"
        }])).rejects.toThrow("The SC doesn't have enough funds to cover the ballot fee");
    });

    it("fails to begin voting if user different than admin or SC account tries", async () => {
        await arbitration.loadFixtures("config", require("../fixtures/arbitration/config.json"));

        await arbitration.loadFixtures("elections", {
            "arbitration": [
                {
                    election_id: 0,
                    ballot_name: "",
                    
                    info_url: "info",
                    candidates: [{ name: "user1", votes: "0.0000 VOTE" }, { name: "user2", votes: "0.0000 VOTE" },{ name: "user3", votes: "0.0000 VOTE" }],
                    available_seats: 5,
                    begin_add_candidates_ts: "1999-12-01T00:16:10",
                    end_add_candidates_ts: "1999-12-31T00:16:10",
                    begin_voting_ts: "1970-01-01T00:00:00",
                    end_voting_ts: "1970-01-01T00:00:00",
                    status: 1
                }
            ]
         })
        
        await expect(arbitration.contract.beginvoting({ ballot_name: "ballotname11", runoff: false },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow("Only admin and SC account can start a voting process");
    })

});