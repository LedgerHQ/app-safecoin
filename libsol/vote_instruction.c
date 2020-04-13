#include "common_byte_strings.h"
#include "sol/transaction_summary.h"
#include "util.h"
#include "vote_instruction.h"

const Pubkey vote_program_id = {{ PROGRAM_ID_VOTE }};

static int parse_vote_instruction_kind(
    Parser* parser,
    enum VoteInstructionKind* kind
) {
    uint32_t maybe_kind;
    BAIL_IF(parse_u32(parser, &maybe_kind));
    switch (maybe_kind) {
        case VoteInitialize:
        case VoteAuthorize:
        case VoteVote:
        case VoteWithdraw:
        case VoteUpdateNode:
            *kind = (enum VoteInstructionKind) maybe_kind;
            return 0;
    }
    return 1;
}

static int parse_vote_initialize_instruction(
    Parser* parser,
    const Instruction* instruction,
    const MessageHeader* header,
    VoteInitializeInfo* info
) {
    BAIL_IF(instruction->accounts_length < 3);
    size_t accounts_index = 0;
    size_t pubkeys_index = instruction->accounts[accounts_index++];
    info->account = &header->pubkeys[pubkeys_index];

    accounts_index++; // Skip rent sysvar
    accounts_index++; // Skip clock sysvar

    BAIL_IF(parse_pubkey(parser, &info->vote_init.node));
    BAIL_IF(parse_pubkey(parser, &info->vote_init.vote_authority));
    BAIL_IF(parse_pubkey(parser, &info->vote_init.withdraw_authority));
    uint8_t commission;
    BAIL_IF(parse_u8(parser, &commission));
    info->vote_init.commission = commission;

    return 0;
}

int parse_vote_instructions(
    const Instruction* instruction,
    const MessageHeader* header,
    VoteInfo* info
) {
    Parser parser = {instruction->data, instruction->data_length};

    BAIL_IF(parse_vote_instruction_kind(&parser, &info->kind));

    switch (info->kind) {
        case VoteInitialize:
            return parse_vote_initialize_instruction(
                &parser,
                instruction,
                header,
                &info->initialize
            );
        case VoteAuthorize:
        case VoteVote:
        case VoteWithdraw:
        case VoteUpdateNode:
            break;
    }

    return 1;
}

int print_vote_info(const VoteInfo* info, const MessageHeader* header) {
    switch (info->kind) {
        case VoteInitialize:
            return print_vote_initialize_info(
                "Init. vote acct.",
                &info->initialize,
                header
            );
        case VoteAuthorize:
        case VoteVote:
        case VoteWithdraw:
        case VoteUpdateNode:
            break;
    }

    return 1;
}

int print_vote_initialize_info(
    const char* primary_title,
    const VoteInitializeInfo* info,
    const MessageHeader* header
) {
    SummaryItem* item;
    if (primary_title != NULL) {
        item = transaction_summary_primary_item();
        summary_item_set_pubkey(item, primary_title, info->account);
    }

    item = transaction_summary_general_item();
    summary_item_set_pubkey(item, "Node", info->vote_init.node);

    item = transaction_summary_general_item();
    summary_item_set_pubkey(item, "New vote auth.", info->vote_init.vote_authority);

    item = transaction_summary_general_item();
    summary_item_set_pubkey(item, "New withdraw auth.", info->vote_init.withdraw_authority);

    item = transaction_summary_general_item();
    summary_item_set_u64(item, "Commission", info->vote_init.commission);

    return 0;
}