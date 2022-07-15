const { loadConfig, Blockchain } = require("@klevoya/hydra");

const config = loadConfig("hydra.yml");

describe("End Election Telos Arbitration Smart Contract Tests", () => {
    let blockchain = new Blockchain(config);
    let arbitration = blockchain.createAccount("arbitration");
    let admin = blockchain.createAccount("admin");
    let user1 = blockchain.createAccount("user1");
    let user2 = blockchain.createAccount("user2");
    let user3 = blockchain.createAccount("user3");
    let user4 = blockchain.createAccount("user4");
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

        decide.updateAuth(`active`, `owner`, {
            accounts: [
                {
                permission: {
                    actor: decide.accountName,
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
        decide.resetTables();

        await decide.loadFixtures();
        await eosiotoken.loadFixtures();
    });

    it("end election", async () => {
        expect.assertions(3);
        
        await decide.loadFixtures("ballots", {
            "telos.decide": [{
                ballot_name: 'ballot',
                category: 'election',
                publisher: arbitration.accountName,
                status: 'voting',
                title: 'Telos Arbitrators Election',
                description: 'This is an election',
                content: 'Content',
                treasury_symbol: '4,VOTE',
                voting_method: '1token1vote',
                min_options: 1,
                max_options: 1,
                options: [{key: "user1", value: "350.0000 VOTE"}, {key: "user4", value: "0.0000 VOTE"},{key: "user2", value: "400.0000 VOTE"}, {key: "user3", value: "345.0000 VOTE"}],
                total_voters: 0,
                total_delegates: 0,
                total_raw_weight: '0.0000 VOTE',
                cleaned_count: 0,
                settings: [ { "key": "lightballot", "value": 0 }, { "key": "revotable", "value": 1 }, { "key": "voteliquid", "value": 0 }, { "key": "votestake", "value": 1 } ] ,
                begin_time: '1999-12-01T00:00:00.000',
                end_time: '1999-12-31T00:00:00.000'
            }]
        })


        await arbitration.loadFixtures("config", require("../fixtures/arbitration/config.json"));

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
                },
                {
                    nominee_name: "user3",
                    credentials_link: "link",
                    application_time: '1999-11-07T00:00:00.000',
                },
                {
                    nominee_name: "user4",
                    credentials_link: "link",
                    application_time: '1999-11-07T00:00:00.000',
                }
            ]
        });

        await arbitration.loadFixtures("elections", {
            "arbitration": [
                {
                    election_id: 0,
                    ballot_name: "ballot",     
                    info_url: "info",
                    candidates: [
                        { name: "user1", votes: "0.0000 VOTE" },
                        { name: "user2", votes: "0.0000 VOTE" },
                        { name: "user3", votes: "0.0000 VOTE" },
                        { name: "user4", votes: "0.0000 VOTE" }
                    ],
                    available_seats: 2,
                    begin_add_candidates_ts: "1999-11-01T00:16:10",
                    end_add_candidates_ts: "1999-11-08T00:16:10",
                    begin_voting_ts: "1999-12-01T00:00:00",
                    end_voting_ts: "1999-12-31T00:00:00",
                    status: 2
                }
            ]
        })
        
        await arbitration.contract.endelection({},
            [{
                actor: admin.accountName,
                permission: "active"
            }]);
                
        const nominees = arbitration.getTableRowsScoped("nominees")[arbitration.accountName];
        expect(nominees).toBeUndefined();

        const arbitrators = arbitration.getTableRowsScoped("arbitrators")[arbitration.accountName];
        expect(arbitrators.length).toEqual(2);

        const elections = arbitration.getTableRowsScoped("elections")[arbitration.accountName];
        expect(elections.find(election => election.election_id == "0")).toEqual({
            "available_seats": 2,
            "ballot_name": "ballot",
            "begin_add_candidates_ts": "1999-11-01T00:16:10.000",
            "begin_voting_ts": "1999-12-01T00:00:00.000",
            "candidates": [{ "name": "user2", "votes": "400.0000 VOTE" }, { "name": "user1", "votes": "350.0000 VOTE" },
                { "name": "user3", "votes": "345.0000 VOTE" }, {"name":"user4", "votes": "0.0000 VOTE"}],
            "election_id": "0",
            "end_add_candidates_ts": "1999-11-08T00:16:10.000",
            "end_voting_ts": "1999-12-31T00:00:00.000",
            "info_url": "info",
            "status": 3,
        });
    });

    it("end election and if there's a tie, don't add those candidates as arbitrators", async () => {
        await decide.loadFixtures("ballots", {
            "telos.decide": [{
                ballot_name: 'ballot',
                category: 'election',
                publisher: arbitration.accountName,
                status: 'voting',
                title: 'Telos Arbitrators Election',
                description: 'This is an election',
                content: 'Content',
                treasury_symbol: '4,VOTE',
                voting_method: '1token1vote',
                min_options: 1,
                max_options: 1,
                options: [{ key: "user1", value: "350.0000 VOTE" }, { key: "user4", value: "350.0000 VOTE" }, { key: "user5", value: "350.0000 VOTE" }, {key: "user2", value: "400.0000 VOTE"}, {key: "user3", value: "345.0000 VOTE"}],
                total_voters: 0,
                total_delegates: 0,
                total_raw_weight: '0.0000 VOTE',
                cleaned_count: 0,
                settings: [ { "key": "lightballot", "value": 0 }, { "key": "revotable", "value": 1 }, { "key": "voteliquid", "value": 0 }, { "key": "votestake", "value": 1 } ] ,
                begin_time: '1999-12-01T00:00:00.000',
                end_time: '1999-12-31T00:00:00.000'
            }]
        })


        await arbitration.loadFixtures("config", require("../fixtures/arbitration/config.json"));

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
                },
                {
                    nominee_name: "user3",
                    credentials_link: "link",
                    application_time: '1999-11-07T00:00:00.000',
                },
                {
                    nominee_name: "user4",
                    credentials_link: "link",
                    application_time: '1999-11-07T00:00:00.000',
                },
                {
                    nominee_name: "user5",
                    credentials_link: "link",
                    application_time: '1999-11-07T00:00:00.000',
                }
            ]
        });

        await arbitration.loadFixtures("elections", {
            "arbitration": [
                {
                    election_id: 0,
                    ballot_name: "ballot",     
                    info_url: "info",
                    candidates: [
                        { name: "user1", votes: "0.0000 VOTE" },
                        { name: "user2", votes: "0.0000 VOTE" },
                        { name: "user3", votes: "0.0000 VOTE" },
                        { name: "user4", votes: "0.0000 VOTE" },
                        { name: "user5", votes: "0.0000 VOTE"},
                    ],
                    available_seats: 2,
                    begin_add_candidates_ts: "1999-11-01T00:16:10",
                    end_add_candidates_ts: "1999-11-08T00:16:10",
                    begin_voting_ts: "1999-12-01T00:00:00",
                    end_voting_ts: "1999-12-31T00:00:00",
                    status: 2
                }
            ]
        })
        
        await arbitration.contract.endelection({},
            [{
                actor: admin.accountName,
                permission: "active"
            }]);
                
        const nominees = arbitration.getTableRowsScoped("nominees")[arbitration.accountName];
        expect(nominees.find(nom => nom.nominee_name === "user2")).toBeUndefined();
        expect(nominees.find(nom => nom.nominee_name === "user3")).toBeUndefined();
        expect(nominees.find(nom => nom.nominee_name === "user1")).toEqual({
            nominee_name: "user1",
            credentials_link: "link",
            application_time: '1999-11-05T00:00:00.000',
        });
        expect(nominees.find(nom => nom.nominee_name === "user4")).toEqual({
            nominee_name: "user4",
            credentials_link: "link",
            application_time: '1999-11-07T00:00:00.000',
        });
        expect(nominees.find(nom => nom.nominee_name === "user5")).toEqual({
            nominee_name: "user5",
            credentials_link: "link",
            application_time: '1999-11-07T00:00:00.000',
        });


        const arbitrators = arbitration.getTableRowsScoped("arbitrators")[arbitration.accountName];
        expect(arbitrators.length).toEqual(1);

        const elections = arbitration.getTableRowsScoped("elections")[arbitration.accountName];
        expect(elections.find(election => election.election_id == "0")).toEqual({
            "available_seats": 2,
            "ballot_name": "ballot",
            "begin_add_candidates_ts": "1999-11-01T00:16:10.000",
            "begin_voting_ts": "1999-12-01T00:00:00.000",
            "candidates": [{ "name": "user2", "votes": "400.0000 VOTE" }, { "name": "user1", "votes": "350.0000 VOTE" },
                {"name":"user4", "votes": "350.0000 VOTE"},{"name":"user5", "votes": "350.0000 VOTE"},{ "name": "user3", "votes": "345.0000 VOTE" }],
            "election_id": "0",
            "end_add_candidates_ts": "1999-11-08T00:16:10.000",
            "end_voting_ts": "1999-12-31T00:00:00.000",
            "info_url": "info",
            "status": 3,
        });

        expect(elections.find(election => election.election_id == "1")).toEqual({
            "available_seats": 1,
            "ballot_name": "j5urkutphcskg",
            "begin_add_candidates_ts": "2000-01-01T00:00:00.000",
            "begin_voting_ts": "2000-01-01T00:00:00.000",
            "candidates": [{"name":"user5", "votes": "0.0000 VOTE"},{"name":"user1", "votes": "0.0000 VOTE"},{"name": "user4", "votes": "0.0000 VOTE"}],
            "election_id": "1",
            "end_add_candidates_ts": "2000-01-01T00:00:00.000",
            "end_voting_ts": "2000-01-15T12:00:00.000",
            "info_url": "info",
            "status": 2,
        });


    });

    it("fails if election not found", async () => {
        await arbitration.loadFixtures("config", require("../fixtures/arbitration/config.json"));

        await expect(arbitration.contract.endelection({},
        [{
            actor: admin.accountName,
            permission: "active"
        }])).rejects.toThrow("Election not found");
    });

    it("fails if SC doesn't have enough funds to cover the SC in case of tie", async () => {
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
                    candidates: [],
                    available_seats: 5,
                    begin_add_candidates_ts: "1999-11-01T00:16:10",
                    end_add_candidates_ts: "1999-11-08T00:16:10",
                    begin_voting_ts: "1999-12-01T00:00:00",
                    end_voting_ts: "1999-12-31T00:00:00",
                    status: 2
                }
            ]
        })

          await expect(arbitration.contract.endelection({},
        [{
            actor: admin.accountName,
            permission: "active"
        }])).rejects.toThrow("Not enough funds to cover ballot fees in case of new election");


    });

    it("fails if election is not in live status", async () => {
        await arbitration.loadFixtures("config", require("../fixtures/arbitration/config.json"));

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

        await expect(arbitration.contract.endelection({},
        [{
            actor: admin.accountName,
            permission: "active"
        }])).rejects.toThrow("Election status must be in live state");
    });

    it("fails if time for voting has not ended", async () => {
        await arbitration.loadFixtures("config", require("../fixtures/arbitration/config.json"));

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
                    begin_voting_ts: "2000-01-09T00:00:00",
                    end_voting_ts: "2000-01-15T00:00:00",
                    status: 2
                }
            ]
        })

        await expect(arbitration.contract.endelection({},
        [{
            actor: admin.accountName,
            permission: "active"
        }])).rejects.toThrow("Vote time has not ended");
    });


    it("fails to end election if user different than admin tries", async () => {
        await arbitration.loadFixtures("config", require("../fixtures/arbitration/config.json"));

        await expect(arbitration.contract.endelection({},
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow();
    })

});