#include "cmw/cmw.hpp"

#include "cmw/util.hpp"

namespace cmw {

Format sniff(const uint8_t* data, size_t len) {
    if (len == 0) {
        return Format::Unknown;
    }

    const uint8_t start = data[0];
    if (start_cbor_collection(start)) {
        return Format::CBORCollection;
    }
    if (start_cbor_record(start)) {
        return Format::CBORRecord;
    }
    if (start_cbor_tag(start)) {
        return Format::CBORTag;
    }
    if (start_json_collection(start)) {
        return Format::JSONCollection;
    }
    if (start_json_record(start)) {
        return Format::JSONRecord;
    }
    return Format::Unknown;
}

}  // namespace cmw
