#pragma once

#include <qcbor/qcbor.h>

#include "cmw/cmw.hpp"
#include "cmw/result.hpp"

namespace cmw::cbor_internal {

Result<void> decode_monad_from_map_entry(QCBORDecodeContext& ctx, const QCBORItem& entry, CMW& out);
Result<void> decode_collection_from_context(QCBORDecodeContext& ctx, const QCBORItem& map_item,
                                            CMW& out);

}  // namespace cmw::cbor_internal
