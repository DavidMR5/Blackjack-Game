#include "agents/HumanAgent.hpp"

#include <cctype>
#include <istream>
#include <ostream>
#include <string>

namespace blackjack {

HumanAgent::HumanAgent(std::istream& in, std::ostream& out, Agent* advisor)
    : in_(in), out_(out), advisor_(advisor) {}

bool HumanAgent::wantsToQuit() const {
    return quit_;
}

Action HumanAgent::chooseAction(const State& state) {
    while (true) {
        out_ << "[H]it, [S]tand"
             << (advisor_ ? ", [?] hint" : "")
             << ", [Q]uit > " << std::flush;

        std::string line;

        if (!std::getline(in_, line)) {
            quit_ = true;
            return Action::Stand;
        }

        const char choice = line.empty()
            ? '\0'
            : static_cast<char>(std::tolower(static_cast<unsigned char>(line.front())));

        switch (choice) {
            case 'h':
                return Action::Hit;
            case 's':
                return Action::Stand;
            case 'q':
                quit_ = true;
                return Action::Stand;
            case '?':
                if (advisor_) {
                    out_ << "  Hint: " << actionToString(advisor_->chooseAction(state)) << '\n';
                    continue;
                }
                break;
            default:
                break;
        }

        out_ << "  Please type H or S.\n";
    }
}

}
