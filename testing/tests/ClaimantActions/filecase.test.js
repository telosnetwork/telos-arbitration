const { loadConfig, Blockchain } = require("@klevoya/hydra");

const config = loadConfig("hydra.yml");

describe("File Case Telos Arbitration Smart Contract Tests", () => {
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

    });

    it("file case", async () => {
        expect.assertions(2);

        await arbitration.contract.filecase({
            claimant: "user1",
            claim_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKdFe5a",
            lang_codes: [0, 1, 2],
            respondant: "user2",
            claim_category: 3,
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])

        const casefiles = arbitration.getTableRowsScoped("casefiles")[arbitration.accountName];
        expect(casefiles.find(cf => cf.case_id === "0")).toEqual({
            case_id: '0',
            case_status: 0,
            claimant: 'user1',
            respondant: 'user2',
            arbitrators: [],
            approvals: [],
            number_claims: 1,
            number_offers: 0,
            required_langs: [ 0, 1, 2 ],
            case_ruling: '',
            update_ts: '2000-01-01T00:00:00.000',
            fee_paid_tlos: "0.0000 TLOS",
            arbitrator_cost_tlos: "0.0000 TLOS",
            sending_offers_until_ts: "1970-01-01T00:00:00.000"
        });  

        const claims = arbitration.getTableRowsScoped("claims")[""];
        expect(claims.find(cl => cl.claim_id === "0")).toEqual({
            claim_id: '0',
            claim_summary: 'QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKdFe5a',
            decision_link: '',
            response_link: '',
            status: 1,
            claim_category: 3,
            claim_info_needed: false,
            response_info_needed: false,
            claimant_limit_time: "1970-01-01T00:00:00.000",
            respondant_limit_time: "1970-01-01T00:00:00.000"
        })
    })

    it("fails if claim category not found", async () => {
         await expect(arbitration.contract.filecase({
            claimant: "user1",
            claim_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKdFe5a",
            lang_codes: [0, 1, 2],
            respondant: "user2",
            claim_category: 24,
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow("Claim category not found");
    })

    
    it("fails if respondant is not an account", async () => {
         await expect(arbitration.contract.filecase({
            claimant: "user1",
            claim_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKdFe5a",
            lang_codes: [0, 1, 2],
            respondant: "user3",
            claim_category: 3,
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow("Respondant must be an account");
    })

    it("fails if not valid claim link", async () => {
        await expect(arbitration.contract.filecase({
            claimant: "user1",
            claim_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKd",
            lang_codes: [0, 1, 2],
            respondant: "user2",
            claim_category: 3,
        },
            [{
              actor: user1.accountName,
              permission: "active"
            }])).rejects.toThrow("invalid ipfs string, valid schema: <hash>");
    })

    it("fails if other than user tries to file case", async () => {
        await expect(arbitration.contract.filecase({
            claimant: "user1",
            claim_link: "QmTtDqWzo1TXU7pf2PodLNjpcpQQCXhLiQXi6wZvKdFe5a",
            lang_codes: [0, 1, 2],
            respondant: "user2",
            claim_category: 3,
        },
            [{
              actor: user2.accountName,
              permission: "active"
            }])).rejects.toThrow();
    })

   
});