//
// Created by munenaga on 2020/01/25.
//

#include "common.h"
#include "http_response_t.h"
#include "http_constants_t.h"
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>

std::string http_response_t::to_string() const {
    std::vector<std::string> lines;
    const auto status_line = (boost::format("HTTP/1.1 %d Success") % this->status_code).str();
    lines.push_back(status_line);

    for(const auto& header_item: this->header) {
        const auto line = (boost::format("%s: %s") % header_item.first % header_item.second).str();
        lines.push_back(line);
    }

    const auto content_length = this->body.size();
    lines.push_back(
        (boost::format("Content-Length: %d") % content_length).str()
    );

    lines.push_back(http_constants_t::CRLF);

    return boost::join(lines, http_constants_t::CRLF) + this->body;
}
