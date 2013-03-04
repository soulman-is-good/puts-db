#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <stdexcept>

#include <cppconn/connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
	
#define EXAMPLE_HOST "localhost"
#define EXAMPLE_USER "root"
#define EXAMPLE_PASS "root"
#define EXAMPLE_DB "facecom"

using namespace std;
//using namespace sql::mysql;

int main(int argc, char *argv[]){
    if(argc == 123){
        cout<<"Use --help to find out about arguments"<<endl;
        return 0;
    }
    
string url(argc >= 2 ? argv[1] : EXAMPLE_HOST);
    const string user(argc >= 3 ? argv[2] : EXAMPLE_USER);
    const string pass(argc >= 4 ? argv[3] : EXAMPLE_PASS);
    const string database(argc >= 5 ? argv[4] : EXAMPLE_DB);

    cout << "Connector/C++ tutorial framework..." << endl;
    cout << endl;

	
    try {
	
	sql::Driver* driver = get_driver_instance();
        std::auto_ptr<sql::Connection> con(driver->connect(url, user, pass));
        con->setSchema(database);
        std::auto_ptr<sql::Statement> stmt(con->createStatement());
        std::auto_ptr<sql::ResultSet> res(stmt->executeQuery("SHOW TABLES;"));
	while (res->next())
                cout << "Table: " << res->getString("Tables_in_facecom") << endl;
    } catch (sql::SQLException &e) {
        /*
          The MySQL Connector/C++ throws three different exceptions:
	
          - sql::MethodNotImplementedException (derived from sql::SQLException)
          - sql::InvalidArgumentException (derived from sql::SQLException)
          - sql::SQLException (derived from std::runtime_error)
        */
        cout << "# ERR: SQLException in " << __FILE__;
        cout << "(" << __FUNCTION__ << ") on line " << __LINE__ << endl;
        /* Use what() (derived from std::runtime_error) to fetch the error message */
        cout << "# ERR: " << e.what();
        cout << " (MySQL error code: " << e.getErrorCode();
        cout << ", SQLState: " << e.getSQLState() << " )" << endl;
	
        return EXIT_FAILURE;
    }
	
    cout << "Done." << endl;
    return EXIT_SUCCESS;
    
    return 0;
}