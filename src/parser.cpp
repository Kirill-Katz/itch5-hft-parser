#include "parser.hpp"

#include <stdint.h>
#include <cstring>

inline uint64_t load_be48(const std::byte* p) {
    return (uint64_t(p[0]) << 40) |
           (uint64_t(p[1]) << 32) |
           (uint64_t(p[2]) << 24) |
           (uint64_t(p[3]) << 16) |
           (uint64_t(p[4]) << 8)  |
           uint64_t(p[5]);
}

inline uint64_t load_be64(const std::byte* p) {
    return (uint64_t(p[0]) << 56) |
           (uint64_t(p[1]) << 48) |
           (uint64_t(p[2]) << 40) |
           (uint64_t(p[3]) << 32) |
           (uint64_t(p[4]) << 24) |
           (uint64_t(p[5]) << 16) |
           (uint64_t(p[6]) << 8)  |
           uint64_t(p[7]);
}

inline uint32_t load_be32(const std::byte* p) {
    return (uint32_t(p[0]) << 24) |
           (uint32_t(p[1]) << 16) |
           (uint32_t(p[2]) << 8)  |
           uint32_t(p[3]);
}

inline uint16_t load_be16(const std::byte* p) {
    return (uint16_t(p[0]) << 8) | uint16_t(p[1]);
}

SystemEvent parse_system_event(std::byte const * src) {
    SystemEvent sysEvent;
    sysEvent.stock_locate = load_be16(src);
    src += 2;
    sysEvent.tracking_number = load_be16(src);
    src += 2;
    sysEvent.timestamp = load_be48(src);
    src += 6;
    sysEvent.event_code = static_cast<char>(src[0]);
    src += 1;
    return sysEvent;
}

StockDirectory parse_stock_directory(std::byte const * src) {
    StockDirectory stockDir;
    stockDir.stock_locate = load_be16(src);
    src += 2;
    stockDir.tracking_number = load_be16(src);
    src += 2;
    stockDir.timestamp = load_be48(src);
    src += 6;
    std::memcpy(stockDir.stock, src, 8);
    src += 8;
    stockDir.market_category = char(src[0]);
    src += 1;
    stockDir.financial_status_indicator = char(src[0]);
    src += 1;
    stockDir.round_lot_size = load_be32(src);
    src += 4;
    stockDir.round_lots_only = static_cast<char>(src[0]);
    src += 1;
    stockDir.issue_classification = static_cast<char>(src[0]);
    src += 1;
    std::memcpy(&stockDir.issue_sub_type, src, 2);
    src += 2;
    stockDir.authenticity = static_cast<char>(src[0]);
    src += 1;
    stockDir.short_sale_threshold_indicator = static_cast<char>(src[0]);
    src += 1;
    stockDir.ipo_flag = static_cast<char>(src[0]);
    src += 1;
    stockDir.luld_reference_price_tier = static_cast<char>(src[0]);
    src += 1;
    stockDir.etp_flag = static_cast<char>(src[0]);
    src += 1;
    stockDir.etp_leverage_factor = load_be32(src);
    src += 4;
    stockDir.inverse_indicator = static_cast<char>(src[0]);
    src += 1;

    return stockDir;
}

TradingAction parse_trading_action(std::byte const * src) {
    TradingAction tradingAction;

    tradingAction.stock_locate = load_be16(src);
    src += 2;
    tradingAction.tracking_number = load_be16(src);
    src += 2;
    tradingAction.timestamp = load_be48(src);
    src += 6;
    std::memcpy(tradingAction.stock, src, 8);
    src += 8;
    tradingAction.trading_state = static_cast<char>(src[0]);
    src += 1;
    tradingAction.reserved = static_cast<char>(src[0]);
    src += 1;
    std::memcpy(tradingAction.stock, src, 4);
    src += 4;

    return tradingAction;
}

RegSho parse_reg_sho(std::byte const * src) {
    RegSho regSho;

    regSho.locate_code = load_be16(src);
    src += 2;
    regSho.tracking_number = load_be16(src);
    src += 2;
    regSho.timestamp = load_be48(src);
    src += 6;
    std::memcpy(regSho.stock, src, 8);
    src += 8;
    regSho.reg_sho_action = static_cast<char>(src[0]);
    src += 1;

    return regSho;
}

MarketParticipantPosition parse_market_participant_pos(std::byte const * src) {
    MarketParticipantPosition marketPartPos;

    marketPartPos.stock_locate = load_be16(src);
    src += 2;
    marketPartPos.tracking_number = load_be16(src);
    src += 2;
    marketPartPos.timestamp = load_be48(src);
    src += 6;
    std::memcpy(marketPartPos.mpid, src, 4);
    src += 4;
    std::memcpy(marketPartPos.stock, src, 8);
    src += 8;
    marketPartPos.primary_market_maker = static_cast<char>(src[0]);
    src += 1;
    marketPartPos.market_maker_mode = static_cast<char>(src[0]);
    src += 1;
    marketPartPos.market_participant_state = static_cast<char>(src[0]);
    src += 1;

    return marketPartPos;
}

MwcbDeclineLevel parse_mwcb_decline_level(std::byte const * src) {
    MwcbDeclineLevel mwcbDeclineLevel;

    mwcbDeclineLevel.stock_locate = load_be16(src);
    src += 2;
    mwcbDeclineLevel.tracking_number = load_be16(src);
    src += 2;
    mwcbDeclineLevel.timestamp = load_be48(src);
    src += 6;

    mwcbDeclineLevel.level1 = load_be64(src);
    src += 8;
    mwcbDeclineLevel.level2 = load_be64(src);
    src += 8;
    mwcbDeclineLevel.level3 = load_be64(src);
    src += 8;

    return mwcbDeclineLevel;
}

MwcbStatus parse_mwcb_status_message(std::byte const * src) {
    MwcbStatus mwcbStatus;

    mwcbStatus.stock_locate = load_be16(src);
    src += 2;
    mwcbStatus.tracking_number = load_be16(src);
    src += 2;
    mwcbStatus.timestamp = load_be48(src);
    src += 6;
    mwcbStatus.breached_level = static_cast<char>(src[0]);
    src += 1;

    return mwcbStatus;
}

IPOQuotingPeriodUpd parse_ipo_quoting_period_upd(std::byte const * src) {
    IPOQuotingPeriodUpd ipoQuotingPerUpd;

    ipoQuotingPerUpd.stock_locate = load_be16(src);
    src += 2;
    ipoQuotingPerUpd.tracking_number = load_be16(src);
    src += 2;
    ipoQuotingPerUpd.timestamp = load_be48(src);
    src += 6;
    std::memcpy(ipoQuotingPerUpd.stock, src, 8);
    src += 8;
    ipoQuotingPerUpd.ipo_quotation_release_time = load_be32(src);
    src += 4;
    ipoQuotingPerUpd.ipo_quatation_release_qualifier = static_cast<char>(src[0]);
    src += 1;
    ipoQuotingPerUpd.ipo_price = load_be32(src);
    src += 4;

    return ipoQuotingPerUpd;
};

LuldAuctionCollar parse_luld_auction_collar(std::byte const * src) {


}

template <typename Handler>
void ItchParser::parse(std::byte const *  src, size_t len, Handler& handler) {
    std::byte const * end = src + len;
    while (src < end) {
        uint16_t size = load_be16(src);
        src += 2;

        auto type = *reinterpret_cast<MessageType const *>(src);
        Message msg{};
        msg.type = type;
        switch (type) {
            case MessageType::SYSTEM_EVENT: {
                msg.system_event = parse_system_event(src);
                handler.handle(msg);
                break;
            }

            case MessageType::STOCK_DIRECTORY: {
                msg.stock_directory = parse_stock_directory(src);
                handler.handle(msg);
                break;
            }

            case MessageType::STOCK_TRADING_ACTION: {
                msg.trading_action = parse_trading_action(src);
                handler.handle(msg);
                break;
            }

            case MessageType::REG_SHO: {
                msg.reg_sho = parse_reg_sho(src);
                handler.handle(msg);
                break;
            }

            case MessageType::MARKET_PARTICIPANT_POSITION: {
                msg.market_participant_pos = parse_market_participant_pos(src);
                handler.handle(msg);
                break;
            }

            case MessageType::MWCB_DECLINE_LEVEL_MESSAGE:
                break;

            case MessageType::MWCB_STATUS_MESSAGE:
                break;

            case MessageType::IPO_QUOTING_PERIOD_UPD:
                break;

            case MessageType::LULD_AUCTION_COLLAR:
                break;

            case MessageType::OPERATIONAL_HALT:
                break;

            case MessageType::ADD_ORDER_NO_MPID:
                break;

            case MessageType::ADD_ORDER_MPID:
                break;

            case MessageType::ORDER_EXECUTED:
                break;

            case MessageType::ORDER_EXECUTED_PRICE:
                break;

            case MessageType::ORDER_CANCEL:
                break;

            case MessageType::ORDER_DELETE:
                break;

            case MessageType::ORDER_REPLACE:
                break;

            case MessageType::NON_CROSS_TRADE_MSG:
                break;

            case MessageType::CROSS_TRADE_MSG:
                break;

            case MessageType::BROKEN_TRADE_MSG:
                break;

            case MessageType::NOII_MSG:
                break;

            case MessageType::DIRECT_LISTING_CAPITAL_RAISE:
                break;

            default:
                break;
        }


        src += size;
    }
}
