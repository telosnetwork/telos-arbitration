const { loadConfig, Blockchain } = require("@klevoya/hydra");

const config = loadConfig("hydra.yml");

describe("Update Claim Telos Arbitration Smart Contract Tests", () => {
    let blockchain = new Blockchain(config);
    let arbitration = blockchain.createAccount("arbitration");
    let admin = blockchain.createAccount("admin");
    let user1 = blockchain.createAccount("user1");
    let user2 = blockchain.createAccount("user2");
    let user3 = blockchain.createAccount("user3");

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


        await arbitration.loadFixtures("casefiles", {
            arbitration: [{
                case_id: '0',
                case_status: 3,
                claimant: 'user1',
                respondant: 'user2',
                arbitrators: [],
                approvals: [],
                number_claims: 1,
                number_offers: 1,
                required_langs: [0, 1, 2],
                case_ruling: '',
                recusal: "",
                update_ts: '2000-01-01T00:00:00.000',
                fee_paid_tlos: "0.0000 TLOS",
                arbitrator_cost_tlos: "150.0000 TLOS",
                sending_offers_until_ts: "2000-01-10T00:00:00.000"
            }]
            
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
                },
                {
                    claim_id: '1',
                    claim_summary: 'QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKdFe5a',
                    decision_link: '',
                    response_link: 'QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKdFe5a',
                    status: 2,
                    claim_category: 1,
                    claimant_limit_time: '2000-01-01T00:00:00.000',
                    claim_info_needed: true,
                    respondant_limit_time: '2000-01-01T00:00:00.000',
                    response_info_needed: false,
                }
            ]
        })

    });

    it("updates claim if is filed", async () => {
        expect.assertions(1);

        await arbitration.contract.updateclaim({
            case_id: "0",
            claim_id: "0",
            claimant: "user1",
            claim_link: "QmTtDqWzo1TXU7pf2ROFLNjpcpQQCXhLiQXi6wZvKd5AFe",
        },
            [{
                actor: user1.accountName,
                permission: "active"
            }]);
        
        const claims = arbitration.getTableRowsScoped("claims")[""];
        expect(claims.find(cl => cl.claim_id === "0")).toEqual({
            claim_id: '0',
            claim_summary: 'QmTtDqWzo1TXU7pf2ROFLNjpcpQQCXhLiQXi6wZvKd5AFe',
            decision_link: '',
            response_link: '',
            status: 1,
            claim_category: 1,
            claimant_limit_time: '2000-01-01T00:00:00.000',
            claim_info_needed: false,
            respondant_limit_time: '2000-01-01T00:00:00.000',
            response_info_needed: false,
        })
    });

    it("updates claim if responded and more info is asked", async () => {
        expect.assertions(1);

        await arbitration.contract.updateclaim({
            case_id: "0",
            claim_id: "1",
            claimant: "user1",
            claim_link: "QmTtDqWzo1TXU7pf2ROFLNjpcpQQCXhLiQXi6wZvKd5AFe",
        },
            [{
                actor: user1.accountName,
                permission: "active"
            }]);
        
        const claims = arbitration.getTableRowsScoped("claims")[""];
        expect(claims.find(cl => cl.claim_id === "1")).toEqual({
            claim_id: '1',
            claim_summary: 'QmTtDqWzo1TXU7pf2ROFLNjpcpQQCXhLiQXi6wZvKd5AFe',
            decision_link: '',
            response_link: 'QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKdFe5a',
            status: 2,
            claim_category: 1,
            claimant_limit_time: '2000-01-01T00:00:00.000',
            claim_info_needed: false,
            respondant_limit_time: '2000-01-01T00:00:00.000',
            response_info_needed: false,
        })
    });


    it("fails if case not found", async () => {
        await expect(arbitration.contract.updateclaim({
            case_id: "1",
            claim_id: "1",
            claimant: "user1",
            claim_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow("Case not found");
    });

    it("fails if user is not the claimant", async () => {
         await expect(arbitration.contract.updateclaim({
            case_id: "0",
            claim_id: "0",
            claimant: "user2",
            claim_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
        },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow("must be the claimant of this case_id");
    });

    it("fails if case is not in case investigation", async () => {
        await arbitration.loadFixtures("casefiles", {
            arbitration: [{
                case_id: '1',
                case_status: 2,
                claimant: 'user1',
                respondant: 'user2',
                arbitrators: [],
                approvals: [],
                number_claims: 1,
                number_offers: 1,
                required_langs: [0, 1, 2],
                case_ruling: '',
                recusal: "",
                update_ts: '2000-01-01T00:00:00.000',
                fee_paid_tlos: "0.0000 TLOS",
                arbitrator_cost_tlos: "150.0000 TLOS",
                sending_offers_until_ts: "2000-01-10T00:00:00.000"
            }]
            
        })

        await arbitration.loadFixtures("claims", {
            "............1": [
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

        await expect(arbitration.contract.updateclaim({
            case_id: "1",
            claim_id: "0",
            claimant: "user1",
            claim_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow("case status does NOT allow responses at this time");
    });

    it("fails if claim not found", async () => {
        await expect(arbitration.contract.updateclaim({
            case_id: "0",
            claim_id: "5",
            claimant: "user1",
            claim_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow("Claim not found");
    });

    it("fails if claim is accepted", async () => {
        await arbitration.loadFixtures("claims", {
            "": [
                {
                    claim_id: '2',
                    claim_summary: 'QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKdFe5a',
                    decision_link: '',
                    response_link: '',
                    status: 3,
                    claim_category: 1,
                    claimant_limit_time: '2000-01-01T00:00:00.000',
                    claim_info_needed: false,
                    respondant_limit_time: '2000-01-01T00:00:00.000',
                    response_info_needed: false,
                }
            ]
        })

        await expect(arbitration.contract.updateclaim({
            case_id: "0",
            claim_id: "2",
            claimant: "user1",
            claim_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow("Claim cannot be updated");

    });

    it("fails if claim is denied", async () => {
        await arbitration.loadFixtures("claims", {
            "": [
                {
                    claim_id: '2',
                    claim_summary: 'QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKdFe5a',
                    decision_link: '',
                    response_link: '',
                    status: 4,
                    claim_category: 1,
                    claimant_limit_time: '2000-01-01T00:00:00.000',
                    claim_info_needed: false,
                    respondant_limit_time: '2000-01-01T00:00:00.000',
                    response_info_needed: false,
                }
            ]
        })

        await expect(arbitration.contract.updateclaim({
            case_id: "0",
            claim_id: "2",
            claimant: "user1",
            claim_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow("Claim cannot be updated");
    });

    it("fails if claim responded and not info needed", async () => {
        await arbitration.loadFixtures("claims", {
            "": [
                {
                    claim_id: '2',
                    claim_summary: 'QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKdFe5a',
                    decision_link: '',
                    response_link: '',
                    status: 2,
                    claim_category: 1,
                    claimant_limit_time: '2000-01-01T00:00:00.000',
                    claim_info_needed: false,
                    respondant_limit_time: '2000-01-01T00:00:00.000',
                    response_info_needed: false,
                }
            ]
        })

        await expect(arbitration.contract.updateclaim({
            case_id: "0",
            claim_id: "2",
            claimant: "user1",
            claim_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow("Claim cannot be updated");
    });


    it("fails if claim link not valid", async () => {
        await expect(arbitration.contract.updateclaim({
            case_id: "0",
            claim_id: "0",
            claimant: "user1",
            claim_link: "QmTtDqWzo1f2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow("invalid ipfs string, valid schema: <hash>");
    });

    it("fails if user other than claimant tries to update", async () => {
        await expect(arbitration.contract.updateclaim({
            case_id: "0",
            claim_id: "0",
            claimant: "user1",
            claim_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd5AFe",
        },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow();
    });




    
});