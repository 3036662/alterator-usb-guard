#include "message_reader.hpp"
#include "lisp_message.hpp"
#include "message_dispatcher.hpp"
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <unordered_map>

// MessageReader::MessageReader(guard::Guard &guard) noexcept
//     : dispatcher_(guard) {}

MessageReader::MessageReader(DispatchFunc dispatcher_func) noexcept
    : dispatcher_(std::move(dispatcher_func)) {}

void MessageReader::Loop() const noexcept {
  std::string line;
  bool msg_in_progress = false;
  LispMessage::MsgAction action;
  LispMessage::MsgObject objects;
  std::unordered_map<std::string, std::string> params;
  // read stdin loop
  while (std::getline(std::cin, line)) {
    boost::trim(line);
    // message begin
    if (boost::contains(line, "_message:begin")) {
      msg_in_progress = true;
      line.clear();
      continue;
    }
    // end of loop
    if (!boost::contains(line, "_message:begin") && !msg_in_progress) {
      break;
    }
    // the end of a message
    if (boost::contains(line, "_message:end")) {
      msg_in_progress = false;
      if (!action.val.empty() && !objects.val.empty()) {
        LispMessage request_message(action, objects, params);
        dispatcher_.Dispatch(request_message);
      }
      params.clear();
      action.val.clear();
      objects.val.clear();
      line.clear();
      continue;
    }
    // find action
    if (boost::starts_with(line, kStrAction)) {
      boost::erase_first(line, kStrAction);
      boost::trim(line);
      action.val = line;
      line.clear();
      continue;
    }
    // find object
    if (boost::starts_with(line, kStrObjects)) {
      boost::erase_first(line, kStrObjects);
      boost::trim(line);
      objects.val = line;
      line.clear();
      continue;
    }
    // find parameters
    size_t pos = line.find(':');
    if (pos != std::string::npos) {
      params.emplace(line.substr(0, pos), // param
                     line.substr(pos + 1) // value
      );
    }
    line.clear();
  }
}