const { loadConfig, Blockchain } = require("@klevoya/hydra");

const config = loadConfig("hydra.yml");

describe("Cancel Case Telos Arbitration Smart Contract Tests", () => {
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

        await eosiotoken.loadFixtures();

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
                    "reserved_funds": "50.0000 TLOS",
                    "max_claims_per_case": 10,
                    "fee_usd": "10.0000 USD",
                    "claimant_accepting_offers_ts": 604800,

                }
            ]
        })


        await arbitration.loadFixtures("casefiles", {
            arbitration: [
                {
                    case_id: '0',
                    case_status: 1,
                    claimant: 'user1',
                    respondant: 'user2',
                    arbitrators: [],
                    approvals: [],
                    number_claims: 1,
                    number_offers: 1,
                    required_langs: [ 0, 1, 2 ],
                    case_ruling: '',
                    recusal: "",
                    update_ts: '2000-01-01T00:00:00.000',
                    fee_paid_tlos: "50.0000 TLOS",
                    arbitrator_cost_tlos: "150.0000 TLOS",
                    sending_offers_until_ts: "2000-01-10T00:00:00.000"
                },
                {
                    case_id: '1',
                    case_status: 1,
                    claimant: 'user1',
                    respondant: 'user2',
                    arbitrators: [],
                    approvals: [],
                    number_claims: 1,
                    number_offers: 0,
                    required_langs: [ 0, 1, 2 ],
                    case_ruling: '',
                    recusal: "",
                    update_ts: '2000-01-01T00:00:00.000',
                    fee_paid_tlos: "50.0000 TLOS",
                    arbitrator_cost_tlos: "150.0000 TLOS",
                    sending_offers_until_ts: "2000-01-10T00:00:00.000"
                },
            ]
            
        })

         await arbitration.loadFixtures("claims", {
            "": [
                {
                    claim_id: '0',
                    claim_summary: 'QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKdFe5a',
                    decision_link: '',
                    response_link: '',
                    status: 1,
                    claim_category: 1, 
                    claimant_limit_time: '2000-01-01T00:00:00.000',
                    claim_info_needed: false,
                    respondant_limit_time: '2000-01-01T00:00:00.000',
                    response_info_needed: false,
                }
            ]
        })

    });

    it("cancels case", async () => {
        expect.assertions(3);

        await arbitration.contract.cancelcase({
            case_id: "0",
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])
        
        const casefiles = arbitration.getTableRowsScoped("casefiles")[arbitration.accountName];
        expect(casefiles.find(cf => cf.case_id === "0").case_status).toEqual(8);  

        const config = arbitration.getTableRowsScoped("config")[arbitration.accountName][0];
        expect(config.reserved_funds).toEqual("0.0000 TLOS");
        expect(config.available_funds).toEqual("55.0000 TLOS");
    });

    it("cancels case and returns fee", async () => {
        expect.assertions(3);
        
        await arbitration.contract.cancelcase({
            case_id: "1",
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])
        
        const casefiles = arbitration.getTableRowsScoped("casefiles")[arbitration.accountName];
        expect(casefiles.find(cf => cf.case_id === "1").case_status).toEqual(8);  

        const config = arbitration.getTableRowsScoped("config")[arbitration.accountName][0];
        expect(config.reserved_funds).toEqual("0.0000 TLOS");

        const balances = arbitration.getTableRowsScoped("accounts");
        expect(balances["user1"][0]).toEqual({ balance: "50.0000 TLOS" });
    });

    it("fails if case not found", async () => {
        await expect(arbitration.contract.cancelcase({
            case_id: "5",
        },
            [{
                actor: user1.accountName,
                permission: "active"
            }])).rejects.toThrow("Case not found");
    });

    it("fails if case status is not awaiting arbs", async () => {

        await arbitration.loadFixtures("casefiles", {
            arbitration: [{
                case_id: '2',
                case_status: 5,
                claimant: 'user1',
                respondant: 'user2',
                arbitrators: [],
                approvals: [],
                number_claims: 1,
                number_offers: 1,
                required_langs: [ 0, 1, 2 ],
                case_ruling: '',
                recusal: "",
                update_ts: '2000-01-01T00:00:00.000',
                fee_paid_tlos: "0.0000 TLOS",
                arbitrator_cost_tlos: "150.0000 TLOS",
                sending_offers_until_ts: "2000-01-10T00:00:00.000"
            }]
            
        })

        await expect(arbitration.contract.cancelcase({
            case_id: "2",
        },
            [{
                actor: user1.accountName,
                permission: "active"
            }])).rejects.toThrow("Case status must be in AWAITING ARBS");
    });

    it("fails if user other than claimant tries", async () => {
        await expect(arbitration.contract.cancelcase({
            case_id: "0",
        },
            [{
                actor: user2.accountName,
                permission: "active"
            }])).rejects.toThrow();
    });
    
});