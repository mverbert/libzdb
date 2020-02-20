/*
 * Copyright (C) 2016 dragon jiang<jianlinlong@gmail.com>
 * Copyright (C) 2019 Tildeslash Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files(the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions :
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _ZDBPP_H_
#define _ZDBPP_H_

#include "zdb.h"
#include <string>
#include <utility>
#include <stdexcept>


namespace zdb {
    
    class sql_exception : public std::runtime_error
    {
    public:
        sql_exception(const char* msg = "SQLException")
        : std::runtime_error(msg)
        {}
    };
    
#define except_wrapper(f) TRY { f; } ELSE {throw sql_exception(Exception_frame.message);} END_TRY
    
    struct noncopyable
    {
        noncopyable() = default;
        
        // make it noncopyable
        noncopyable(noncopyable const&) = delete;
        noncopyable& operator=(noncopyable const&) = delete;
        
        // make it not movable
        noncopyable(noncopyable&&) = delete;
        noncopyable& operator=(noncopyable&&) = delete;
    };
    
    
    class URL: private noncopyable
    {
    public:
        URL(const std::string& url)
        :URL(url.c_str())
        {}
        
        URL(const char *url) {
            t_ = URL_new(url);
        }
        
        ~URL() {
            if (t_)
                URL_free(&t_);
        }
        
        operator URL_T() {
            return t_;
        }
        
    public:
        const char *protocol() const {
            return URL_getProtocol(t_);
        }
        
        const char *user() const {
            return URL_getUser(t_);
        }
        
        const char *password() const {
            return URL_getPassword(t_);
        }
        
        const char *host() const {
            return URL_getHost(t_);
        }
        
        int port() const {
            return URL_getPort(t_);
        }
        
        const char *path() const {
            return URL_getPath(t_);
        }
        
        const char *queryString() const {
            return URL_getQueryString(t_);
        }
        
        const char **parameterNames() const {
            return URL_getParameterNames(t_);
        }
        
        const char *parameter(const char *name) const {
            return URL_getParameter(t_, name);
        }
        
        const char *tostring() const {
            return URL_toString(t_);
        }
        
    private:
        URL_T t_;
    };
    
    class ResultSet : private noncopyable
    {
    public:
        operator ResultSet_T() {
            return t_;
        }
        
        ResultSet(ResultSet&& r)
        :t_(r.t_)
        {
            r.t_ = nullptr;
        }
        
    protected:
        friend class PreparedStatement;
        friend class Connection;
        
        ResultSet(ResultSet_T t)
        :t_(t)
        {}
        
    public:
        int columnCount() {
            return ResultSet_getColumnCount(t_);
        }
        
        const char *columnName(int columnIndex) {
            except_wrapper( RETURN ResultSet_getColumnName(t_, columnIndex) );
        }
        
        long columnSize(int columnIndex) {
            except_wrapper( RETURN ResultSet_getColumnSize(t_, columnIndex) );
        }

        void setFetchSize(int prefetch_rows) {
            ResultSet_setFetchSize(t_, prefetch_rows);
        }

        int getFetchSize() {
            return ResultSet_getFetchSize(t_);
        }

        bool next() {
            except_wrapper( RETURN ResultSet_next(t_) );
        }
        
        bool isnull(int columnIndex) {
            except_wrapper( RETURN ResultSet_isnull(t_, columnIndex) );
        }
        
        const char *getString(int columnIndex) {
            except_wrapper( RETURN ResultSet_getString(t_, columnIndex) );
        }
        
        const char *getString(const char *columnName) {
            except_wrapper( RETURN ResultSet_getStringByName(t_, columnName) );
        }
        
        int getInt(int columnIndex) {
            except_wrapper( RETURN ResultSet_getInt(t_, columnIndex) );
        }
        
        int getInt(const char *columnName) {
            except_wrapper( RETURN ResultSet_getIntByName(t_, columnName) );
        }
        
        long long getLLong(int columnIndex) {
            except_wrapper( RETURN ResultSet_getLLong(t_, columnIndex) );
        }
        
        long long getLLong(const char *columnName) {
            except_wrapper( RETURN ResultSet_getLLongByName(t_, columnName) );
        }
        
        double getDouble(int columnIndex) {
            except_wrapper( RETURN ResultSet_getDouble(t_, columnIndex) );
        }
        
        double getDouble(const char *columnName) {
            except_wrapper( RETURN ResultSet_getDoubleByName(t_, columnName) );
        }
        
        template <typename T>
        std::tuple<const void*, int> getBlob(T v) {
            int size = 0;
            const void *blob = NULL;
            if constexpr (std::is_integral<T>::value)
                except_wrapper( blob = ResultSet_getBlob(t_, v, &size) );
            else
                except_wrapper( blob = ResultSet_getBlobByName(t_, v, &size) );
            return {blob, size};
        }
        
        time_t getTimestamp(int columnIndex) {
            except_wrapper( RETURN ResultSet_getTimestamp(t_, columnIndex) );
        }
        
        time_t getTimestamp(const char *columnName) {
            except_wrapper( RETURN ResultSet_getTimestampByName(t_, columnName) );
        }
        
        struct tm getDateTime(int columnIndex) {
            except_wrapper( RETURN ResultSet_getDateTime(t_, columnIndex) );
        }
        
        struct tm getDateTime(const char *columnName) {
            except_wrapper( RETURN ResultSet_getDateTimeByName(t_, columnName) );
        }

    private:
        ResultSet_T t_;
    };
    
    class PreparedStatement : private noncopyable
    {
    public:
        operator PreparedStatement_T() {
            return t_;
        }
        
        PreparedStatement(PreparedStatement&& r)
        :t_(r.t_)
        {
            r.t_ = nullptr;
        }
        
    protected:
        friend class Connection;
        
        PreparedStatement(PreparedStatement_T t)
        :t_(t)
        {}
        
    public:
        void setString(int parameterIndex, const char *x) {
            except_wrapper( PreparedStatement_setString(t_, parameterIndex, x) );
        }
        
        void setInt(int parameterIndex, int x) {
            except_wrapper( PreparedStatement_setInt(t_, parameterIndex, x) );
        }
        
        void setLLong(int parameterIndex, long long x) {
            except_wrapper( PreparedStatement_setLLong(t_, parameterIndex, x) );
        }
        
        void setDouble(int parameterIndex, double x) {
            except_wrapper( PreparedStatement_setDouble(t_, parameterIndex, x) );
        }
        
        void setBlob(int parameterIndex, const void *x, int size) {
            except_wrapper( PreparedStatement_setBlob(t_, parameterIndex, x, size) );
        }
        
        void setTimestamp(int parameterIndex, time_t x) {
            except_wrapper( PreparedStatement_setTimestamp(t_, parameterIndex, x) );
        }
        
        void execute() {
            except_wrapper( PreparedStatement_execute(t_) );
        }
        
        ResultSet executeQuery() {
            except_wrapper(
                           ResultSet_T r = PreparedStatement_executeQuery(t_);
                           RETURN ResultSet(r);
                           );
        }
        
        long long rowsChanged() {
            return PreparedStatement_rowsChanged(t_);
        }
        
        int getParameterCount() {
            return PreparedStatement_getParameterCount(t_);
        }
        
    public:
        void bind(int parameterIndex, const char *x) {
            this->setString(parameterIndex, x);
        }
        
        void bind(int parameterIndex, const std::string& x) {
            this->setString(parameterIndex, x.c_str());
        }
        
        void bind(int parameterIndex, int x) {
            this->setInt(parameterIndex, x);
        }
        
        void bind(int parameterIndex, long long x) {
            this->setLLong(parameterIndex, x);
        }
        
        void bind(int parameterIndex, double x) {
            this->setDouble(parameterIndex, x);
        }
        
        void bind(int parameterIndex, time_t x) {
            this->setTimestamp(parameterIndex, x);
        }
        
        //blob
        void bind(int parameterIndex, std::tuple<const void *, int> x) {
            auto [blob, size] = x;
            this->setBlob(parameterIndex, blob, size);
        }
        
    private:
        PreparedStatement_T t_;
    };
    
    class Connection : private noncopyable
    {
    public:
        operator Connection_T() {
            return t_;
        }
        
        ~Connection() {
            if (t_) {
                close();
            }
        }
        
    protected:  // for ConnectionPool
        friend class ConnectionPool;
        
        Connection(Connection_T C)
        :t_(C)
        {}
        
        void setClosed() {
            t_ = nullptr;
        }
        
    public:
        void setQueryTimeout(int ms) {
            Connection_setQueryTimeout(t_, ms);
        }
        
        int getQueryTimeout() {
            return Connection_getQueryTimeout(t_);
        }
        
        void setMaxRows(int max) {
            Connection_setMaxRows(t_, max);
        }
        
        int getMaxRows() {
            return Connection_getMaxRows(t_);
        }
        
        void setFetchSize(int rows) {
            Connection_setFetchSize(t_, rows);
        }
        
        int getFetchSize() {
            return Connection_getFetchSize(t_);
        }
        
        //not supported
        //URL_T Connection_getURL(T C);
        
        bool ping() {
            return Connection_ping(t_);
        }
        
        void clear() {
            Connection_clear(t_);
        }
        
        //after close(), t_ is set to NULL. so this Connection object can not be used again!
        void close() {
            if (t_) {
                Connection_close(t_);
                setClosed();
            }
        }
        
        void beginTransaction() {
            except_wrapper( Connection_beginTransaction(t_) );
        }
        
        void commit() {
            except_wrapper( Connection_commit(t_) );
        }
        
        void rollback() {
            except_wrapper( Connection_rollback(t_) );
        }
        
        long long lastRowId() {
            return Connection_lastRowId(t_);
        }
        
        long long rowsChanged() {
            return Connection_rowsChanged(t_);
        }
        
        void execute(const char *sql) {
            except_wrapper( Connection_execute(t_, "%s", sql) );
        }
        
        template <typename ...Args>
        void execute(const char *sql, Args ... args) {
            PreparedStatement p(this->prepareStatement(sql, args...));
            p.execute();
        }
        
        ResultSet executeQuery(const char *sql) {
            except_wrapper(
                           ResultSet_T r = Connection_executeQuery(t_, "%s", sql);
                           RETURN ResultSet(r);
                           );
        }
        
        template <typename ...Args>
        ResultSet executeQuery(const char *sql, Args ... args) {
            PreparedStatement p(this->prepareStatement(sql, args...));
            return p.executeQuery();
        }
        
        PreparedStatement prepareStatement(const char *sql) {
            except_wrapper(
                           PreparedStatement_T p = Connection_prepareStatement(t_, "%s", sql);
                           RETURN PreparedStatement(p);
                           );
        }
        
        template <typename ...Args>
        PreparedStatement prepareStatement(const char *sql, Args ... args) {
            except_wrapper(
                           PreparedStatement p(this->prepareStatement(sql));
                           int i = 1;
                           (p.bind(i++, args), ...);
                           RETURN p;
                           );
        }
        
        const char *getLastError() {
            return Connection_getLastError(t_);
        }
        
        static bool isSupported(const char *url) {
            return Connection_isSupported(url);
        }
        
    private:
        Connection_T t_;
    };
    
    
    class ConnectionPool : private noncopyable
    {
    public:
        ConnectionPool(const std::string& url)
        :ConnectionPool(url.c_str())
        {}
        
        ConnectionPool(const char* url)
        :url_(url)
        {
            if (!url_)
                throw sql_exception("Invalid URL");
            t_ = ConnectionPool_new(url_);
        }
        
        ~ConnectionPool() {
            ConnectionPool_free(&t_);
        }
        
        operator ConnectionPool_T() {
            return t_;
        }
        
    public:
        const URL& getURL() {
            return url_;
        }
        
        void setInitialConnections(int connections) {
            ConnectionPool_setInitialConnections(t_, connections);
        }
        
        int getInitialConnections() {
            return ConnectionPool_getInitialConnections(t_);
        }
        
        void setMaxConnections(int maxConnections) {
            ConnectionPool_setMaxConnections(t_, maxConnections);
        }
        
        int getMaxConnections() {
            return ConnectionPool_getMaxConnections(t_);
        }
        
        void setConnectionTimeout(int connectionTimeout) {
            ConnectionPool_setConnectionTimeout(t_, connectionTimeout);
        }
        
        int getConnectionTimeout() {
            return ConnectionPool_getConnectionTimeout(t_);
        }
        
        void setAbortHandler(void(*abortHandler)(const char *error)) {
            ConnectionPool_setAbortHandler(t_, abortHandler);
        }
        
        void setReaper(int sweepInterval) {
            ConnectionPool_setReaper(t_, sweepInterval);
        }
        
        int size() {
            return ConnectionPool_size(t_);
        }
        
        int active() {
            return ConnectionPool_active(t_);
        }
        
        void start() {
            except_wrapper( ConnectionPool_start(t_) );
        }
        
        void stop() {
            ConnectionPool_stop(t_);
        }
        
        Connection getConnection() {
            Connection_T C = ConnectionPool_getConnection(t_);
            if (!C) {
                throw sql_exception("maxConnection is reached (got null connection)!");
            }
            return Connection(C);
        }
        
        void returnConnection(Connection& con) {
            con.close();
        }
        
        int reapConnections() {
            return ConnectionPool_reapConnections(t_);
        }
        
        static const char *version(void) {
            return ConnectionPool_version();
        }
        
    private:
        URL url_;
        ConnectionPool_T t_;
    };
    
    
} // namespace

#endif
