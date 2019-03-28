#include <iostream>
#include <string>
#include <map>
#include <set>

#include "zdbpp.h"
using namespace zdb;

const std::map<std::string, std::string> data {
        {"Fry",                 "Ceci n'est pas une pipe"},
        {"Leela",               "Mona Lisa"},
        {"Bender",              "Bryllup i Hardanger"},
        {"Farnsworth",          "The Scream"},
        {"Zoidberg",            "Vampyre"},
        {"Amy",                 "Balcony"},
        {"Hermes",              "Cycle"},
        {"Nibbler",             "Day & Night"},
        {"Cubert",              "Hand with Reflecting Sphere"},
        {"Zapp",                "Drawing Hands"},
        {"Joey Mousepad",       "Ascending and Descending"}
};

const std::map<std::string, std::string> schema {
        { "mysql", "CREATE TABLE zild_t(id INTEGER AUTO_INCREMENT PRIMARY KEY, name VARCHAR(255), percent REAL, image BLOB);"},
        { "postgresql", "CREATE TABLE zild_t(id SERIAL PRIMARY KEY, name VARCHAR(255), percent REAL, image BYTEA);"},
        { "sqlite", "CREATE TABLE zild_t(id INTEGER PRIMARY KEY, name VARCHAR(255), percent REAL, image BLOB);"},
        { "oracle", "CREATE TABLE zild_t(id NUMBER GENERATED AS IDENTITY, name VARCHAR(255), percent REAL, image CLOB);"}
};

static void testCreateSchema(ConnectionPool& pool) {
        Connection con = pool.getConnection();
        try { con.execute("drop table zild_t;"); } catch (...) {}
        con.execute(schema.at(pool.getURL().protocol()).c_str());
}

static void testPrepared(ConnectionPool& pool) {
        double rows = 0.12;
        Connection con = pool.getConnection();
        // A plain explicit prepared statement
        PreparedStatement p1 = con.prepareStatement("insert into zild_t (name, percent, image) values(?, ?, ?);");
        con.beginTransaction();
        for (const auto &[name, img] : data) {
                rows += 1 + rows / 7;
                p1.bind(1, name);
                p1.bind(2, rows);
                p1.bind(3, img.c_str(), int(img.length() + 1)); // include terminating \0
                p1.execute();
        }
        // Implicit prepared statement. Any execute or executeQuery statement which
        // takes parameters are automatically translated into a prepared statement.
        // Here we also demonstrate how to set a SQL null value by using nullptr which
        // must be used instead of NULL
        con.execute("update zild_t set image = ? where id = ?", nullptr, 11);
        con.commit();
}

static void testQuery(ConnectionPool& pool) {
        Connection con = pool.getConnection();
        // Also translated into a prepared statement because use of parameter
        ResultSet result = con.executeQuery("select id, name, percent, image from zild_t where id < ? order by id;", 100);
        result.setFetchSize(100);
        assert(result.columnCount() == 4);
        assert(std::string(result.columnName(1)) == "id");
        while (result.next()) {
                int id = result.getInt(1);
                const char *name = result.getString("name");
                double percent = result.getDouble("percent");
                auto [blob, size] = result.getBlob(4);
                printf("\t%-5d%-20s%-10.2f%-16.38s\n", id, name ? name : "null", percent, size ? (char *)blob : "null");
        }
}

static void testDropSchema(ConnectionPool& pool) {
        Connection con = pool.getConnection();
        con.execute("drop table zild_t;");
}

int main(void) {
        auto help =
        "C++17 zdbpp.h Interface Test\n\n"
        "Please enter a valid database connection URL and press ENTER\n"
        "E.g. sqlite:///tmp/sqlite.db?synchronous=off&heap_limit=2000\n"
        "E.g. mysql://localhost:3306/test?user=root&password=root\n"
        "E.g. postgresql://localhost:5432/test?user=root&password=root\n"
        "E.g. oracle://scott:tiger@localhost:1521/servicename\n"
        "To exit, enter '.' on a single line\n\nConnection URL> ";
        std::cout << std::string(80, '=') + "\n" << help;
        for (std::string line; std::getline(std::cin, line);) {
                if (line == "q" || line == ".")
                        break;
                URL url(line);
                if (!url) {
                        std::cout << "Please enter a valid database URL or stop by entering '.'\n\n";
                        std::cout << "Connection URL> ";
                        continue;
                }
                ConnectionPool pool(line);
                pool.start();
                std::cout << std::string(50, '=') + "> Start Tests\n";
                testCreateSchema(pool);
                testPrepared(pool);
                testQuery(pool);
                testDropSchema(pool);
                std::cout << std::string(50, '=') + "> Tests: OK\n\n";
                std::cout << help;
        }
}
