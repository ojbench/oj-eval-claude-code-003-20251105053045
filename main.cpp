#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <unordered_map>
#include <algorithm>
#include <sstream>
#include <queue>

using namespace std;

struct Submission {
    string problem;
    string team;
    string status;
    int time;
    int submit_id;
};

struct ProblemStatus {
    int attempts_before_freeze = 0;
    int attempts_after_freeze = 0;
    bool solved = false;
    int solve_time = -1;
    bool frozen = false;

    int getPenaltyTime() const {
        if (!solved) return 0;
        return 20 * attempts_before_freeze + solve_time;
    }

    string getDisplayStatus() const {
        if (frozen) {
            if (attempts_before_freeze == 0) {
                return "0/" + to_string(attempts_after_freeze);
            }
            return to_string(attempts_before_freeze) + "/" + to_string(attempts_after_freeze);
        } else if (solved) {
            if (attempts_before_freeze == 0) return "+";
            return "+" + to_string(attempts_before_freeze);
        } else {
            if (attempts_before_freeze == 0) return ".";
            return "-" + to_string(attempts_before_freeze);
        }
    }
};

struct Team {
    string name;
    unordered_map<char, ProblemStatus> problems;
    vector<Submission> submissions;

    int getSolvedCount() const {
        int count = 0;
        for (const auto& p : problems) {
            if (p.second.solved && !p.second.frozen) count++;
        }
        return count;
    }

    int getTotalPenalty() const {
        int total = 0;
        for (const auto& p : problems) {
            if (p.second.solved && !p.second.frozen) {
                total += p.second.getPenaltyTime();
            }
        }
        return total;
    }

    vector<int> getSolveTimes() const {
        vector<int> times;
        for (const auto& p : problems) {
            if (p.second.solved && !p.second.frozen) {
                times.push_back(p.second.solve_time);
            }
        }
        sort(times.rbegin(), times.rend());
        return times;
    }
};

struct TeamRanking {
    string name;
    int solved_count;
    int penalty_time;
    vector<int> solve_times;

    bool operator<(const TeamRanking& other) const {
        if (solved_count != other.solved_count) {
            return solved_count > other.solved_count;
        }
        if (penalty_time != other.penalty_time) {
            return penalty_time < other.penalty_time;
        }
        for (size_t i = 0; i < min(solve_times.size(), other.solve_times.size()); i++) {
            if (solve_times[i] != other.solve_times[i]) {
                return solve_times[i] < other.solve_times[i];
            }
        }
        return name < other.name;
    }
};

class ICPCManagementSystem {
private:
    map<string, Team> teams;
    bool competition_started = false;
    bool competition_ended = false;
    int duration_time = 0;
    int problem_count = 0;
    bool frozen = false;
    int freeze_time = -1;
    vector<TeamRanking> last_rankings;
    vector<Submission> all_submissions;
    int next_submit_id = 1;

    vector<string> tokenize(const string& line) {
        vector<string> tokens;
        istringstream iss(line);
        string token;
        while (iss >> token) {
            tokens.push_back(token);
        }
        return tokens;
    }

    void updateRankings() {
        last_rankings.clear();
        for (const auto& team_pair : teams) {
            const Team& team = team_pair.second;
            TeamRanking ranking;
            ranking.name = team.name;
            ranking.solved_count = team.getSolvedCount();
            ranking.penalty_time = team.getTotalPenalty();
            ranking.solve_times = team.getSolveTimes();
            last_rankings.push_back(ranking);
        }
        sort(last_rankings.begin(), last_rankings.end());
    }

    int getTeamRanking(const string& team_name) {
        for (size_t i = 0; i < last_rankings.size(); i++) {
            if (last_rankings[i].name == team_name) {
                return i + 1;
            }
        }
        return -1;
    }

    void printScoreboard() {
        for (size_t i = 0; i < last_rankings.size(); i++) {
            const string& team_name = last_rankings[i].name;
            const Team& team = teams.at(team_name);
            cout << team_name << " " << (i + 1) << " "
                 << last_rankings[i].solved_count << " " << last_rankings[i].penalty_time;

            for (char c = 'A'; c < 'A' + problem_count; c++) {
                cout << " ";
                if (team.problems.find(c) != team.problems.end()) {
                    cout << team.problems.at(c).getDisplayStatus();
                } else {
                    cout << ".";
                }
            }
            cout << endl;
        }
    }

public:
    void processCommands() {
        string line;
        while (getline(cin, line)) {
            vector<string> tokens = tokenize(line);
            if (tokens.empty()) continue;

            if (tokens[0] == "ADDTEAM") {
                handleAddTeam(tokens);
            } else if (tokens[0] == "START") {
                handleStart(tokens);
            } else if (tokens[0] == "SUBMIT") {
                handleSubmit(tokens);
            } else if (tokens[0] == "FLUSH") {
                handleFlush();
            } else if (tokens[0] == "FREEZE") {
                handleFreeze();
            } else if (tokens[0] == "SCROLL") {
                handleScroll();
            } else if (tokens[0] == "QUERY_RANKING") {
                handleQueryRanking(tokens);
            } else if (tokens[0] == "QUERY_SUBMISSION") {
                handleQuerySubmission(tokens);
            } else if (tokens[0] == "END") {
                handleEnd();
                break;
            }
        }
    }

    void handleAddTeam(const vector<string>& tokens) {
        string team_name = tokens[1];
        if (competition_started) {
            cout << "[Error]Add failed: competition has started." << endl;
            return;
        }
        if (teams.find(team_name) != teams.end()) {
            cout << "[Error]Add failed: duplicated team name." << endl;
            return;
        }
        Team team;
        team.name = team_name;
        teams[team_name] = team;
        cout << "[Info]Add successfully." << endl;
    }

    void handleStart(const vector<string>& tokens) {
        if (competition_started) {
            cout << "[Error]Start failed: competition has started." << endl;
            return;
        }
        duration_time = stoi(tokens[2]);
        problem_count = stoi(tokens[4]);
        competition_started = true;
        cout << "[Info]Competition starts." << endl;
    }

    void handleSubmit(const vector<string>& tokens) {
        string problem = tokens[1];
        string team = tokens[3];
        string status = tokens[5];
        int time = stoi(tokens[7]);

        Submission sub;
        sub.problem = problem;
        sub.team = team;
        sub.status = status;
        sub.time = time;
        sub.submit_id = next_submit_id++;

        all_submissions.push_back(sub);
        teams[team].submissions.push_back(sub);

        char prob_char = problem[0];
        ProblemStatus& prob_status = teams[team].problems[prob_char];

        if (frozen && time > freeze_time && !prob_status.solved) {
            prob_status.frozen = true;
            if (status != "Accepted") {
                prob_status.attempts_after_freeze++;
            }
        } else if (!prob_status.solved) {
            if (status != "Accepted") {
                prob_status.attempts_before_freeze++;
            } else {
                prob_status.solved = true;
                prob_status.solve_time = time;
            }
        }
    }

    void handleFlush() {
        updateRankings();
        cout << "[Info]Flush scoreboard." << endl;
    }

    void handleFreeze() {
        if (frozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen." << endl;
            return;
        }
        frozen = true;
        freeze_time = all_submissions.empty() ? 0 : all_submissions.back().time;
        cout << "[Info]Freeze scoreboard." << endl;
    }

    void handleScroll() {
        if (!frozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen." << endl;
            return;
        }

        cout << "[Info]Scroll scoreboard." << endl;

        // First, flush scoreboard
        updateRankings();

        // Print scoreboard before scrolling
        printScoreboard();

        vector<pair<string, pair<int, int>>> ranking_changes;

        // Get current rankings before unfreezing
        vector<string> old_ranking_order;
        for (const auto& r : last_rankings) {
            old_ranking_order.push_back(r.name);
        }

        // Unfreeze all problems
        for (auto& team_pair : teams) {
            string team_name = team_pair.first;
            Team& team = team_pair.second;

            for (auto& prob_pair : team.problems) {
                if (prob_pair.second.frozen) {
                    char prob_char = prob_pair.first;
                    ProblemStatus& prob_status = prob_pair.second;

                    // Check if solved after freeze
                    for (const auto& sub : team.submissions) {
                        if (sub.problem[0] == prob_char && sub.time > freeze_time) {
                            if (sub.status == "Accepted" && !prob_status.solved) {
                                prob_status.solved = true;
                                prob_status.solve_time = sub.time;
                                break;
                            }
                        }
                    }
                    prob_status.frozen = false;
                }
            }
        }

        // Update rankings after unfreezing
        updateRankings();

        // Print ranking changes that caused position changes
        for (size_t i = 0; i < last_rankings.size(); i++) {
            const string& team_name = last_rankings[i].name;

            // Find old position
            size_t old_pos = 0;
            for (old_pos = 0; old_pos < old_ranking_order.size(); old_pos++) {
                if (old_ranking_order[old_pos] == team_name) break;
            }

            // If team moved up in ranking (position number decreased)
            if (i < old_pos) {
                int new_solved = teams[team_name].getSolvedCount();
                int new_penalty = teams[team_name].getTotalPenalty();

                // Find which team was displaced
                for (size_t j = i; j < old_pos; j++) {
                    const string& displaced_team = last_rankings[j].name;
                    cout << team_name << " " << displaced_team << " "
                         << new_solved << " " << new_penalty << endl;
                    break;
                }
            }
        }

        // Print final scoreboard
        printScoreboard();

        frozen = false;
        freeze_time = -1;
    }

    void handleQueryRanking(const vector<string>& tokens) {
        string team_name = tokens[1];
        if (teams.find(team_name) == teams.end()) {
            cout << "[Error]Query ranking failed: cannot find the team." << endl;
            return;
        }
        cout << "[Info]Complete query ranking." << endl;
        if (frozen) {
            cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled." << endl;
        }

        // Before first flush, ranking is based on lexicographic order
        if (last_rankings.empty()) {
            vector<string> team_names;
            for (const auto& team_pair : teams) {
                team_names.push_back(team_pair.first);
            }
            sort(team_names.begin(), team_names.end());
            for (size_t i = 0; i < team_names.size(); i++) {
                if (team_names[i] == team_name) {
                    cout << team_name << " NOW AT RANKING " << (i + 1) << endl;
                    return;
                }
            }
        } else {
            cout << team_name << " NOW AT RANKING " << getTeamRanking(team_name) << endl;
        }
    }

    void handleQuerySubmission(const vector<string>& tokens) {
        string team_name = tokens[1];
        string problem = tokens[3];
        string status = tokens[5];

        if (teams.find(team_name) == teams.end()) {
            cout << "[Error]Query submission failed: cannot find the team." << endl;
            return;
        }

        cout << "[Info]Complete query submission." << endl;

        const Submission* result = nullptr;
        for (auto it = all_submissions.rbegin(); it != all_submissions.rend(); ++it) {
            if (it->team == team_name) {
                bool problem_match = (problem == "ALL" || it->problem == problem);
                bool status_match = (status == "ALL" || it->status == status);
                if (problem_match && status_match) {
                    result = &(*it);
                    break;
                }
            }
        }

        if (result == nullptr) {
            cout << "Cannot find any submission." << endl;
        } else {
            cout << result->team << " " << result->problem << " "
                 << result->status << " " << result->time << endl;
        }
    }

    void handleEnd() {
        cout << "[Info]Competition ends." << endl;
        competition_ended = true;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    ICPCManagementSystem system;
    system.processCommands();

    return 0;
}