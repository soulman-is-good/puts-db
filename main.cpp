/**
 * MySQL stack daemon
 */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>

#include <cppconn/connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

#include "boost/date_time/posix_time/posix_time.hpp"

#define EXAMPLE_HOST "localhost"
#define EXAMPLE_USER "root"
#define EXAMPLE_PASS "root"
#define EXAMPLE_DB "test"

#define MAX_LOG_FILESIZE 5242880 //5Mb

//Daemon vars
#define RUNNING_DIR	"/tmp"
#define LOCK_FILE	"putsdb.lock"
#define LOG_FILE	"putsdb.log"

using namespace std;
using namespace boost::posix_time;
using namespace boost::gregorian;
//using namespace sql::mysql;

void logme(std::string message);
void signal_handler(int sig);
void daemonize();

int main(int argc, char *argv[]) {

    daemonize();

    const string user(argc >= 2 ? argv[1] : EXAMPLE_USER);
    const string pass(argc >= 3 ? argv[2] : EXAMPLE_PASS);
    const string database(argc >= 4 ? argv[3] : EXAMPLE_DB);
    string url(argc >= 5 ? argv[4] : EXAMPLE_HOST);
    std::stringstream s;
    s << "Starting daemon at " << url << "@" << database;
    logme(s.str());

    try {
        while (1) {
            sql::Driver* driver = get_driver_instance();
            std::auto_ptr<sql::Connection> con(driver->connect(url, user, pass));
            con->setSchema(database);
            std::auto_ptr<sql::Statement> stmt(con->createStatement());
            std::auto_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT * FROM `__x3_stack`"));
            while (res->next()) {
                string id(res->getString("id"));
                string q(res->getString("query"));
                try {
                    q += ';';
                    logme(string("Quering: ") + q);
                    stmt->execute(q);
                    q = string("DELETE FROM `__x3_stack` WHERE `id`=") + id + ';';
                    stmt->execute(q);
                } catch (sql::SQLException &e) {
                    std::stringstream s;
                    s << "Error in query #" << id << " Message: " << e.what() << " (MySQL error code: " << e.getErrorCode() << ", SQLState: " << e.getSQLState() << " )";
                    logme(s.str());
                }
            }
            sleep(10);
        }
    } catch (sql::SQLException &e) {
        /*
          The MySQL Connector/C++ throws three different exceptions:
	
          - sql::MethodNotImplementedException (derived from sql::SQLException)
          - sql::InvalidArgumentException (derived from sql::SQLException)
          - sql::SQLException (derived from std::runtime_error)
         */
        std::stringstream s;
        s << "Error handling database connection" << e.what() << " (MySQL error code: " << e.getErrorCode() << ", SQLState: " << e.getSQLState() << " )" << __LINE__;
        logme(s.str());

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/**
 * Log messages to a log file
 * @param message message string to log
 */
void logme(std::string message) {
    FILE *logfile;
    if (logfile = fopen(LOG_FILE, "r")) {
        fseek(logfile, 0L, SEEK_END);
        long size = ftell(logfile);
        fclose(logfile);
        if (size > MAX_LOG_FILESIZE) {
            //TODO: counter for log archive files
        }
    }
    logfile = fopen(LOG_FILE, "a");
    if (!logfile) return;
    ptime p = second_clock::local_time();
    static std::locale loc(std::wcout.getloc(),
            new wtime_facet(L"%d-%m-%Y %H:%M:%S"));

    std::basic_stringstream<char> wss;
    wss.imbue(loc);
    wss << p;
    const char *s = wss.str().c_str();
    fprintf(logfile, "[%s]>%s\n", s, message.c_str());
    fclose(logfile);
}

/**
 * Signame handler
 * @param sig signal
 */
void signal_handler(int sig) {
    switch (sig) {
        case SIGHUP:
            logme("Application just hangup");
            break;
        case SIGTERM:
            logme("Got SIGTERM. Terminating...");
            exit(0);
            break;
    }
}

/**
 * Demonize the process
 */
void daemonize() {
    int i, lfp;
    char str[10];
    if (getppid() == 1) return; /* already a daemon */
    i = fork();
    if (i < 0) exit(1); /* fork error */
    if (i > 0) exit(0); /* parent exits */
    /* child (daemon) continues */
    setsid(); /* obtain a new process group */
    for (i = getdtablesize(); i >= 0; --i) close(i); /* close all descriptors */
    i = open("/dev/null", O_RDWR);
    dup(i);
    dup(i); /* handle standart I/O */
    umask(027); /* set newly created file permissions */
    chdir(RUNNING_DIR); /* change running directory */
    /*Mutex logic further*/
    lfp = open(LOCK_FILE, O_RDWR | O_CREAT, 0640);
    if (lfp < 0) {
        logme("Can not create lock file...");
        exit(1); /* can not open */
    }
    if (lockf(lfp, F_TLOCK, 0) < 0) {
        logme("Can not lock process. Seems already running...");
        exit(0); /* can not lock */
    }
    /* first instance continues */
    sprintf(str, "%d\n", getpid());
    write(lfp, str, strlen(str)); /* record pid to lockfile */
    signal(SIGCHLD, SIG_IGN); /* ignore child */
    signal(SIGTSTP, SIG_IGN); /* ignore tty signals */
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGHUP, signal_handler); /* catch hangup signal */
    signal(SIGTERM, signal_handler); /* catch kill signal */
}