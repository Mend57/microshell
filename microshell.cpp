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
    int totalSeconds = difftime(end, startTime);
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;

    fprintf(stdout, "Elapsed time: %d Hours, %d Minutes, %d Seconds\n", hours, minutes, seconds);
}

void sigint_handler(int) {
    fprintf(stdout, "\nDo you really want to exit? (yes/no) ? ");
    fflush(stdout);

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
        fprintf(stdout, "\nProcess %d finished\n", pid);
    }
}

vector<string> read_command() {
    char cwd[PATH_MAX];
    getcwd(cwd, sizeof(cwd));
    fprintf(stdout, "%s > ", cwd);
    fflush(stdout);

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
    int status;
    int childPid;

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
            if (chdir(dir) != 0) fprintf(stderr, "cd: %s\n", strerror(errno));
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

        if ((childPid = fork()) == -1) {
            fprintf(stderr,"can't fork\n");
            exit(1);
        }
        if (childPid == 0) {
            execvp(args[0], args.data());
            fprintf(stderr, "microshell: Command not found: %s\n", cmd.c_str());
            exit(1);
        }

        if (background) {
            fprintf(stdout, "[%d]\n", childPid);
        } else {
            wait(&status);
        }
    }
}
