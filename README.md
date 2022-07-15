# Arbitration Setup/Usage

The arbitration contract is a tool for Telos Blockchain Network members and Arbitrators to contractually file for and process arbitration within the network.

## Common Terminology

* `Claimant` : 

* `Respondant` :

* `Arbitrator` : 

## Telos Arbitration Configuration

Once the Smart Contract is initialized, an admin will be assigned. The idea is that the account `eosio` is set as an admin, so BPs are the ones controlling the Smart Contract. 

The admin will be then authorized to modify the config parameters of the Smart Contract using the following action:

* `void setconfig(uint16_t max_elected_arbs, uint32_t election_duration, uint32_t runoff_duration, uint32_t election_add_candidates_duration,  uint32_t arbitrator_term_length,  uint8_t max_claims_per_case, asset fee_usd, uint32_t claimant_accepting_offers_duration)`

    The setconfig action initializes the contract config with the given values.

    `max_elected_arbs` is the max number of arbs allowed to be elected at any given time.

    `election_duration` is the length of arbitrator elections in seconds.

    `runoff_duration` is the length of arbitrator second election performed if there are tied candidates in seconds

    `election_add_candidates_duration` is the length of time in seconds that accepts new candidates after initiating an election

    `arbitrator_term_length` is the term length of arbitrators in seconds.

    `max_claims_per_case` is the maximum number of claims each case can have.

    `fee_usd` is the fixed fee that claimants need to pay when setting a case as ready.

    `claimant_accepting_offers_duration` is the lenght of time in seconds the claimant has to accept any arbitrator offer before the case can be  closed due to inactivity.


## Telos Arbitration Procedure

The Telos Arbitration Smart Contract actions can be split in different blocks: the arbitrator election, the filing process, the case assignment and the case management. In this section we will explain how each procedure works and the different actions that can be used for that.

### Arbitrators Electing Process

An election is handled every time that there are available seats for being an arbitrator. Most of the parameters of the election, such as the voting time or the adding candidates time are set in the configuration by the BPs. 

To init a new election, the admin must call `initelection`, which will create a new arbitrator election as long as there isn't any election running and there is at least one available seat. 

* `initelection(string election_info)`
    
    `string election_info` is a string containing information about the election.

As soon as the election is initialized, there will be a fixed period of time in seconds, determined by the parameter `election_add_candidates_duration`, in which people can apply as a candidate. 

The candidate register is done in two steps: `regarb` and `candaddlead`. 

The `regarb` action will create an arbitrator candidate application for the given candidate. This will allow the user to apply for an existing arbitrator election.

* `regarb(name nominee, string credentials_link)`

    `nominee` is the account name of the nominee.

    `credentials_link` is a link (ideally an IPFS hash) to the candidate's credentials. Candidates that fail to supply or supply false credentials will not be considered for election.


A candidate can also decide to remove its application if it is not added to the election candidate's list.

* `unregnominee(name nominee)`

    `candidate` is the account to remove from the nominee's list.

A user can add himself as a nominee at any time, but only when an election process is being done the user will be allowed to send his candidature. To do so, the user will use `candaddlead`

* `candaddlead(name nominee)`

    `nominee` is the name of the account being added in the arbitrators candidates list.  

Note that candidates can only be added to an election **before** the period for adding candidates has ended. 

If the user wants to remove its candidature, and the candidates period has not ended, the user will be able to do so calling the following action: 

* `candrmvlead(name nominee)`

    `nominee` is the name of the account to remove from the arbitrators candidate list.

Once the adding candidates period has ended, a voting process will be done, since arbitrator candidates must be elected through a regular process in order to begin accepting cases.

The voting will be done using the **telos.decide** Smart Contract and will be initiated calling `beginvoting`

* `beginvoting(name ballot_name, bool runoff)`

    `ballot_name` is the name that will be used for creating a new ballot in the *telos.decide* Smart Contract

    `runoff` is a boolean that determines if the ballot we are creating is for breaking the tie between candidates. It will usually be false

In the case that there were less or equal number of candidates than available seats, all the candidates would be automatically be assigned as arbitrators. Therefore, a voting process will only take place in the case there are more candidates than spots.

To close the voting, the admin will call `endelection`. This action will only be called after the election is no longer open for voting in the *telos.decide* Smart Contract.

Arbitrators will be assigned by number of votes, meaning that the candidates that received the most votes will be assigned first until all the spots are covered. In the very unlikely case where there was a tie that caused a tied candidate to not be selected, a second voting with all the tied candidates would be conducted.

### Filing A Case Procedure

Any user can file a case, as long as they are willing to pay for the associated costs. To file a new case, a claimant will need to call `filecase`

* `filecase(name claimant, string claim_link, vector<uint16_t> lang_codes,std::optional<name> respondant, uint8_t claim_category)` 

    `claimant` is the user that is filing the case
    `claim_link` is a link (ideally an IPFS hash) to the claim's information
    `lang_codes` is the languages in which the claim is written and so the arbitrator that takes the case needs to know at least one.
    `respondant` is the user accused. It is an optional parameter, some cases might not have a respondant
    `claim_category` is the category of the claim 

Although most of the cases will have a single claim, more claims can be added if needed using `addclaim`.

* `addclaim(uint64_t case_id, string claim_link, name claimant, uint8_t claim_category)`

    `case_id` is the id of the case 
    `claim_link` is a link (ideally an IPFS hash) to the claim's information
    `claimant` is the user that is adding the claim the case and needs to match with the user that filed the case
    `claim_category` is the category of the claim

Claims can also be removed by using `removeclaim`

* `removeclaim(uint64_t case_id, uint64_t claim_id, name claimant)`
    `case_id` is the id of the case
    `claim_id` is the id of the claim the user is removing
    `claimant` is the user that added the claim

Moreover, the user can also dismiss the entire case while it is in drafting status by calling the following action:

* `shredcase(uint64_t case_id, name claimant)`
    `case_id` is the id of the case
    `claimant` is the user that filed the case

Whenever the user completes the filing, the case will be started calling `readycase`. In order to call this action, the user will need to pay a small fee, which is determined in the configuration by the BPs. In the scenario where the case was dismissed, this fee would be returned to the claimant. The idea of the fee is to prevent spam cases.

* `readycase(uint64_t case_id, name claimant)`
    `case_id` is the id of the case
    `claimant` is the user that filed the case

### Arbitrator Selection 

Once the case is ready, it will be moved into the AWAITING ARBS status. During this phase, arbitrators that are available will send their hourly rate plus the estimated amount of hours that will be spent for resolving the case. Then, the claimant will decide if wants to accept the offer or not. 

To send their rate, an arbitrator can do so calling the following action:

* `makeoffer(uint64_t case_id, int64_t offer_id, name arbitrator, asset hourly_rate, uint8_t estimated_hours)`
    `case_id` is the id of the case
    `offer_id` is -1 if it is a new offer or the id of the offer the arbitrator is trying to edit
    `arbitrator` is the name of the arbitrator that is sending the offer
    `hourly rate` is the price per hour that the arbitrator will bill
    `estimated_hours` is the number of hours the arbitrator will spend

**Note**: that in the configuration table there is a parameter called `sending_offers_until_ts` that determines the time period in which an arbitrator can send a new offer. After that period, no more offers will be accepted

**Note**: Arbitrators will only be allowed to send 1 offer per case, and they will be allowed to edit that offer as long as the claimant doesn't accept nor deny it.

Arbitrators can also remove an offer by calling:

* `dismissoffer(uint64_t case_id, uint64_t offer_id)`
    `case_id` is the id of the case
    `offer_id` is the id of the offer the arbitrator made

Claimant can decide to accept or deny an offer made by an arbitrator. To respond to an offer, the claimant will call `respondoffer`

* `respondoffer(uint64_t case_id, uint64_t offer_id, bool accept)`
    `case_id` is the id of the case
    `offer_id` is the id of the offer
    `accept` is a boolean that determines whether the offer is accepted or not

If the user is willing to accept the offer, he will need to pay all the arbitrator rate (*estimated hours x hourly rate*) upfront. The amount paid will be then frozen to reserved funds and will only be paid to the arbitrator after completion. 

There is no time limit for the claimant to respond to an offer. However, since the arbitrators can also dismiss them, if the claimant takes long enough there might me no offers available anymore. If the case reaches this situation, the claimant will be forced to cancel the case.

* `cancelcase(uint64_t case_id)`
    `case_id` is the id of the case

**Note**: If the case didn't receive any offer before cancelling, the fee will be returned to the claimant. Otherwise, the fee will be kept and added to available funds.

If an offer is accepted, the case will be moved to `ARBS_ASSIGNED`

### Case Management

When the assigned arbitrator is ready to start the case investigation, he will call `startcase`. 

* `startcase(uint64_t case_id, name assigned_arb, uint8_t number_days_respondant)`
    `case_id` is the id of the case
    `assigned_arb` is the name of the arbitrator
    `number_days_respondant` is the number of days the respondant, if there's one, has to respond the claim.
    This number of days is not a limit, meaning that the respondant can potentially send the information after the deadline. However, once the number of days has been accomplished the arbitrator will be able to resolve the claim, even if the respondant has not provided an answer yet.

As soon as this action is called, the case will move to `CASE_INVESTIGATION`. During this phase, the arbitrator will study the case filed by the claimant. Initially, a period of time will be set for the respondant to provide an answer to claim. However, if during the investigation the arbitrator needs more information, he can extend the time period using `reviewclaim`

* `reviewclaim(uint64_t case_id, uint64_t claim_id, name assigned_arb, bool claim_info_needed, bool response_info_needed, uint8_t number_days_claimant, uint8_t number_days_respondant)`
    `case_id` is the id of the case
    `claim_id` is the id of the claim being reviewed
    `assigned_arb` is the name of the arbitrator assigned to the case that is reviewing the claim
    `claim_info_needed` is a boolean that determines whether the claimant needs to provide more information
    `response_info_needed` is a boolean that determines whether the respondant needs to provide more information
    `number_of_days_claimant` is the number of days the claimant has to provide the information
    `number_of_days_respondant` is the number of days the respondant has to provide the information


Notice that by calling this action the arbitrator can also ask for extra information to the claimant. This action can be also used to extend the time the respondant has to provide the information

The claimant and the respondant will have different actions to provide the corresponding information. The claimant will need to call `updateclaim`, while the respondant will call `respond`

* `updateclaim(uint64_t case_id, uint64_t claim_id, name claimant, string claim_link)`
    `case_id` is the id of the case
    `claim_id` is the id of the claim
    `claimant` is the name of the claimant
    `claim_link` is a link (ideally an IPFS hash) to the claim's information

* `respond(uint64_t case_id, uint64_t claim_id, name respondant, string response_link)`
    `case_id` is the id of the case
    `claim_id` is the id of the claim
    `respondant` is the name of the respondant
    `response_link` is a link (ideally an IPFS hash) to the response's information

The assigned arbitrator will be able to resolve a claim only after the claimant and the respondant has provided the information or if the deadline assigned has ben surpassed. To settle a claim, it will be done by calling: `settleclaim`

* `settleclaim(uint64_t case_id, name assigned_arb, uint64_t claim_id, bool accept, string decision_link)`
    `case_id` is the id of the case
    `claim_id`is the id of the claim
    `assigned_arb` is the name of the arbitrator assigned to the case that is settling the claim
    `decision_link` is a link (ideally an IPFS hash) to the decision's information
    `accept` is a boolean that determines whether the claim is accepted or not

During the CASE INVESTIGATION phase, the arbitrator will review and settle each of the claims of the claimant. When all the claims are resolved, the arbitrator will set his final decision with the following action:

* `setruling(uint64_t case_id, name assigned_arb, string case_ruling)`
    `case_id` is the id of the case
    `assigned_arb` is the name of the arbitrator assigned to the case that is setting the case ruling
    `case_ruling` is a link (ideally an IPFS hash) to the case ruling

The case will move to the DECISION phase as soon as the ruling has been set. 

In this next phase, the admin will validate the case, which means to ensure that the arbitrator performed his job appropiately. If the case is considered valid, the case fee will be added to the Smart Contract funds and the arbitrator will be paid for his service. On the other hand, if the case is invalidated both the fee and the arbitrator rate cost will be returned to the claimant and the case will be considered DISMISSED. The case will be validated calling `validatecase`

* `validatecase(uint64_t case_id, bool proceed)`
    `case_id` is the id of the case
    `proceed` is a boolean that determines whether to proceed or not with the case enforcement

If the case is accepted, then the case will be moved to ENFORCEMENT. In this stage, the BPs will apply the ruling that has been set by the arbitrator. 

Finally, when the ruling has been successfully applied the case will be closed by calling `closecase`

* `closecase(uint64_t case_id)`
    `case_id` is the id of the case

### Recusing an arbitrator

There might be some scenarios where the arbitrator can no longer continue investigating the case. There might be the case where the arbitrator is suddenly unavailable, or maybe the there is a conflict of interests and the arbitrator is no longer impartial. An arbitrator assigned to a case can recuse calling the following action:

* `recuse(uint64_t case_id, string rationale, name assigned_arb)`
    `case_id` is the id of the case
    `string rationale` is a string containing the reason why the arbitrator is recusing
    `assigned_arb` is the name of the arbitrator assigned to the case that is recusing

When an arbitrator recuse, the case is automatically set as a MISTRIAL and all the funds (both the fee and arbitrator rate cost) are returned to the claimant.

**Note**: An arbitrator can only recuse if it is assigned to the case and the case decision has not been validated by the admin

The admin also has the power to force the recusal of an arbitrator from a case by calling `forcerecusal`. This action will perform as the one mentioned above, meaning that the case will be set as MISTRIAL and funds will be returned.

The admin will also have the power of dismiss an arbitrator from his position by calling `dismissarb`. When calling this action, the admin will have to decide if the arbitrator is authorized to close the cases he is investigation or if it is recused from all his cases.

* `dismissarb(name arbitrator, bool remove_from_cases)`
    `arbitrator` is the name of the arbitrator to be dismissed
    `remove_from_cases` if the arbitrator is removed from his active cases


### Case Lifecycle

* `CASE_SETUP` : When an arbitration case is filed it begins in the CASE_SETUP stage where the claimant can build the case file before opening it up for review by arbitrators. To ready the case, the claimant will need to pay a small fee. This fee is set to prevent spam.

* `AWAITING_ARBS` : After a case file has been built and is ready for review, the case enters the AWAITING_ARBS stage where any arbitrator can send an offer to the claimant with their hourly rate and the estimated number of hours. To accept an offer the claimant will need to pay the whole cost upfront. However, the funds will not be transferred to the arbitrator until the case is resolved, and can be returned in case of mistrial.

* `ARB_ASSIGNED` : As soon as the claimant accepts an offer, the case will move to ARB_ASSIGNED in this case. Whenever the arbitrator is ready to start the case investigation, it will call the startcase action.

* `CASE_INVESTIGATION` : When the arbitrator calls startcase, the case then enters the CASE_INVESTIGATION stage. In this stage the arbitrators review and assess the claims within the case to determine their validity. This stage may also involve communication between the claimant or the respondant and the arbitrator in situations that require clarification or more sufficient evidence.

* `DECISION` : Once all the claims has been settled, the arbitrator will set their decision calling setruling. The case will then enter in the DECISION stage.

* `ENFORCEMENT`: If the BPs find that the case and the arbitrator's decision are valid in the DECISION stage, they will call validatecase to move the case to the ENFORCEMENT status. In this stage, Block Producers will apply the ruling done by the arbitrator. Moreover, the arbitrator will receive the rate cost the claimant paid at the beginning of the case.

* `RESOLVED`: Once the ruling has been applied, the case will be closed and moved to the RESOLVED stage.

* `DISMISSED` : If during the DECISION stage the case is found to be invalid or having a non-sense ruling, the Block Producers may move the case to DISMISSED stage. In this scenario, all the funds (the fee + the arbitrator rate cost) will be returned to the claimant.

* `MISTRIAL` : If during the ARBS_ASSIGNED, CASE_INVESTIGATION or DECISION stage the arbitrator recuses (or is forced to recuse), the case will be considered a MISTRIAL and all the funds (the fee + the arbitrator rate cost) will be returned to the claimant.

