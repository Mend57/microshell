#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>
#include <cstdlib>
#include <sstream>
#include <vector>
#include <cstring>
#include <csignal>
#include <ctime>

using namespace std;

time_t startTime = 0;

void printTime() {
    time_t end = time(nullptr);
    int seconds = difftime(end, startTime);

    int h = seconds / 3600;
    int m = (seconds % 3600) / 60;
    int s = seconds % 60;

    cout << "Elapsed time: " << h << " Hours, "
         << m << " Minutes, "
         << s << " Seconds\n";
}

void sigint_handler(int) {
    cout << "\nDo you really want to exit? (yes/no) ? ";
    string answer;
    getline(cin, answer);

    if (answer == "y" || answer == "yes" || answer == "YES"|| answer == "Y" ) {
        printTime();
        exit(0);
    }
}

void sigchld_handler(int) {
    int status;
    pid_t pid;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        cout << "\nProcess " << pid << " finished" << endl;
    }
}

vector<string> read_command() {
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));

    cout << cwd << " > ";

    string line;
    getline(cin, line);

    vector<string> tokens;
    stringstream ss(line);
    string part;

    while (ss >> part) tokens.push_back(part);

    return tokens;
}

int main() {
    startTime = time(nullptr);
    signal(SIGINT, sigint_handler);
    signal(SIGCHLD, sigchld_handler);

    while (true) {
        vector<string> tokens = read_command();

        if (tokens.empty()) continue;

        string cmd = tokens[0];

        if (cmd == "exit") {
            printTime();
            return 0;
        }

        if (cmd == "cd") {
            const char* dir = (tokens.size() >= 2) ? tokens[1].c_str() : getenv("HOME");
            if (!dir) dir = "/";
            if (chdir(dir) != 0) {
                perror("cd");
            }
            continue;
        }

        bool background = false;
        if (!tokens.empty() && tokens.back() == "&") {
            background = true;
            tokens.pop_back();
        }

        vector<char*> args;
        for (auto &t : tokens) {
            args.push_back(const_cast<char*>(t.c_str()));
        }
        args.push_back(nullptr);

        pid_t childPid = fork();

        if (childPid < 0) {
            perror("fork");
            continue;
        }

        if (childPid == 0) {
            execvp(args[0], args.data());
            perror("execvp");
            exit(1);
        }

        if (background) {
            cout << "[" << childPid << "]\n";
        } else {
            wait(&status);;
        }
    }
}
